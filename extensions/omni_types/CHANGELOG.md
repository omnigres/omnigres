# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.0] - 2024-02-13

### Added

* Support for comparing sum types [#793](https://github.com/omnigres/omnigres/pull/793])

## [0.2.1] - 2024-12-16

### Fixed

* Works with new omni_polyfill interface on Postgres 13 [#707](https://github.com/omnigres/omnigres/pull/707])

## [0.2.0] - 2024-09-12

### Added

* Function signature types [#634](https://github.com/omnigres/omnigres/pull/634])

### Fixed

* Sum types will be now included into `pg_dump`-produced dumps [#636](https://github.com/omnigres/omnigres/pull/636])

## [0.1.1] - 2024-08-19

### Fixed

* Variable-size types were not detoasted/unpacked correctly [#620](https://github.com/omnigres/omnigres/pull/620])

## [0.1.0] - 2024-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_httpd

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/620]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/620]

[0.2.1]: [https://github.com/omnigres/omnigres/pull/707]

[0.3.0]: [https://github.com/omnigres/omnigres/pull/793]
