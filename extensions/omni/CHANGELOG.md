# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.13] - 2025-08-29

### Fixed

* Holding locks when calling user-supplifed functions [#933](https://github.com/omnigres/omnigres/pull/933)

## [0.2.12] - 2025-08-28

### Fixed

* Support for Postgres 19 [#931](https://github.com/omnigres/omnigres/pull/931)

## [0.2.11] - 2025-07-17

### Fixed

* Initial support for Postgres 19 [#883](https://github.com/omnigres/omnigres/pull/883)

## [0.2.10] - 2025-06-03

### Fixed

* Support for the upcoming Postgres 18 [#878](https://github.com/omnigres/omnigres/pull/878)

## [0.2.9] - 2025-04-28

### Fixed

* Ensure extensions are always loaded in live backends when installed in
  others [#867](https://github.com/omnigres/omnigres/pull/867)
* Enable extensions to operate in recovery mode if necessary [#870](https://github.com/omnigres/omnigres/pull/870)

## [0.2.8] - 2025-03-22

### Fixed

* Don't wait for background worker startup in recovery if they are scheduled for after recovery is finished [#837](https://github.com/omnigres/omnigres/pull/837)

## [0.2.7] - 2025-02-24

### Fixed

* Support for the upcoming Postgres 18 [#811](https://github.com/omnigres/omnigres/pull/811)

## [0.2.6] - 2025-02-20

### Fixed

* Support for the upcoming Postgres 18 [#804](https://github.com/omnigres/omnigres/pull/804)

## [0.2.5] - 2025-01-23

### Added

* Support for the upcoming Postgres 18 [#763](https://github.com/omnigres/omnigres/pull/763)

## [0.2.4] - 2024-12-06

### Fixed

* Occassional crash while handling `CALL`/`DO` statements [#696](https://github.com/omnigres/omnigres/pull/696)
* Unnecessary extension rescanning on all transaction rollbacks [#696](https://github.com/omnigres/omnigres/pull/696)

## [0.2.3] - 2024-11-30

### Changed

* Backend initialization transaction ends with a commit [#699](https://github.com/omnigres/omnigres/pull/699)

## [0.2.2] - 2024-11-29

### Changed

* All shmem allocations are to be zero-initialized [#698](https://github.com/omnigres/omnigres/pull/698)

## [0.2.1] - 2024-10-26

### Fixed

* Sporadic `unsupported byval length` error message upon module
  initialization [#677](https://github.com/omnigres/omnigres/pull/677)

## [0.2.0] - 2024-10-04

### Added

* Additional metadata information in `omni.modules`  [#574](https://github.com/omnigres/omnigres/pull/574)
* Warn if omni-enabled module has been built against a different major/minor version of Postgres.
  Differences between minor versions may present subtle
  incompatibilities. [#573](https://github.com/omnigres/omnigres/pull/573)
* Support for function-less native extensions [#586](https://github.com/omnigres/omnigres/pull/586)
* `omni_is_present()` to detect if omni has been preloaded [#605](https://github.com/omnigres/omnigres/pull/605)

## [0.1.5] - 2024-09-27

### Added

* Support for Postgres 17 [#650](https://github.com/omnigres/omnigres/pull/650)

## [0.1.4] - 2024-06-28

### Fixed

* Remove redundant (and conflicting) dshash symbols [#566](https://github.com/omnigres/omnigres/pull/566)

## [0.1.3] - 2024-04-16

### Added

* Planner hook support [#537](https://github.com/omnigres/omnigres/pull/537)

### Fixed

* In some cases, extension modules might be loaded more than once [#563](https://github.com/omnigres/omnigres/pull/563),
  or fail to unload [#564](https://github.com/omnigres/omnigres/pull/564),
  [#565](https://github.com/omnigres/omnigres/pull/565)

## [0.1.2] - 2024-03-23

### Fixed

* `alter extension ... update to` overzealous application of pg_proc rewriting rule has been
  fixed [#532](https://github.com/omnigres/omnigres/pull/532)
* Hook registration incorrectly adjusted indexing for context reference purposes, leading to undefined
  behavior [#532](https://github.com/omnigres/omnigres/pull/532)

## [0.1.1] - 2024-03-22

### Fixed

* `default_needs_fmgr` had an invalid signature [#519](https://github.com/omnigres/omnigres/pull/519)
* Mismatched extension upgrades may have resulted in a non-functional
  extension [6ea51b5](https://github.com/omnigres/omnigres/pull/522/commits/6ea51b5ef931d5a62af44234055223538ad3f721), [#529](https://github.com/omnigres/omnigres/pull/529),
  [81906791c](https://github.com/omnigres/omnigres/pull/522/commits/81906791cbae9eab07e2a3414720255b6bd2e4c2)
* Fixed a case when excessive or dynamic creation of backends may lead to  
  "too many dynamic shared memory segments" error [#528](https://github.com/omnigres/omnigres/pull/528)

## [0.1.0] - 2024-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/522]

[0.1.2]: [https://github.com/omnigres/omnigres/pull/531]

[0.1.3]: [https://github.com/omnigres/omnigres/pull/540]

[0.1.4]: [https://github.com/omnigres/omnigres/pull/566]

[0.1.5]: [https://github.com/omnigres/omnigres/pull/650]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/572]

[0.2.1]: [https://github.com/omnigres/omnigres/pull/677]

[0.2.2]: [https://github.com/omnigres/omnigres/pull/698]

[0.2.3]: [https://github.com/omnigres/omnigres/pull/699]

[0.2.4]: [https://github.com/omnigres/omnigres/pull/696]

[0.2.5]: [https://github.com/omnigres/omnigres/pull/763]

[0.2.6]: [https://github.com/omnigres/omnigres/pull/803]

[0.2.7]: [https://github.com/omnigres/omnigres/pull/811]

[0.2.8]: [https://github.com/omnigres/omnigres/pull/837]

[0.2.9]: [https://github.com/omnigres/omnigres/pull/866]

[0.2.10]: [https://github.com/omnigres/omnigres/pull/878]

[0.2.11]: [https://github.com/omnigres/omnigres/pull/883]

[0.2.12]: [https://github.com/omnigres/omnigres/pull/931]

[0.2.13]: [https://github.com/omnigres/omnigres/pull/933]
