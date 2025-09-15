from dataclasses import dataclass, field

import pygit2
import semver

@dataclass
class Extension:
    name: str
    version: semver.Version | str
    commit: pygit2.Commit | None = None

    def __post_init__(self):
        self.version = semver.Version.parse(self.version)

    def __repr__(self):
        return f"{self.name}--{self.version}"
