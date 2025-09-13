# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - TBD

### Added

* SQL autostart functionality [#932](https://github.com/omnigres/omnigres/pull/932)
* Timer handler for worker tasks [#943](https://github.com/omnigres/omnigres/pull/943)

### Fixed

* SQL execution role restriction [#945](https://github.com/omnigres/omnigres/pull/945)
* SQL handler should return after committing [#946](https://github.com/omnigres/omnigres/pull/946)

## [0.1.4] - 2025-09-04

### Fixed

* Potential build failure (missing std::optional) [#938](https://github.com/omnigres/omnigres/pull/938)

## [0.1.3] - 2025-08-24

### Fixed

* Make it compile on master Postgres [#915](https://github.com/omnigres/omnigres/pull/915)

## [0.1.2] - 2025-07-27

### Fixed

* Fix support for recent Clang compilers [#884](https://github.com/omnigres/omnigres/pull/884)

## [0.1.1] - 2025-05-15

### Fixed

* Update oink library to fix potential compilation error [#873](https://github.com/omnigres/omnigres/pull/873)

## [0.1.0] - 2025-04-24

Initial release

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_worker

[0.1.0]: [https://github.com/omnigres/omnigres/pull/856]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/873]

[0.1.2]: [https://github.com/omnigres/omnigres/pull/884]

[0.1.3]: [https://github.com/omnigres/omnigres/pull/915]

[0.1.4]: [https://github.com/omnigres/omnigres/pull/938]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/865]
