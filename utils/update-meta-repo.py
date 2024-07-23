# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "gitpython",
#   "ruamel.yaml",
# ]
# ///

import sys
from pathlib import Path

from git import Repo
from ruamel.yaml import YAML

try:
    repository_path = Path(sys.argv[1])
    manifest_file = Path(sys.argv[2])
except IndexError:
    print(
        "Missing file name/path: expected <repo path> <manifest file>", file=sys.stderr
    )
    exit(1)

repo = Repo(repository_path)
latest_hash = repo.rev_parse("HEAD").hexsha


def read_file(path: Path) -> str:
    with open(path, "r") as f:
        return f.read()


def update_commit_hash() -> None:
    yaml = YAML(typ="rt")
    manifest = yaml.load(read_file(manifest_file))

    def find_install_module(item: dict | object) -> bool:
        if not isinstance(item, dict):
            return False

        return "name" in item and item["name"] == "install"

    install_module = next(filter(find_install_module, manifest["modules"]))
    install_module["sources"][0]["commit"] = latest_hash

    with open(manifest_file, "w") as f:
        yaml.dump(manifest, f)
        # ruamel.yaml.dump(manifest)


if __name__ == "__main__":
    update_commit_hash()
