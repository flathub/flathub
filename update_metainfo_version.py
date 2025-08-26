#!/usr/bin/env python3
"""
Update metainfo.xml with a release version.

If the version doesn't exist, it becomes the only release.
If the version exists, the file is left unchanged.

Usage:
    python3 update_metainfo_version.py --version <VERSION> --metainfo <PATH> [--date <DATE>]

Arguments:
    --version:   The version to ensure exists (required)
    --metainfo:  Path to the metainfo.xml file (required)
    --date:      The release date in YYYY-MM-DD format (optional, defaults to today)
"""

import argparse
import sys
from datetime import datetime
from pathlib import Path
from xml.etree import ElementTree as ET


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Update metainfo.xml with a release version"
    )
    parser.add_argument(
        "--version",
        required=True,
        help="Version to ensure exists in metainfo.xml",
    )
    parser.add_argument(
        "--metainfo",
        required=True,
        type=Path,
        help="Path to metainfo.xml file",
    )
    parser.add_argument(
        "--date",
        default=datetime.now().strftime("%Y-%m-%d"),
        help="Release date in YYYY-MM-DD format (default: today)",
    )
    parser.add_argument(
        "--type",
        default="stable",
        help="Release type (default: stable)",
    )
    return parser.parse_args()


def version_exists(root, version):
    """Check if a release with the given version exists."""
    releases = root.find("releases")
    if releases is None:
        return False

    for release in releases.findall("release"):
        if release.get("version") == version:
            return True
    return False


def update_metainfo(metainfo_path, version, date, release_type):
    """
    Update metainfo.xml with the given version.

    If version doesn't exist, it becomes the only release.
    """
    try:
        tree = ET.parse(metainfo_path)
        root = tree.getroot()
    except ET.ParseError as e:
        print(f"Error parsing {metainfo_path}: {e}", file=sys.stderr)
        return False
    except FileNotFoundError:
        print(f"Error: {metainfo_path} not found", file=sys.stderr)
        return False

    if version_exists(root, version):
        print(f"Version {version} already exists in metainfo.xml")
        return True

    releases = root.find("releases")
    if releases is None:
        releases = ET.Element("releases")
        root.append(releases)

    for release in list(releases.findall("release")):
        releases.remove(release)

    new_release = ET.Element("release")
    new_release.set("date", date)
    new_release.set("type", release_type)
    new_release.set("version", version)
    releases.append(new_release)

    try:
        tree.write(
            metainfo_path,
            encoding="utf-8",
            xml_declaration=True,
        )
        print(f"Updated {metainfo_path}: version {version} is now the only release")
        return True
    except IOError as e:
        print(f"Error writing to {metainfo_path}: {e}", file=sys.stderr)
        return False


def main():
    args = parse_arguments()

    try:
        datetime.strptime(args.date, "%Y-%m-%d")
    except ValueError:
        print(
            f"Error: Invalid date format '{args.date}'. Expected YYYY-MM-DD",
            file=sys.stderr,
        )
        return 1

    if update_metainfo(args.metainfo, args.version, args.date, args.type):
        return 0
    else:
        return 1


if __name__ == "__main__":
    sys.exit(main())
