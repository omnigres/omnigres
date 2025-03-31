# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.2] - 2025-02-25

### Fixed

* UUID timestamp and version extraction functions are strict now [#813](https://github.com/omnigres/omnigres/pull/813)

## [0.2.1] - 2025-01-23

### Added

* Support for unreleased Postgres versions [#763](https://github.com/omnigres/omnigres/pull/763)

## [0.2.0] - 2024-12-16

### Added

* Upcoming `uuidv7` support [#707](https://github.com/omnigres/omnigres/pull/707)

### Changed

* `omni_polyfill` is now namespaced in its own schema and requires search path
  setup [#707](https://github.com/omnigres/omnigres/pull/707)

## [0.1.0] - 2024-08-23

### Added

* `trim_array` [#635](https://github.com/omnigres/omnigres/pull/635)

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_polyfill

[0.1.0]: [https://github.com/omnigres/omnigres/pull/635]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/707]

[0.2.1]: [https://github.com/omnigres/omnigres/pull/763]
