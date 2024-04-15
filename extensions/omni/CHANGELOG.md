# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.3] - TBD

### Added

* Planner hook support [#537](https://github.com/omnigres/omnigres/pull/537)

### Fixed

* In some cases, extension modules might be loaded more than once [#563](https://github.com/omnigres/omnigres/pull/563)

## [0.1.2] - 2023-03-23

### Fixed

* `alter extension ... update to` overzealous application of pg_proc rewriting rule has been
  fixed [#532](https://github.com/omnigres/omnigres/pull/532)
* Hook registration incorrectly adjusted indexing for context reference purposes, leading to undefined
  behavior [#532](https://github.com/omnigres/omnigres/pull/532)

## [0.1.1] - 2023-03-22

### Fixed

* `default_needs_fmgr` had an invalid signature [#519](https://github.com/omnigres/omnigres/pull/519)
* Mismatched extension upgrades may have resulted in a non-functional
  extension [6ea51b5](https://github.com/omnigres/omnigres/pull/522/commits/6ea51b5ef931d5a62af44234055223538ad3f721), [#529](https://github.com/omnigres/omnigres/pull/529),
  [81906791c](https://github.com/omnigres/omnigres/pull/522/commits/81906791cbae9eab07e2a3414720255b6bd2e4c2)
* Fixed a case when excessive or dynamic creation of backends may lead to  
  "too many dynamic shared memory segments" error [#528](https://github.com/omnigres/omnigres/pull/528)

## [0.1.0] - 2023-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/522]

[0.1.2]: [https://github.com/omnigres/omnigres/pull/531]
