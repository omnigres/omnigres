# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0] - TBD

## [0.3.0] - 2025-03-01

### Added

* Added schema metainformation and its diffing [#766](https://github.com/omnigres/omnigres/pull/766),[#769](https://github.com/omnigres/omnigres/pull/769),[#770](https://github.com/omnigres/omnigres/pull/770)
* Added schema revision capture and migration capabilities [#790](https://github.com/omnigres/omnigres/pull/790)

### Fixed

* `assemble_schema` no longer uses internal details of `omni_vfs.table_fs` [#767](https://github.com/omnigres/omnigres/pull/767)

## [0.2.3] - 2025-01-08

### Added

* Add error detail on requirements installation [#738](https://github.com/omnigres/omnigres/pull/738)

## [0.2.2] - 2025-01-05

### Fixed

* `assemble_schema` logs errors in notices now [#734](https://github.com/omnigres/omnigres/pull/734)

## [0.2.1] - 2024-12-29

### Fixed

* `assemble_schema` handles syntax errors now [#722](https://github.com/omnigres/omnigres/pull/722)

### Added

* `assemble_schema` will now emit progress notices [#724](https://github.com/omnigres/omnigres/pull/724)

## [0.2.0] - 2024-04-19

### Added

* Schema dependency resolution (`assemble_schema`) [#548](https://github.com/omnigres/omnigres/pull/548)

## [0.1.0] - 2024-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_schema

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/567]

[0.2.1]: [https://github.com/omnigres/omnigres/pull/721]

[0.2.2]: [https://github.com/omnigres/omnigres/pull/734]

[0.2.3]: [https://github.com/omnigres/omnigres/pull/738]

[0.3.0]: [https://github.com/omnigres/omnigres/pull/766]

[0.4.0]: [https://github.com/omnigres/omnigres/pull/766]
