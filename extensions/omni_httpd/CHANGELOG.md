# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - TBD

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
