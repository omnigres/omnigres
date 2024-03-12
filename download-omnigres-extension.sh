#!/usr/bin/env bash

set -e

if ! which curl > /dev/null; then
  echo "curl not found, install curl" && exit 1
fi

subcommand="$1"
ext_name="$2"
ext_version="$3"

OS="$(uname)"

if [ "$OS" = Linux ]; then
  OS_ARCH="ubuntu-x86-64"
elif [ "$OS" = Darwin ]; then
    OS_ARCH="macos-arm"
else
  echo "extensions not available for os: $OS" && exit 1
fi

regex="PostgreSQL[[:space:]]+([0-9]+)\.[0-9]+"

pg_config_version="$(pg_config --version)"

if [[ "$pg_config_version" =~ $regex ]]; then
    PG_MAJOR_VERSION="${BASH_REMATCH[1]}"
else
  echo "pg_config --version output doesn't match the regex: \"$regex\"" && exit 1
fi

platform_url="https://index.omnigres.com/$PG_MAJOR_VERSION/Release/$OS_ARCH"

tarball_path="/tmp/$ext_name--$ext_version.tar.gz"
untarred_dir_path="/tmp/$ext_name--$ext_version--untarred"

clean_up () {
    exit_code=$1

    if [ -f "$tarball_path" ]; then
      rm "$tarball_path"
    fi

    if [ -d "$untarred_dir_path" ]; then
      rm -rf "$untarred_dir_path"
    fi

    exit $exit_code
}

fail () {
  error_msg=$1
  echo "$error_msg

Usage: $0 <subcommand>

Subcommands:
  install <ext-name> <ext-version>
    download and copy the extension files(only if it doesn't exist) to postgres directories of the given omnigres extension version


Visit $platform_url/index.json to view available omnigres extension releases"

  clean_up 1
}

case "$subcommand" in

  install)
    if [ -z "$ext_name" ] || [ -z "$ext_version" ]; then
      fail "extension name and version should not be empty"
    fi

    if ! curl -sSL --fail-with-body "$platform_url/$ext_name/$ext_version?with-dependencies" -o "$tarball_path"; then
      fail "extension download error: $(cat $tarball_path)"
    fi

    rm -rf "$untarred_dir_path" && mkdir "$untarred_dir_path" && tar -C "$untarred_dir_path" -xzf "$tarball_path"

    control_and_sql_files_destination="$(pg_config --sharedir)/extension"

    # `cp -n` (avoid copy if file exists at destination) fails in macos with non-zero exit code if file already exists
    # avoid overwriting of files while copying because .so file of an extension may already be loaded in memory
    # so loop through list returned by glob and copy if file doesn't exist

    shopt -s nullglob # required for returning empty list in case of no matching entries for glob
    for file in "$untarred_dir_path"/*{.sql,.control}; do
      if [ ! -f "$control_and_sql_files_destination"/$(basename "$file") ]; then
        cp -v "$file" "$control_and_sql_files_destination"
      else
        echo "skipping copy of $(basename "$file"), already exists"
      fi
    done
    shopt -u nullglob # unset the above set option

    if [ -d "$untarred_dir_path/lib" ]; then
      so_files_destination="$(pg_config --pkglibdir)"
      shopt -s nullglob
      for file in "$untarred_dir_path"/lib/*.so; do
        if [ ! -f "$so_files_destination"/$(basename "$file") ]; then
          cp -v "$file"  "$so_files_destination"
        else
          echo "skipping copy of ./lib/$(basename "$file"), already exists"
        fi
      done
      shopt -u nullglob
    fi

    # not printing for omni_manifest to avoid confusion
    # as omni_manifest is reponsible for creating other extensions
    if [ "$ext_name" != "omni_manifest" ]; then
      artifact_format=$(grep "^${ext_name}=" $untarred_dir_path/artifacts.txt)

      echo -e "\nThe omni_manifest query to create the extension:
      select *
      from
          omni_manifest.install('$artifact_format'::text::omni_manifest.artifact[])
      "
    fi

    clean_up 0
    ;;
  *)
    fail "unknown subcommand: '$subcommand'"
    ;;
esac