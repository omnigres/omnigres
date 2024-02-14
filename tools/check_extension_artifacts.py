import re
import sys

OMNI_EXTENSION_PREFIX = "omni_"
OMNI_EXTENSION_UNRELEASED_VERSION = "unreleased"


def topological_sort(digraph):
    # digraph is a dictionary:
    #   key: a node
    # value: a set of adjacent neighboring nodes

    # construct a dictionary mapping nodes to their
    # indegrees
    indegrees = {node: 0 for node in digraph}
    for node in digraph:
        for neighbor in digraph[node]:
            if neighbor not in indegrees:
                raise Exception(
                    f"'{neighbor}' extension doesn't have it's own entry in artifacts file"
                )
            indegrees[neighbor] += 1

    # track nodes with no incoming edges
    nodes_with_no_incoming_edges = []
    for node in digraph:
        if indegrees[node] == 0:
            nodes_with_no_incoming_edges.append(node)

    # initially, no nodes in our ordering
    topological_ordering = []

    # as long as there are nodes with no incoming edges
    # that can be added to the ordering
    while len(nodes_with_no_incoming_edges) > 0:
        # add one of those nodes to the ordering
        node = nodes_with_no_incoming_edges.pop()
        topological_ordering.append(node)

        # decrement the indegree of that node's neighbors
        for neighbor in digraph[node]:
            indegrees[neighbor] -= 1
            if indegrees[neighbor] == 0:
                nodes_with_no_incoming_edges.append(neighbor)

    # we've run out of nodes with no incoming edges
    # did we add all the nodes or find a cycle?
    if len(topological_ordering) == len(digraph):
        return topological_ordering  # got them all
    else:
        raise Exception(
            "Extension dependency graph has a cycle! No topological ordering exists. "
            "Please check artifacts.txt in the build directory to find the cycle."
        )


def check_extension_artifacts(artifacts):
    """
    validates artifacts file contents and
    returns the extensions with release version
    """
    graph = {}
    extension_versions = {}
    version_pattern = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")

    for line in artifacts.splitlines():
        ext = line.split("#")[0]
        [ext_name, ext_version] = ext.split("=")
        if (
                version_pattern.match(ext_version) is None
                and ext_version != OMNI_EXTENSION_UNRELEASED_VERSION
        ):
            raise Exception(
                f"extension '{ext_name}' version '{ext_version}' is neither semver compliant "
                f"nor '{OMNI_EXTENSION_UNRELEASED_VERSION}'"
            )

        if not ext_name.startswith(OMNI_EXTENSION_PREFIX):
            raise Exception(
                f"non omnigres extension (doesn't have '{OMNI_EXTENSION_PREFIX}' prefix) "
                f"'{ext_name}' can only be in dependencies"
            )

        if ext_name in graph:
            raise Exception(
                f"extension '{ext_name}' is at the beginning of more than one line"
            )
        graph[ext_name] = []

        if (
                ext_name in extension_versions
                and extension_versions[ext_name] != ext_version
        ):
            raise Exception(
                f"version of '{ext_name}' extension should be same throughout the file"
            )
        else:
            extension_versions[ext_name] = ext_version

        if len(line.split("#")) == 2:
            dependencies = line.split("#")[1].split(",")
            for d in dependencies:
                [d_name, d_version] = d.split("=")

                if d_name.startswith(OMNI_EXTENSION_PREFIX):
                    if (
                            version_pattern.match(d_version) is None
                            and d_version != OMNI_EXTENSION_UNRELEASED_VERSION
                    ):
                        raise Exception(
                            f"'{d_name}' dependency version '{d_version}' is neither semver "
                            f"nor '{OMNI_EXTENSION_UNRELEASED_VERSION}'"
                        )

                    if (
                            version_pattern.match(ext_version) is not None
                            and d_version == OMNI_EXTENSION_UNRELEASED_VERSION
                    ):
                        raise Exception(
                            f"'{ext_name}' extension with release version '{ext_version}' "
                            f"can't have '{OMNI_EXTENSION_UNRELEASED_VERSION}' dependency '{d_name}'"
                        )

                else:
                    if d_version != "*":
                        raise Exception(
                            f"non omnigres dependency (doesn't have '{OMNI_EXTENSION_PREFIX}' prefix) "
                            f"'{d_name}' version should only be '*'"
                        )

                if (
                        d_name in extension_versions
                        and extension_versions[d_name] != d_version
                ):
                    raise Exception(
                        f"version of '{d_name}' extension should be same throughout the file"
                    )
                else:
                    extension_versions[d_name] = d_version

                if d_name in graph[ext_name]:
                    raise Exception(
                        f"{d_name} found more than once in {ext_name} dependencies"
                    )
                graph[ext_name].append(d_name)

    # non omnigres extensions like pgcrypto only show up in dependency
    # populate these in graph with no neighbors
    for e, v in extension_versions.items():
        if e not in graph and v == "*":
            graph[e] = []

    extensions_to_release = []
    for ext in topological_sort(graph):
        if (
                ext.startswith(OMNI_EXTENSION_PREFIX)
                and version_pattern.match(extension_versions[ext]) is not None
        ):
            extensions_to_release.append(f"{ext}={extension_versions[ext]}")

    return extensions_to_release


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise Exception(
            "Only artifacts file is expected as an argument\n"
            "Usage: python check_extension_artifacts.py <artifacts.txt>"
        )
    with open(sys.argv[1], "r") as artifacts_file:
        print(check_extension_artifacts(artifacts_file.read()))