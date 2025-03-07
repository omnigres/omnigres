# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.2] - 2025-03-07

### Fixed

* Support for upcoming Postgres 18 [#817](https://github.com/omnigres/omnigres/pull/817)

## [0.4.1] - 2025-03-04

### Fixed

* Fix build under Debian testing [#815](https://github.com/omnigres/omnigres/pull/815)

## [0.4.0] - 2025-02-28

### Fixed

* HTTP response should contain correct status code reasons [#781](https://github.com/omnigres/omnigres/pull/781)
* Rejected WebSocket requests are sent an HTTP response [#797](https://github.com/omnigres/omnigres/pull/797)
* File descriptor exhaustion prevention [#810](https://github.com/omnigres/omnigres/pull/810)

### Changed

* Core routing changed to a router mechanism with URLPattern support [#783](https://github.com/omnigres/omnigres/pull/783)
* Static file handler uses the new router mechanism [#795](https://github.com/omnigres/omnigres/pull/795)

## [0.3.1] - 2025-02-20

### Fixed

* Support for the upcoming Postgres 18 [#805](https://github.com/omnigres/omnigres/pull/805)

## [0.3.0] - 2025-02-04

### Added

* `Omnigres-Connecting-IP` header [#714](https://github.com/omnigres/omnigres/pull/714)

### Changed

* omni_httpd.handler is no longer a function but a procedure [#715](https://github.com/omnigres/omnigres/pull/715)

### Fixed

* Response body is no longer sent before committing the transaction [#778](https://github.com/omnigres/omnigres/pull/778)

## [0.2.10] - 2025-01-23

### Added

* Support for the upcoming Postgres 18 [#763](https://github.com/omnigres/omnigres/pull/763)

## [0.2.9] - 2025-01-17

### Fixed

* Exception handling when stopping shortly after starting [#752](https://github.com/omnigres/omnigres/pull/752)

## [0.2.8] - 2025-01-16

### Fixed

* Invalid behavior and crashes when stopped and dropped shortly after starting [#751](https://github.com/omnigres/omnigres/pull/751)

## [0.2.7] - 2025-01-13

### Fixed

* Stopped the server restarts again with a new connection [#744](https://github.com/omnigres/omnigres/pull/744)

## [0.2.6] - 2025-01-11

### Changed

* Don't start omni_httpd in a template database [#743](https://github.com/omnigres/omnigres/pull/743)

## [0.2.5] - 2024-12-23

### Changed

* Using a newer version of [h2o](h2o/h2o: H2O - the optimized HTTP/1, HTTP/2, HTTP/3
  server) [#719](https://github.com/omnigres/omnigres/pull/719)

## [0.2.4] - 2024-11-30

### Added

* Explicit omni_httpd server controls [#699](https://github.com/omnigres/omnigres/pull/699)

## [0.2.3] - 2024-11-29

### Fixed

* Running omni_httpd in multiple databases [#698](https://github.com/omnigres/omnigres/pull/698)

## [0.2.2] - 2024-11-21

### Fixed

* Ensure the code compiles across wider configurations [#692](https://github.com/omnigres/omnigres/pull/692)

## [0.2.1] - 2024-10-04

### Fixed

* Handle HTTP worker termination gracefully [#656](https://github.com/omnigres/omnigres/pull/656)

## [0.2.0] - 2024-08-26

### Added

* Multi-transactional handlers [#609](https://github.com/omnigres/omnigres/pull/556)
* Experimental WebSocket support [#628](https://github.com/omnigres/omnigres/pull/628)

### Changed

* Handler queries are being deprecated in favor of functions [#609](https://github.com/omnigres/omnigres/pull/556)

### Fixed

* omni_httpd may crash under certain circumstances during early
  startup [#551](https://github.com/omnigres/omnigres/pull/551), [#556](https://github.com/omnigres/omnigres/pull/556)
* omni_httpd master and its workers loop infinitely when postgres `max_worker_processes` is less than `omni_httpd.http_workers`.
  [#587](https://github.com/omnigres/omnigres/pull/587)
* omni_httpd workers leaking memory on every request [#610](https://github.com/omnigres/omnigres/pull/610)
* omni_httpd underallocated memory when proxying, can lead to undefined
  behavior [#630](https://github.com/omnigres/omnigres/pull/630)

## [0.1.2] - 2024-04-07

### Fixed

* When multiple listeners are present, some listeners may have become
  unresponsive. [#545](https://github.com/omnigres/omnigres/pull/545)

## [0.1.1] - 2024-03-22

### Changed

* Minimum required Omni interface version is 0G (implemented in omni 0.1.1)

## [0.1.0] - 2024-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_httpd

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/522]

[0.1.2]: [https://github.com/omnigres/omnigres/pull/544]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/550]

[0.2.1]: [https://github.com/omnigres/omnigres/pull/657]

[0.2.2]: [https://github.com/omnigres/omnigres/pull/692]

[0.2.3]: [https://github.com/omnigres/omnigres/pull/698]

[0.2.4]: [https://github.com/omnigres/omnigres/pull/699]

[0.2.5]: [https://github.com/omnigres/omnigres/pull/719]

[0.2.6]: [https://github.com/omnigres/omnigres/pull/743]

[0.2.7]: [https://github.com/omnigres/omnigres/pull/744]

[0.2.8]: [https://github.com/omnigres/omnigres/pull/751]

[0.2.9]: [https://github.com/omnigres/omnigres/pull/752]

[0.2.10]: [https://github.com/omnigres/omnigres/pull/763]

[0.3.0]: [https://github.com/omnigres/omnigres/pull/713]

[0.3.1]: [https://github.com/omnigres/omnigres/pull/805]

[0.4.0]: [https://github.com/omnigres/omnigres/pull/780]

[0.4.1]: [https://github.com/omnigres/omnigres/pull/815]

[0.4.2]: [https://github.com/omnigres/omnigres/pull/817]
