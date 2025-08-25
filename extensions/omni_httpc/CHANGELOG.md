# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.9] - 2025-07-24

### Fixed

* Spuriously failing SSL requests [#921](https://github.com/omnigres/omnigres/pull/921)

## [0.1.8] - 2025-07-24

### Fixed

* Fix ALPN protocol mismatch [#920](https://github.com/omnigres/omnigres/pull/920)

## [0.1.7] - 2025-07-24

### Fixed

* Not loading certificates when CA bundle is loaded [#919](https://github.com/omnigres/omnigres/pull/919)

## [0.1.6] - 2025-07-24

### Fixed

* Not loading different certificates [#917](https://github.com/omnigres/omnigres/pull/917)

## [0.1.5] - 2025-02-10

### Added

* Response now provides status reason for completeness [#782](https://github.com/omnigres/omnigres/pull/782)

## [0.1.4] - 2024-12-23

### Changed

* Using a newer version of [h2o](h2o/h2o: H2O - the optimized HTTP/1, HTTP/2, HTTP/3
  server) [#719](https://github.com/omnigres/omnigres/pull/719)

## [0.1.3] - 2024-12-16

### Addded

* Support for custom CA certificates [#676](https://github.com/omnigres/omnigres/pull/676)
* Support for custom client certificates [#676](https://github.com/omnigres/omnigres/pull/676)
* An option to ignore self-signed certificates [#676](https://github.com/omnigres/omnigres/pull/676)

## [0.1.2] - 2024-09-27

### Added

* Support for Postgres 17 [#650](https://github.com/omnigres/omnigres/pull/650)

## [0.1.1] - 2024-06-12

# Fixed

* Fixed sporadic failures in tests [#579](https://github.com/omnigres/omnigres/pull/579)

## [0.1.0] - 2024-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_httpc

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/578]

[0.1.2]: [https://github.com/omnigres/omnigres/pull/650]

[0.1.3]: [https://github.com/omnigres/omnigres/pull/676]

[0.1.4]: [https://github.com/omnigres/omnigres/pull/719]

[0.1.5]: [https://github.com/omnigres/omnigres/pull/782]

[0.1.6]: [https://github.com/omnigres/omnigres/pull/917]

[0.1.7]: [https://github.com/omnigres/omnigres/pull/919]

[0.1.8]: [https://github.com/omnigres/omnigres/pull/920]

[0.1.9]: [https://github.com/omnigres/omnigres/pull/921]
