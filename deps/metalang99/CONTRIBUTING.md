# Contributing

To introduce changes:

 1. fork this repository,
 2. create your own branch from `master`,
 3. make required changes,
 4. open a PR to `master` from your branch,
 5. wait until it gets reviewed.

To be able to work with low-level stuff such as the interpreter, I highly recommend to first observe the [Cloak Wiki].

To be able to work with the metalanguage itself, some basic familiarity with programming language theory is expected. For learning materials, see https://github.com/steshaw/plt.

[Cloak Wiki]: https://github.com/pfultz2/Cloak/wiki/C-Preprocessor-tricks,-tips,-and-idioms

Some useful scripts are:

| Description | Command |
|----------|----------|
| Format all the code base | `./scripts/fmt.sh` |
| Check code formatting | `./scripts/check-fmt.sh` |
| Test only `tests/` | `./scripts/test.sh` |
| Test only `examples/` | `./scripts/test-examples.sh` |
| Test both `tests/` and `examples/` | `./scripts/test-all.sh`  |
| Generate the documentation | `./scripts/docs.sh` |
| Open the documentation | `./scripts/open-docs.sh` |
| Generate the specification | `./scripts/spec.sh` |
| Open the specification | `./scripts/open-spec.sh` |
| Run the benchmarks | `./scripts/bench.sh` |

Happy hacking!

## Release procedure

 1. Update the `PROJECT_NUMBER` field in `Doxyfile`.
 2. Update the `release` field in `docs/conf.py`.
 3. Update `ML99_MAJOR`, `ML99_MINOR`, and `ML99_PATCH` in `include/metalang99.h`.
 4. Update the version number in `spec/spec.tex` & `spec/spec.pdf`.
 5. Update `CHANGELOG.md`.
 6. Release the project in [GitHub Releases].

[GitHub Releases]: https://github.com/Hirrolot/metalang99/releases
