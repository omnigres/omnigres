# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0] - TBD

## [0.3.0] - 2025-09-03

### Added

* Support for `generateName` [#923](https://github.com/omnigres/omnigres/pull/923)
* Basic support for watches [#924](https://github.com/omnigres/omnigres/pull/924)
* Basic support for selectors [#925](https://github.com/omnigres/omnigres/pull/925)
* Resource
  tables [#927](https://github.com/omnigres/omnigres/pull/927), [#929](https://github.com/omnigres/omnigres/pull/929)

### Fixed

* Inserting into and updating resource views should return new resource
  tuples [#926](https://github.com/omnigres/omnigres/pull/926)

### Changed

* Refactored API to make queries more composable [#928](https://github.com/omnigres/omnigres/pull/928)

### Removed

* Per-statement API call caching [#929](https://github.com/omnigres/omnigres/pull/929)

## [0.2.0] - 2025-07-24

### Added

* Universal resource management framework [#914](https://github.com/omnigres/omnigres/pull/914)
* Credential loading [#916](https://github.com/omnigres/omnigres/pull/916)

### Removed

* Custom views for basic resources such as pods and nodes [#914](https://github.com/omnigres/omnigres/pull/914)

## [0.1.1] - 2024-12-30

### Fixed

* Works on Postgres 13 [#729](https://github.com/omnigres/omnigres/pull/729)

## [0.1.0] - 2024-11-30

Initial release

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_kube

[0.1.0]: [https://github.com/omnigres/omnigres/pull/676]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/729]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/913]

[0.3.0]: [https://github.com/omnigres/omnigres/pull/922]

[0.4.0]: [https://github.com/omnigres/omnigres/pull/948]
