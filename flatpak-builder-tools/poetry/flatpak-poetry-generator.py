#!/usr/bin/env python3

__license__ = "MIT"

import argparse
import json
import re
import sys
import urllib.parse
import urllib.request
from collections import OrderedDict
from typing import Any

import toml


def get_pypi_source(
    name: str, version: str, hashes: list[str]
) -> tuple[str, str] | None:
    """Get the source information for a dependency.

    Args:
        name (str): The package name.
        version (str): The package version.
        hashes (list): The list of hashes for the package version.

    Returns (tuple): The url and sha256 hash.

    """

    matched = None
    url = "https://pypi.org/pypi/{}/json".format(name)
    print("Extracting download url and hash for {}, version {}".format(name, version))
    with urllib.request.urlopen(url) as response:
        body = json.loads(response.read().decode("utf-8"))
        releases = body.get("releases", {})

        if version not in releases:
            raise ValueError(f"Version {version} not found for package {name}")

        source_list = releases[version]
        for source in source_list:
            sha256 = source.get("digests", {}).get("sha256")
            if sha256 not in hashes:
                continue
            if (
                source.get("packagetype") == "bdist_wheel"
                and source.get("filename", "").endswith("-none-any.whl")
                and "py3" in source.get("python_version", "")
            ):
                matched = (source["url"], sha256)
                break
            if (
                not matched
                and source.get("packagetype") == "sdist"
                and "source" in source.get("python_version", "")
            ):
                matched = (source["url"], sha256)

        if not matched:
            print(
                f"\nW: Failed to find a suitable package to include for "
                f"{name} version {version} from {url}\nThis package is "
                "likely missing an 'any' wheel and 'sdist' on PyPi. Please "
                "edit the generated manifest to include it manually.\n"
            )

    return matched


def get_module_sources(
    parsed_lockfile: dict[str, Any], include_devel: bool = True
) -> list[dict[str, str]]:
    """Gets the list of sources from a toml parsed lockfile.

    Args:
        parsed_lockfile (dict): The dictionary of the parsed lockfile.
        include_devel (bool): Include dev dependencies, defaults to True.

    Returns (list): The sources.

    """
    sources = []
    hash_re = re.compile(r"(sha1|sha224|sha384|sha256|sha512|md5):([a-f0-9]+)")
    for section, packages in parsed_lockfile.items():
        if section == "package":
            for package in packages:
                category = package.get("category")
                groups = package.get("groups", [])
                optional = package.get("optional", False)
                if (
                    not category
                    or (category == "dev" and include_devel and not optional)
                    or (category == "main" and not optional)
                ):
                    hashes = []
                    # Check for old metadata format (poetry version < 1.0.0b2)
                    if "hashes" in parsed_lockfile["metadata"]:
                        hashes = parsed_lockfile["metadata"]["hashes"][package["name"]]
                    # metadata format 1.1
                    elif "files" in parsed_lockfile["metadata"]:
                        for package_name in parsed_lockfile["metadata"]["files"]:
                            if package_name == package["name"]:
                                package_files = parsed_lockfile["metadata"]["files"][
                                    package["name"]
                                ]
                                num_files = len(package_files)
                                for num in range(num_files):
                                    match = hash_re.search(package_files[num]["hash"])
                                    if match:
                                        hashes.append(match.group(2))
                    # metadata format 2.0
                    else:
                        if groups == ["dev"] and not include_devel:
                            continue
                        for file in package["files"]:
                            match = hash_re.search(file["hash"])
                            if match:
                                hashes.append(match.group(2))
                    package_source = package.get("source")
                    if package_source and package_source["type"] == "directory":
                        print(
                            f'Skipping download url and hash extraction for {package["name"]}, source type is directory'
                        )
                        continue
                    pypi_source_ret = get_pypi_source(
                        package["name"], package["version"], hashes
                    )
                    if pypi_source_ret:
                        url, hash = pypi_source_ret
                        source = {"type": "file", "url": url, "sha256": hash}
                        sources.append(source)
    return sources


def get_dep_names(
    parsed_lockfile: dict[str, Any], include_devel: bool = True
) -> list[str]:
    """Gets the list of dependency names.

    Args:
        parsed_lockfile (dict): The dictionary of the parsed lockfile.
        include_devel (bool): Include dev dependencies, defaults to True.

    Returns (list): The dependency names.

    """
    dep_names = []
    for section, packages in parsed_lockfile.items():
        if section == "package":
            for package in packages:
                category = package.get("category")
                groups = package.get("groups", [])
                optional = package.get("optional", False)
                if groups == ["dev"] and not include_devel:
                    continue
                if (
                    not category
                    or (category == "dev" and include_devel and not optional)
                    or (category == "main" and not optional)
                ):
                    dep_names.append(package["name"])
    return dep_names


def main() -> None:
    parser = argparse.ArgumentParser(description="Flatpak Poetry generator")
    parser.add_argument("lockfile", type=str)
    parser.add_argument(
        "-o", type=str, dest="outfile", default="generated-poetry-sources.json"
    )
    parser.add_argument("--production", action="store_true", default=False)
    args = parser.parse_args()

    include_devel = not args.production
    outfile = args.outfile
    lockfile = args.lockfile

    print('Scanning "%s" ' % lockfile, file=sys.stderr)

    with open(lockfile, "r", encoding="utf-8") as f:
        parsed_lockfile = toml.load(f)
        dep_names = get_dep_names(parsed_lockfile, include_devel=include_devel)
        pip_command = [
            "pip3",
            "install",
            "--no-index",
            '--find-links="file://${PWD}"',
            "--prefix=${FLATPAK_DEST}",
            " ".join(dep_names),
        ]
        main_module: dict[str, Any] = OrderedDict(
            [
                ("name", "poetry-deps"),
                ("buildsystem", "simple"),
                ("build-commands", [" ".join(pip_command)]),
            ]
        )
        sources = get_module_sources(parsed_lockfile, include_devel=include_devel)
        main_module["sources"] = sources

    print(" ... %d new entries" % len(sources), file=sys.stderr)

    print('Writing to "%s"' % outfile)
    with open(outfile, "w", encoding="utf-8") as f:
        f.write(json.dumps(main_module, indent=4))


if __name__ == "__main__":
    main()
