import os
from io import StringIO
from typing import Optional, Iterable, Tuple

import pygit2

import rich

from .extensions import Extension


class ProjectRepository(pygit2.Repository):
    """
    An augmentation of pygit2.Repository to provide functionality that is specific to Omnigres
    repository.
    """

    def top_level_commits(self, branch: str = "master") -> Iterable[pygit2.Commit]:
        """
        Top-level commits in the branch: either individual commits or merge commits into the branch.

        :param branch: Branch name, "master" by default.
        :return: an iterable of pygit2.Commit
        """
        target = self.lookup_branch(branch).target
        for commit in self.walk(target):
            if len(commit.parent_ids) > 1:  # Merge commit
                first_parent = self.get(commit.parent_ids[0])
                if len(first_parent.parent_ids) <= 1:
                    yield commit
            elif len(commit.parent_ids) <= 1:  # Direct commit or initial commit
                yield commit

class PublishedFile:
    def __init__(self, project_path: str, path: str, branch: str = "master"):
        self.path = path
        self.project_path = project_path
        self.repo = ProjectRepository(self.project_path)
        self.branch = branch

    def __iter__(self) -> Iterable[Tuple[pygit2.Commit, bytes]]:
        for commit in self.repo.top_level_commits(self.branch):
            if self.path in commit.tree:
                yield (commit, self.repo.get(commit.tree[self.path].id).data)

class Project:
    path: str

    def __init__(self, path: Optional[str] = None):
        self.path = path or os.path.join(os.path.dirname(__file__), "..")

    def extensions(self, include_current_version: bool = False) -> Iterable[Extension]:
        if include_current_version:
            with open(os.path.join(self.path, "versions.txt"), "r") as f:
                for line in f:
                    line = line.strip()
                    (ext, ver) = line.split("=")
                    yield Extension(ext, ver)
        for (commit, file) in PublishedFile(self.path, path="versions.txt"):
            for line in StringIO(file.decode("utf-8")):
                line = line.strip()
                (ext, ver) = line.split("=")
                yield Extension(ext, ver, commit)
