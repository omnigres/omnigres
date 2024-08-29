# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.1] - 2024-08-29

### Fixed

* Identity type functions made strict to avoid crashes on NULLs [#645](https://github.com/omnigres/omnigres/pull/645)
* Identity type functions made parallel safe as their underlying
  implementations [#646](https://github.com/omnigres/omnigres/pull/646)

## [0.3.0] - 2024-08-28

### Added

* Support for UUID-based IDs [#625](https://github.com/omnigres/omnigres/pull/625)
* UUIDv7 generator function (`uuidv7`) [#626](https://github.com/omnigres/omnigres/pull/626)

## [0.2.0] - 2024-08-04

### Changed

* Expose operators to the `public` schema [#603](https://github.com/omnigres/omnigres/pull/603)

## [0.1.2] - 2024-08-04

### Fixed

* Support for namespaced identity types [#602](https://github.com/omnigres/omnigres/pull/602)

## [0.1.1] - 2024-08-02

### Added

* Identity type constructors [#599](https://github.com/omnigres/omnigres/pull/599)

## [0.1.0] - 2024-08-02

Initial release

[0.1.0]: [https://github.com/omnigres/omnigres/pull/597]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/599]

[0.1.2]: [https://github.com/omnigres/omnigres/pull/602]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/603]

[0.3.0]: [https://github.com/omnigres/omnigres/pull/624]

[0.3.1]: [https://github.com/omnigres/omnigres/pull/644]

