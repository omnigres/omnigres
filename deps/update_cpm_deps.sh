#! /usr/bin/env bash

set -e

while IFS= read -r line
do
  # Skip empty lines
  if [[ -z "$line" ]]; then
    continue
  fi

  name="${line%%=*}"
  value="${line#*=}"

  repo="${value%%@*}"
  ver="${value#*@}"

  rm -rf "${name}"
  git clone "${repo}" "${name}"
  git -C "${name}" checkout ${ver}
  rm -rf "${name}/.git"
  # -f is important as it will allow overriding erroneous .gitignore
  # files in these repos
  git add -f --all "${name}"

done < "dependencies.txt"