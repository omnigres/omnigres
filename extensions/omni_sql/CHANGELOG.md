# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.5.2] - 2025-07-17

### Fixed

* Initial support for Postgres 19 [#883](https://github.com/omnigres/omnigres/pull/883)

## [0.5.1] - 2025-01-23

### Added

* Support for the upcoming Postgres 18 [#763](https://github.com/omnigres/omnigres/pull/763)

## [0.5.0] - 2025-01-20

### Added

* `raw_statements` now has a second boolean argument indicating whether the function should preserve transaction blocks in a single result row or split them into separate rows. The argument defaults to false in order perserve backwards compatibility. [#753](https://github.com/omnigres/omnigres/pull/753)

## [0.4.0] - 2025-01-14

### Added

* `is_replace_statement` function to detect whether a statement has an `OR REPLACE` clause [#747](https://github.com/omnigres/omnigres/pull/747)

## [0.3.7] - 2024-11-17

### Fixed

* SQL execution with typed `null`-valued parameters where different type
  expected [#717](https://github.com/omnigres/omnigres/pull/718)

## [0.3.6] - 2024-11-16

### Fixed

* SQL execution with typed `null`-valued parameters [#717](https://github.com/omnigres/omnigres/pull/717)

## [0.3.5] - 2024-11-13

### Fixed

* SQL execution with typeless `null`-valued parameters [#709](https://github.com/omnigres/omnigres/pull/709)

## [0.3.4] - 2024-11-09

### Fixed

* SQL execution with text parameters mapped to non-text types [#689](https://github.com/omnigres/omnigres/pull/689)

## [0.3.3] - 2024-09-27

### Added

* Support for Postgres 17 [#650](https://github.com/omnigres/omnigres/pull/650)

## [0.3.2] - 2024-08-20

## Fixed

* `execute` will now handle multiple statements correctly [#616](https://github.com/omnigres/omnigres/pull/616)

## Changed

* `is_returning_statement` supports multiple statements [#616](https://github.com/omnigres/omnigres/pull/616)

## [0.3.1] - 2024-08-05

### Fixed

* Ensure proper SPI disconnection [#601](https://github.com/omnigres/omnigres/pull/601)

### Changed

* Improved internal documentation [#601](https://github.com/omnigres/omnigres/pull/601)

## [0.3.0] - 2024-07-31

### Added

* Dynamic SQL execution functionality
  [#596](https://github.com/omnigres/omnigres/pull/596)

## [0.2.2] - 2024-06-19

### Fixed

* Query validation might not work in presence of a post_parse_analyze hook
  [#582](https://github.com/omnigres/omnigres/pull/582)

## [0.2.1] - 2024-04-14

### Fixed

* `raw_statements` parser would incorrectly calculate the length of a statement
  if it is preceeded by whitespace [#561](https://github.com/omnigres/omnigres/pull/561)

## [0.2.0] - 2024-04-10

### Added

* Raw statement parser that preserves statement syntax and
  location [#554](https://github.com/omnigres/omnigres/pull/554)
* Statement type function [#555](https://github.com/omnigres/omnigres/pull/555)

## [0.1.0] - 2024-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_sql

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/553]

[0.2.1]: [https://github.com/omnigres/omnigres/pull/561]

[0.2.2]: [https://github.com/omnigres/omnigres/pull/581]

[0.3.0]: [https://github.com/omnigres/omnigres/pull/596]

[0.3.1]: [https://github.com/omnigres/omnigres/pull/601]

[0.3.2]: [https://github.com/omnigres/omnigres/pull/615]

[0.3.3]: [https://github.com/omnigres/omnigres/pull/650]

[0.3.4]: [https://github.com/omnigres/omnigres/pull/688]

[0.3.5]: [https://github.com/omnigres/omnigres/pull/708]

[0.3.6]: [https://github.com/omnigres/omnigres/pull/717]

[0.3.7]: [https://github.com/omnigres/omnigres/pull/718]

[0.4.0]: [https://github.com/omnigres/omnigres/pull/747]

[0.5.0]: [https://github.com/omnigres/omnigres/pull/753]

[0.5.1]: [https://github.com/omnigres/omnigres/pull/763]

[0.5.2]: [https://github.com/omnigres/omnigres/pull/883]
