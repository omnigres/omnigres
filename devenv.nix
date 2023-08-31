{ pkgs, lib, ... }:

{

  packages = with pkgs; [
    pkgsStatic.openssl
    pkgconfig
    cmake
    flex
    readline
    zlib
    clang-tools
    python3
  ];

  languages = {
    python.enable = true;
    c.enable = true;
    cplusplus.enable = true;
  };

  # use the superior process manager
  # process.implementation = "process-compose";

  # processes.omnigres = {
  #   exec = "echo TODO";
  #   process-compose = {
  #     depends_on.postgres.condition = "process_healthy";
  #     availability.restart = "on_failure";
  #   };
  # };

  # https://devenv.sh/reference/options/#servicespostgresenable
  services.postgres = {
    enable = true;
    package = pkgs.postgresql_15;
    # install all extensions sans timescaledb
    extensions =
      let
        # TODO: package omnigres extensions via Nix, import them here
        omnigres-extensions = [ ];

        filterBrokenAndNonHostPackages = hostPlatform: pkgs:
          lib.filterAttrs
            (name: pkg:
              !(pkg.meta ? broken && pkg.meta.broken) &&
              (pkg.meta ? platforms && lib.lists.elem hostPlatform pkg.meta.platforms)
            )
            pkgs;
      in
      extensions: (lib.attrValues
        (builtins.removeAttrs
          (filterBrokenAndNonHostPackages pkgs.system extensions)
          [ "tds_fdw" "timescaledb_toolkit" "timescaledb-apache" "timescaledb" ])) ++
      omnigres-extensions;
    initdbArgs = [
      "--locale=C"
      "--encoding=UTF8"
    ];

    initialDatabases = [
      { name = "omnigres"; }
    ];

    initialScript = ''
      create user omnigres with password 'omnigres';
      grant all privileges on database omnigres to omnigres;
      alter database omnigres owner to omnigres;
    '';

    listen_addresses = "127.0.0.1,localhost";

    port = 5432;
    # https://www.postgresql.org/docs/11/config-setting.html#CONFIG-SETTING-CONFIGURATION-FILE
    settings = {
      max_connections = 500;
      work_mem = "20MB";
      log_error_verbosity = "TERSE";
      log_min_messages = "NOTICE";
      log_min_error_statement = "WARNING";
      log_line_prefix = "%m [%p] %u@%d/%a";
      shared_preload_libraries = "pg_stat_statements";
      statement_timeout = "100s";
      deadlock_timeout = 3000;
      # maximize speed for local development
      fsync = "off";
      jit = "0";
      synchronous_commit = "off";
    };
  };

  scripts = {
    "pg".exec = "psql -U omnigres $@";
    "pg-clear".exec = "git clean -xf $PGDATA";
  };

  pre-commit.hooks = {
    prettier.enable = true;
    nixpkgs-fmt.enable = true;
    shellcheck.enable = true;
  };
}
