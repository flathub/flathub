# Creates a new version entry in the metainfo.xml for the latest version of the package
# Assumes there is already one manually aded <release> element in the manifest

# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "httpx",
#   "xmltodict",
# ]
# ///

import json
import sys
from datetime import datetime
from pathlib import Path
from typing import Tuple

import httpx
import xmltodict

try:
    package_name = sys.argv[1]
    metainfo_file_path = Path(sys.argv[2])
except IndexError:
    print("Expected args: <package name> <path to .metainfo.xml file>", file=sys.stderr)
    exit(1)


def get_latest_version(package_name: str) -> Tuple[str, datetime]:
    with httpx.Client() as client:
        data = json.loads(
            (client.get(f"https://pypi.org/pypi/{package_name}/json")).text
        )
        latest_version = data["info"]["version"]
        release_date = datetime.fromisoformat(
            data["releases"][latest_version][0]["upload_time"]
        )
        return latest_version, release_date


def read_file(path: Path) -> str:
    with open(path) as f:
        return f.read()


def write_file(path: Path, content: str) -> None:
    with open(path, "w") as f:
        f.write(content)


def ensure_list(element: dict[str, dict[str, str] | list[dict[str, str]]]) -> None:
    for k, v in element.items():
        if isinstance(v, dict):
            element[k] = [v]


# noinspection PyTypeChecker
def get_releases(xml: dict[str, str]) -> list[dict[str, str]]:
    releases = xml["component"]["releases"]
    ensure_list(releases)
    release_list = releases["release"]
    return release_list


def update_releases() -> None:
    latest_version, latest_version_time = get_latest_version(package_name)
    xml = xmltodict.parse(read_file(metainfo_file_path))
    releases: list[dict[str, str]] = get_releases(xml)

    if releases[0]["@version"] == latest_version:
        exit(2)

    releases.insert(
        0, {"@version": latest_version, "@date": str(latest_version_time.date())}
    )

    write_file(metainfo_file_path, xmltodict.unparse(xml, pretty=True))


if __name__ == "__main__":
    update_releases()
