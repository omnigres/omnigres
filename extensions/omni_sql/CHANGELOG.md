# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

