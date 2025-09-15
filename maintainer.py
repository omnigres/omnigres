#! /usr/bin/env uv run
from collections import defaultdict

import maintainer
import rich
import click

import semver

from itertools import groupby
from more_itertools import unique
from rich.console import Console


@click.group()
def main():
    pass


def do_lint_versions():
    project = maintainer.Project()

    def version_gaps():
        """
        Check if any extension has a version gap
        :return:
        """
        for (name, versions) in groupby(sorted(project.extensions(), key=lambda e: e.name),
                                        key=lambda e: e.name):
            sorted_by_versions = sorted(unique(versions, key=lambda e: e.version), key=lambda e: e.version)
            versions = [ext.version for ext in sorted_by_versions]

            major_gaps = []
            minor_gaps = []

            # Group by major.minor
            major_minor_pairs = set((v.major, v.minor) for v in versions)

            # Check each major version's minors
            by_major = defaultdict(list)
            for major, minor in major_minor_pairs:
                by_major[major].append(minor)

            success = True
            for major, minors in by_major.items():
                minors = sorted(minors)
                for i in range(len(minors) - 1):
                    if minors[i + 1] - minors[i] > 1:
                        missing = list(range(minors[i] + 1, minors[i + 1]))
                        minor_gaps.append((major, missing))

            # Check major version gaps
            majors = sorted(by_major.keys())
            for i in range(len(majors) - 1):
                if majors[i + 1] - majors[i] > 1:
                    missing = list(range(majors[i] + 1, majors[i + 1]))
                    major_gaps.append(missing)

            if len(major_gaps) > 0:
                rich.print(f":x: Major gaps in {name}:")
                rich.print(major_gaps)
                success = False

            if len(minor_gaps) > 0:
                rich.print(f":x: Minor gaps in {name}:")
                rich.print(major_gaps)
                success = False

        if success:
            rich.print(f":white_check_mark: No version gaps found")

        return success

    success = version_gaps()
    if not success:
        return 1


@click.group()
def lint():
    pass


@click.command("versions")
def lint_versions():
    do_lint_versions()


lint.add_command(lint_versions)

main.add_command(lint)

if __name__ == "__main__":
    main()
