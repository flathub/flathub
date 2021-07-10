#!/usr/bin/env python3
"""
This is a very pragmatic (i.e. simple) tool for using Go vendor
with flatpak.

To make use of the tool you need to produce a vendor/modules.txt
through `go mod vendor`.  One approach is to modify your manifest to
include

build-options:
    build-args:
        - --share=network

and run `go mod vendor` just before you start to build.

Once that is done, you should see a "vendors/modules.txt" which
you can point this tool at.

This tool has a few rough edges, such as special-casing a few things.
For example, it assumes that everything is git-clonable.
Except for certain URLs which are rewritten.

The real solution is https://github.com/golang/go/issues/35922
"""
import json
import logging
import sys

import attr

log = logging.getLogger(__name__)

@attr.s
class GoModule:
    name = attr.ib()
    version = attr.ib()
    revision = attr.ib()

def parse_modules(fh):
    for line in (l.strip() for l in fh if l.strip()):
        if line.startswith("#"):
            _, name, version = line.split(" ")
            if '-' in version:
                version, date, revision = version.strip().split("-")
            else:
                revision = None

            m = GoModule(name, version, revision)
            yield m

def go_module_to_flatpak(m):
    if not m.name.startswith("github.com"):
        url = m.name
    else:
        splits = m.name.split('/')
        if len(splits) > 3:
            url = '/'.join(splits[:3])
        else:
            url = m.name
    url = url.replace('golang.org/x/', 'go.googlesource.com/')
    url = "https://" + url

    if not '+' in m.version:
        tag = m.version
    else:
        splits = m.version.split('+')
        log.debug(f"Splitting version for {url}: {m.version} {splits}")
        tag = splits[0]

    rev = m.revision
    source = {
        "type": "git",
        "url": url,
        "tag": tag,
        "dest": "vendor/" + m.name,
    }
    if m.revision:
        del source["tag"]
        source["commit"] = m.revision

    return source

def main():
    modules_file = sys.argv[1]
    fh = open(modules_file)
    fp_modules = [go_module_to_flatpak(m) for m in parse_modules(fh)]
    print (json.dumps(fp_modules, indent=4))

if __name__ == "__main__":
    main()
