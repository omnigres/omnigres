#!/usr/bin/env bash
# Script that generates an SQL upgrade between two revisions: revision passed at $VER
# (defaults to HEAD^) and the current HEAD.
# ```shell
#  PG_CONFIG=.pg/<OS>-Debug/<VERSION>/build/bin/pg_config VER=<git rev> ./generate-upgrades.sh
# ```

script_dir=$(dirname "$0")

BUILD_DIR=${BUILD_DIR:=$script_dir/build}
DEST_DIR=${BUILD_DIR}/_migrations
VER=${VER:=HEAD^}

PG_CONFIG=${PG_CONFIG:=pg_config}
PG_BINDIR=$($PG_CONFIG --bindir)

git_dir=$(git rev-parse --git-dir)


revision() {
    rev=$(git rev-parse --short $1)
    if [ ! -f "${DEST_DIR}/$rev/.complete" ]; then
      rm -rf "${DEST_DIR}/$rev" # in case if there was a prior failure to complete
      echo -n "Cloning $rev... "
      git clone --shared ${script_dir} "${DEST_DIR}/$rev" 2>/dev/null
      git -C "${DEST_DIR}/$rev" checkout $rev
      echo "done."
      # Try to speed builds up by sharing CPM modules
      export CPM_SOURCE_CACHE=$DEST_DIR/.cpm_cache
      cmake -S "${DEST_DIR}/$rev" -B "${DEST_DIR}/$rev/build" -DPG_CONFIG=$PG_CONFIG
      cmake --build "${DEST_DIR}/$rev/build" --parallel || exit 1
      cmake --build "${DEST_DIR}/$rev/build" --parallel --target package || exit 1
      # We're done
      touch "${DEST_DIR}/$rev/.complete"
    fi
    # For every extension in the current revision
    for subdir in "$script_dir"/extensions/*/; do
        ext=$(basename $subdir)
        if [ "$ext" == "omni_ext" ]; then
          # Skip omni_ext, we don't handle it right now
          continue
        fi
        # Get a version of this extension from this particular revision
        echo "Processing $ext for $rev"
        # If it is not present, continue with the next extension
        if [ ! -d "$DEST_DIR/$rev/extensions/$ext" ]; then
          echo "Extension $ext not present in $rev, skipping"
          continue
        fi
        # Prepare the migration specific for that version
        rm -f "$DEST_DIR/$ext--$rev.sql"
        for f in $(ls "$DEST_DIR/$rev/extensions/$ext/migrate/"*.sql | sort -V) # use of `sort -V` is critical (mirrors what we do in the build system)
          do
            $BUILD_DIR/misc/inja/inja "$f" >> "$DEST_DIR/$ext--$rev.sql"
            # Ensure there is a newline
            echo >> "$DEST_DIR/$ext--$rev.sql"
        done
        # If it is the head revision
        if [ "$HEAD_REV" == "$rev" ]; then
          # List migrations in HEAD
          head_migrations=()
          for file in "$DEST_DIR/$rev/extensions/$ext/migrate/"*.sql; do
            head_migrations+=($(basename "$file"))
          done
          HEAD_MIGRATIONS[$ext]="$(IFS=,; echo "${head_migrations[*]}")"
        else
          # Otherwise, produce an upgrade script:
          # First, for every migration not present in this revision, add it to the upgrade
          IFS=, read -r -a head_migrations <<< "${HEAD_MIGRATIONS[$ext]}"
          rm -f "$DEST_DIR/$ext--$rev--$HEAD_REV.sql"
          for mig in ${head_migrations[@]}; do
            if [ ! -f "$DEST_DIR/$rev/extensions/$ext/migrate/$mig" ]; then
               $BUILD_DIR/misc/inja/inja "$DEST_DIR/$HEAD_REV/extensions/$ext/migrate/$mig" >> "$DEST_DIR/$ext--$rev--$HEAD_REV.sql"
               # Ensure a new line
               echo >> "$DEST_DIR/$ext--$rev--$HEAD_REV.sql"
            fi
          done
          # Now, we need to replace functions that have changed
          # For this, we'll:
          # * prepare a database
          db="$DEST_DIR/db_$ext-$rev-$HEAD_REV"
          rm -rf "$db"
          "$PG_BINDIR/initdb" -D "$db" --no-clean --no-sync --locale=C --encoding=UTF8 || exit 1
          sockdir=$(mktemp -d)
          export PGSHAREDIR="$DEST_DIR/$rev/build/pg-share"
          "$PG_BINDIR/pg_ctl" start -D "$db" -o "-c listen_addresses=''" -o "-k $sockdir" || exit 1
          "$PG_BINDIR/createdb" -h "$sockdir" "$ext" || exit 1
          # * install the extension in this revision, snapshot pg_proc, drop the extension
          # We copy all scripts because there are dependencies
          cp  "$DEST_DIR"/$rev/build/packaged/extension/*.sql "$PGSHAREDIR/extension"
          cat <<EOF | "$PG_BINDIR/psql" -h "$sockdir" $ext -v ON_ERROR_STOP=1
           create table procs0 as (select * from pg_proc);
           create extension $ext version '$rev' cascade;
           create table procs as (select pg_proc.*, pg_get_functiondef(pg_proc.oid) as src, pg_get_function_identity_arguments(pg_proc.oid) as identity_args from pg_proc left outer join procs0 on pg_proc.oid = procs0.oid
           inner join pg_language on pg_language.oid = pg_proc.prolang
           inner join pg_namespace on pg_namespace.oid = pg_proc.pronamespace and pg_namespace.nspname = '$ext'
           left outer join pg_aggregate on pg_aggregate.aggfnoid = pg_proc.oid
           where procs0.oid is null and aggfnoid is null and lanname not in ('c', 'internal'));
           drop extension $ext cascade;
EOF
          if [ "${PIPESTATUS[1]}" -ne 0 ]; then
              exit 1
          fi
          "$PG_BINDIR/pg_ctl" stop -D  "$db" -m smart
          export PGSHAREDIR="$DEST_DIR/$HEAD_REV/build/pg-share"
          # We copy all scripts because there are dependencies
          cp "$DEST_DIR"/$HEAD_REV/build/packaged/extension/*.sql "$PGSHAREDIR/extension"
          "$PG_BINDIR/pg_ctl" start -D "$db" -o "-c listen_addresses=''" -o "-k $sockdir" || exit 1
          # * install the extension from the head revision
          echo "create extension $ext version '$HEAD_REV' cascade;" | "$PG_BINDIR/psql" -h "$sockdir" -v ON_ERROR_STOP=1 $ext
          if [ "${PIPESTATUS[0]}" -ne 0 ]; then
              exit 1
          fi
          # get changed functions
          cat <<EOF | "$PG_BINDIR/psql" -h "$sockdir" $ext -v ON_ERROR_STOP=1 -t -A -F "," -X >> "$DEST_DIR/$ext--$rev--$HEAD_REV.sql" || exit 1
          -- Changed code
          select pg_get_functiondef(pg_proc.oid) || ';' from (select * from pg_proc where oid not in (select aggfnoid from pg_aggregate)) as pg_proc
          inner join pg_language on pg_language.oid = pg_proc.prolang and pg_language.lanname not in ('c', 'internal')
          inner join pg_namespace on pg_namespace.oid = pg_proc.pronamespace and pg_namespace.nspname = '$ext'
          inner join procs on
          ((procs.proname::text = pg_proc.proname::text) and procs.pronamespace = pg_proc.pronamespace and
            pg_get_function_identity_arguments(pg_proc.oid) = identity_args and pg_get_functiondef(pg_proc.oid) != procs.src);
EOF
          if [ "${PIPESTATUS[1]}" -ne 0 ]; then
              exit 1
          fi
          # test the upgrade
          cp "$DEST_DIR/$ext--$rev--$HEAD_REV.sql" "$DEST_DIR/$rev/build/pg-share/extension/$ext--$rev.control" "$DEST_DIR/$rev/build/pg-share/extension/$ext--$rev.sql" "$PGSHAREDIR/extension"
          cat <<EOF | "$PG_BINDIR/psql" -h "$sockdir" $ext -v ON_ERROR_STOP=1
          drop extension $ext;
          create extension $ext version '$rev' cascade;
          alter extension $ext update to '$HEAD_REV';
EOF
          if [ "${PIPESTATUS[1]}" -ne 0 ]; then
            exit 1
          fi
          # * shut down the database
          "$PG_BINDIR/pg_ctl" stop -D  "$db" -m smart
          # * remove it
          rm -rf "$db"
        fi
    done
}

# Check if we are using our special build of Postgres
_sharedir_test=$(PGSHAREDIR=test "$PG_CONFIG" --sharedir)
if [ "test" != "$_sharedir_test" ]; then
  echo "Using non-Omnigres build of Postgres, bailing"
  exit 1
fi

# Capture the revision of the HEAD
HEAD_REV=$(git rev-parse --short HEAD)
declare -A HEAD_MIGRATIONS

revision HEAD
revision $VER