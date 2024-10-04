import os
import shutil
import sys
from pathlib import Path
import tarfile

import requests
import check_extension_artifacts as artifact_checker


def package_extension(artifacts, ext_name, ext_version):
    found = False
    for line in artifacts.splitlines():
        if line.startswith(f"{ext_name}={ext_version}"):
            found = True
            ext = line.split("#")[0]
            [ext_name, ext_version] = ext.split("=")

            if len(line.split("#")) == 2:
                dependencies = line.split("#")[1].split(",")
                for dependency in dependencies:
                    [dep_name, dep_version] = dependency.split("=")
                    # only package omnigres dependencies
                    if dep_name.startswith(artifact_checker.OMNI_EXTENSION_PREFIX):
                        # copy/download dependency if not already present
                        if not os.path.isfile(
                            DESTINATION_DIR / f"{dep_name}--{dep_version}.sql"
                        ):
                            dep_destination_dir = DESTINATION_DIR / f"../{dep_name}"
                            print(dep_destination_dir)
                            # True if dependency is released in same commit
                            if os.path.isdir(dep_destination_dir):
                                shutil.copytree(
                                    dep_destination_dir,
                                    DESTINATION_DIR,
                                    dirs_exist_ok=True,
                                )
                            else:
                                with requests.get(
                                    f"https://{os.getenv('OMNIGRES_INDEX_DOMAIN')}/{os.getenv('MATRIX_COMBINATION')}"
                                    f"/{dep_name}/{dep_version}",
                                    allow_redirects=True,
                                ) as response:
                                    response.raise_for_status()
                                    dep_tar_gz_filepath = (
                                        f"{os.getenv('TMPDIR')}/{dep_name}.tar.gz"
                                    )
                                    with open(
                                        dep_tar_gz_filepath, mode="wb+"
                                    ) as dep_tar_gz_file:
                                        dep_tar_gz_file.write(response.content)

                                    with tarfile.open(
                                        dep_tar_gz_filepath, mode="r:gz"
                                    ) as tar:
                                        tar.extractall(DESTINATION_DIR)

    if not found:
        raise Exception(
            f"entry of extension '{ext_name}' versioned '{ext_version}' "
            f"not found in artifacts: {artifacts}"
        )


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise Exception(
            "Usage: python package_extension_with_dependencies.py <artifacts.txt> <ext-name> <ext-version>"
        )
    required_env_variables = [
        "OMNIGRES_INDEX_DOMAIN",
        "MATRIX_COMBINATION",
        "DESTINATION_DIR",
        "TMPDIR",
    ]
    for env_variable in required_env_variables:
        if os.getenv(env_variable) is None:
            raise Exception(f"{env_variable} is not set")

    DESTINATION_DIR = Path(os.getenv("DESTINATION_DIR"))
    DESTINATION_DIR.mkdir(parents=True, exist_ok=True)

    with open(sys.argv[1], "r") as artifacts_file:
        original_artifacts = artifacts_file.read()
        try:
            package_extension(original_artifacts, sys.argv[2], sys.argv[3])
        except requests.exceptions.HTTPError as e:
            if e.response.status_code == 404:
                print(
                    f"Can't find a build for {sys.argv[2]} {sys.argv[2]} ({os.getenv('MATRIX_COMBINATION')}), proceeding...")



    # the artifacts file may have been overwritten due to untarring of dependencies restore it
    with open(sys.argv[1], "w") as artifacts_file:
        artifacts_file.write(original_artifacts)
