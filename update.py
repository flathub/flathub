#! /usr/bin/env python3
from datetime import datetime
from pathlib import Path

import requests
from lxml import etree


TARBALL_URL = "https://artifacts.plex.tv/plex-desktop-experimental/{version}/linux/Plex-{version}-linux-x86_64.tar.bz2"


def update_yaml(app_id, version):
  tmpl = Path(f"{app_id}.yml.in").read_text()
  url = TARBALL_URL.format(version=version)
  sha = requests.get(f"{url}.sha256").content.decode().strip()
  with open(f"{app_id}.yml", "w") as fp:
    tmpl = tmpl.replace("@FULL_VERSION@", version)
    tmpl = tmpl.replace("@TARBALL_SHA256@", sha)
    fp.write(tmpl)


def update_xml(app_id, version):
  xml_filename = f"{app_id}.metainfo.xml"
  parts = version.split(".")
  public_version = f"{parts[0]}.{parts[1]}.{parts[2]}"
  parser = etree.XMLParser(remove_comments=False)
  tree = etree.parse(xml_filename, parser=parser)
  release = etree.Element(
    "release",
    {"version": public_version, "date": datetime.today().strftime("%Y-%m-%d")},  # Thanks spotify <3
  )
  releases = tree.find("releases")
  release.tail = "\n    "
  releases.insert(0, release)
  tree.write(xml_filename, xml_declaration=True, encoding="utf-8")


if __name__ == "__main__":
  from argparse import ArgumentParser

  parser = ArgumentParser()
  parser.add_argument("app_id")
  parser.add_argument("version")
  args = parser.parse_args()

  update_yaml(args.app_id, args.version)
  update_xml(args.app_id, args.version)
