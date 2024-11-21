# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.1] - 2024-11-20

### Fixed

* Avoid failure-prone transaction restart to attain proper isolation
  level [#691](https://github.com/omnigres/omnigres/pull/691)

## [0.4.0] - 2024-10-29

### Added

* Experimental serializable transaction linearization
  functionality [#666](https://github.com/omnigres/omnigres/pull/666)

### Fixed

* `retry` was not setting up predicate locks to be tracked [#668](https://github.com/omnigres/omnigres/pull/668)

### Removed

* Deprecate variable functionality has been removed [#673](https://github.com/omnigres/omnigres/pull/673)

## [0.3.0] - 2024-10-17

### Added

* `omni_txn.retry` now accepts an optional `params` argument [#663](https://github.com/omnigres/omnigres/pull/663)
* `omni_txn.retry` will now cache prepared statement plans [#664](https://github.com/omnigres/omnigres/pull/664)

### Fixed

* Retry backoff values collection parameter was not recognized [#662](https://github.com/omnigres/omnigres/pull/662)

## [0.2.1] - 2024-09-27

### Added

* Support for Postgres 17 [#650](https://github.com/omnigres/omnigres/pull/650)

## [0.2.0] - 2024-08-15

### Added

* Support for transaction retry logic [#606](https://github.com/omnigres/omnigres/pull/606)

## [0.1.0] - 2024-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_txn

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/606]

[0.2.1]: [https://github.com/omnigres/omnigres/pull/650]

[0.3.0]: [https://github.com/omnigres/omnigres/pull/661]

[0.4.0]: [https://github.com/omnigres/omnigres/pull/665]

[0.4.1]: [https://github.com/omnigres/omnigres/pull/691]
