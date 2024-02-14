#!/usr/bin/env bash
# Script that generates an SQL upgrade for an extension ext_name between ext_version and
# other versions of the same extension present in index.json file
# ```shell
#  PG_CONFIG=.pg/<OS>-Debug/<VERSION>/build/bin/pg_config ./generate-upgrades.sh <index.json file> <ext_name> <ext_version>
# ```

script_dir=$(dirname "$0")

BUILD_TYPE=${BUILD_TYPE:=RelWithDebInfo}
BUILD_DIR=${BUILD_DIR:=$script_dir/build}
DEST_DIR=${DEST_DIR:=${BUILD_DIR}/_migrations}
DEST_DIR=$(realpath $DEST_DIR)

PG_CONFIG=${PG_CONFIG:=pg_config}
PG_BINDIR=$($PG_CONFIG --bindir)

index_contents=$(cat $1)
ext_name=$2
ext_ver=$3


revision() {
    old_ver=$1
    new_ver=$2

    for ver in $old_ver $new_ver; do
      rev=$(echo $index_contents | jq -r ".extensions.$ext_name.\"$ver\"")
      if [ ! -f "${DEST_DIR}/$rev/.complete" ]; then
        rm -rf "${DEST_DIR}/$rev" # in case if there was a prior failure to complete
        echo -n "Cloning $rev... "
        git clone --shared ${script_dir} "${DEST_DIR}/$rev" 2>/dev/null
        git -C "${DEST_DIR}/$rev" checkout $rev
        echo "done."
        # Try to speed builds up by sharing CPM modules
        export CPM_SOURCE_CACHE=$DEST_DIR/.cpm_cache
        cmake -S "${DEST_DIR}/$rev" -B "${DEST_DIR}/$rev/build" -DPG_CONFIG=$PG_CONFIG -DCMAKE_BUILD_TYPE=$BUILD_TYPE
        cmake --build "${DEST_DIR}/$rev/build" --parallel || exit 1
        cmake --build "${DEST_DIR}/$rev/build" --parallel --target package_extensions || exit 1
        # We're done
        touch "${DEST_DIR}/$rev/.complete"
      fi
    done

    old_rev=$(echo $index_contents | jq -r ".extensions.$ext_name.\"$old_ver\"")
    new_rev=$(echo $index_contents | jq -r ".extensions.$ext_name.\"$new_ver\"")

    extpath=$(cat ${DEST_DIR}/$new_rev/build/paths.txt | grep "^$ext_name " | cut -d " " -f 2)
    # List migrations in new version
    new_migrations=()
    new_migrate_path="$DEST_DIR/$new_rev/$extpath/migrate"
    if [ -d "$new_migrate_path/$ext_name" ]; then
       # Extension that shares a folder with another extension
       new_migrate_path="$new_migrate_path/$ext_name"
    fi
    for file in "$new_migrate_path/"*.sql; do
      new_migrations+=($(basename "$file"))
    done

    extpath=$(cat ${DEST_DIR}/$old_rev/build/paths.txt | grep "^$ext_name " | cut -d " " -f 2)
    # List migrations in old version
    old_migrations=()
    old_migrate_path="$DEST_DIR/$old_rev/$extpath/migrate"
    if [ -d "$old_migrate_path/$ext_name" ]; then
       # Extension that shares a folder with another extension
       old_migrate_path="$old_migrate_path/$ext_name"
    fi
    for file in "$old_migrate_path/"*.sql; do
      old_migrations+=($(basename "$file"))
    done

    for mig in ${new_migrations[@]}; do
      if [ ! -f "$old_migrate_path/$mig" ]; then
         $BUILD_DIR/misc/inja/inja "$new_migrate_path/$mig" >> "$DEST_DIR/$ext_name--$old_ver--$new_ver.sql"
         # Ensure a new line
         echo >> "$DEST_DIR/$ext_name--$old_ver--$new_ver.sql"
      fi
    done

    # Now, we need to replace functions that have changed
    # For this, we'll:
    # * prepare a database
    db="$DEST_DIR/db_$ext_name-$old_ver-$new_ver"
    rm -rf "$db"
    "$PG_BINDIR/initdb" -D "$db" --no-clean --no-sync --locale=C --encoding=UTF8 || exit 1
    sockdir=$(mktemp -d)
    export PGSHAREDIR="$DEST_DIR/$old_rev/build/pg-share"
    "$PG_BINDIR/pg_ctl" start -D "$db" -o "-c max_worker_processes=64" -o "-c listen_addresses=''" -o "-k $sockdir" -o "-c shared_preload_libraries='$DEST_DIR/$old_rev/build/extensions/omni_ext/omni_ext.so'" || exit 1
    "$PG_BINDIR/createdb" -h "$sockdir" "$ext_name" || exit 1
    # * install the extension in this revision, snapshot pg_proc, drop the extension
    # We copy all scripts because there are dependencies
    cp  "$DEST_DIR"/$old_rev/build/packaged/extension/*.sql "$PGSHAREDIR/extension"
    cat <<EOF | "$PG_BINDIR/psql" -h "$sockdir" $ext_name -v ON_ERROR_STOP=1
     create table procs0 as (select * from pg_proc);
     create extension $ext_name version '$old_ver' cascade;
     create table procs as (select pg_proc.*, pg_get_functiondef(pg_proc.oid) as src, pg_get_function_identity_arguments(pg_proc.oid) as identity_args from pg_proc left outer join procs0 on pg_proc.oid = procs0.oid
     inner join pg_language on pg_language.oid = pg_proc.prolang
     inner join pg_namespace on pg_namespace.oid = pg_proc.pronamespace and pg_namespace.nspname = '$ext_name'
     left outer join pg_aggregate on pg_aggregate.aggfnoid = pg_proc.oid
     where procs0.oid is null and aggfnoid is null and lanname not in ('c', 'internal'));
     drop extension $ext_name cascade;
EOF
    if [ "${PIPESTATUS[1]}" -ne 0 ]; then
        exit 1
    fi
    "$PG_BINDIR/pg_ctl" stop -D  "$db" -m smart
    export PGSHAREDIR="$DEST_DIR/$new_rev/build/pg-share"
    # We copy all scripts because there are dependencies
    cp "$DEST_DIR"/$new_rev/build/packaged/extension/*.sql "$PGSHAREDIR/extension"
    "$PG_BINDIR/pg_ctl" start -D "$db" -o "-c max_worker_processes=64" -o "-c listen_addresses=''" -o "-k $sockdir"  -o "-c shared_preload_libraries='$DEST_DIR/$new_rev/build/extensions/omni_ext/omni_ext.so'" || exit 1
    # * install the extension from the head revision
    echo "create extension $ext_name version '$new_ver' cascade;" | "$PG_BINDIR/psql" -h "$sockdir" -v ON_ERROR_STOP=1 $ext_name
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then
        exit 1
    fi
    # get changed functions
    cat <<EOF | "$PG_BINDIR/psql" -h "$sockdir" $ext_name -v ON_ERROR_STOP=1 -t -A -F "," -X >> "$DEST_DIR/$ext_name--$old_ver--$new_ver.sql" || exit 1
    -- Changed code
    select pg_get_functiondef(pg_proc.oid) || ';' from (select * from pg_proc where oid not in (select aggfnoid from pg_aggregate)) as pg_proc
    inner join pg_language on pg_language.oid = pg_proc.prolang and pg_language.lanname not in ('c', 'internal')
    inner join pg_namespace on pg_namespace.oid = pg_proc.pronamespace and pg_namespace.nspname = '$ext_name'
    inner join procs on
    ((procs.proname::text = pg_proc.proname::text) and procs.pronamespace = pg_proc.pronamespace and
      pg_get_function_identity_arguments(pg_proc.oid) = identity_args and pg_get_functiondef(pg_proc.oid) != procs.src);
EOF
    if [ "${PIPESTATUS[1]}" -ne 0 ]; then
        exit 1
    fi
    # test the upgrade
    cp "$DEST_DIR/$ext_name--$old_ver--$new_ver.sql" "$DEST_DIR/$old_rev/build/pg-share/extension/$ext_name--$old_ver.control" "$DEST_DIR/$old_rev/build/pg-share/extension/$ext_name--$old_ver.sql" "$PGSHAREDIR/extension"
    cat <<EOF | "$PG_BINDIR/psql" -h "$sockdir" $ext_name -v ON_ERROR_STOP=1
    drop extension $ext_name;
    create extension $ext_name version '$old_ver' cascade;
    alter extension $ext_name update to '$new_ver';
EOF
    if [ "${PIPESTATUS[1]}" -ne 0 ]; then
      exit 1
    fi
    # * shut down the database
    "$PG_BINDIR/pg_ctl" stop -D  "$db" -m smart
    # * remove it
    rm -rf "$db"
    # * copy upgrade script files
    cp -f "$DEST_DIR/$ext_name--$old_ver--$new_ver.sql" "$DEST_DIR/packaged"
}

# Check if we are using our special build of Postgres
_sharedir_test=$(PGSHAREDIR=test "$PG_CONFIG" --sharedir)
if [ "test" != "$_sharedir_test" ]; then
  echo "Using non-Omnigres build of Postgres, bailing"
  exit 1
fi

mkdir -p "$DEST_DIR/packaged"

while read -r ver; do
  if [ $ver = $ext_ver ]; then
    continue
  fi
  echo -e "$ver\n$ext_ver" | sort -CV
  if [ $? = 0 ]; then
    revision $ver $ext_ver
  else
    revision $ext_ver $ver
  fi
done < <(echo $index_contents | jq -r ".extensions.$ext_name | keys_unsorted[]" | sort -V)