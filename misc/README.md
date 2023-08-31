- [IntelliJ SQL formatting rules](omnigres_sql.xml)

# Devenv.sh based local development environment

## Initial setup

Follow these guides:

1. https://devenv.sh/getting-started/
2. https://devenv.sh/automatic-shell-activation/
3. Run `direnv allow` in omnigres repo

## Day-to-day development

1. `cd` into the repo. This brings in all dependencies.
2. To bring up development stack (Postgres with all extensions, etc.), run:
   `devenv up`

Once the development environment is running, you can connect to it by issuing:

- `pg` -> this connects to Postgres through a UNIX socket, for maximum performance. CLI args forwarded.
- `pgclear` -> removes the PGDATA folder contents. You want to restart `devenv up` after this so Postgres can reinitialize as per `devenv.nix`.
