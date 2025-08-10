# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.2] - 2025-08-10

### Fixed

* Return 'dir' as type when calling omni_vfs.file_info(dir) [#834][https://github.com/omnigres/omnigres/pull/834]

## [0.2.1] - 2025-02-12

### Fixed

* Writing file with `null` content [#789](https://github.com/omnigres/omnigres/pull/789)

## [0.2.0] - 2025-01-19

### Added

* Filesystem writing capabilities (`omni_vfs.write`) [#712](https://github.com/omnigres/omnigres/pull/712)
* Remote Filesystem (`remote_fs`) to access filesystems in other databases [#755](https://github.com/omnigres/omnigres/pull/755)

## [0.1.2] - 2024-09-27

### Added

* Support for Postgres 17 [#650](https://github.com/omnigres/omnigres/pull/650)

## [0.1.1] - 2024-04-14

### Fixed

* A potential crash when operating with deleted local FS mounts [#560](https://github.com/omnigres/omnigres/pull/560)

## [0.1.0] - 2024-03-05

Initial release following a few months of iterative development.

[Unreleased]: https://github.com/omnigres/omnigres/commits/next/omni_httpd

[0.1.0]: [https://github.com/omnigres/omnigres/pull/511]

[0.1.1]: [https://github.com/omnigres/omnigres/pull/559]

[0.1.2]: [https://github.com/omnigres/omnigres/pull/650]

[0.2.0]: [https://github.com/omnigres/omnigres/pull/710]

[0.2.1]: [https://github.com/omnigres/omnigres/pull/789]

[0.2.2]: [https://github.com/omnigres/omnigres/pull/820]
