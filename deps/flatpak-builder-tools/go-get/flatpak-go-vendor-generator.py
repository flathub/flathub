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
import urllib.request
from html.parser import HTMLParser

import attr

log = logging.getLogger(__name__)

@attr.s
class GoModule:
    name = attr.ib()
    version = attr.ib()
    revision = attr.ib()

def parse_modules(fh):
    for line in (l.strip() for l in fh if l.strip()):
        log.debug("Read line: %s", line)
        if line.startswith("# "):
            splits = line.split(" ")
            name, line_version = splits[-2], splits[-1]
            if '-' in line_version:
                log.debug("Parsing version: %s", line_version)
                _version, date_revision = line_version.strip().split("-", 1)
                try:
                    log.debug("Splitting %s", date_revision)
                    date, revision = date_revision.split('-')
                except ValueError:
                    log.debug("no further split of %s", date_revision)
                    date = None
                    version = revision = line_version
                else:
                    version = _version

                log.debug("Parsed version into: %s %s %s", version, date, revision)
            else:
                revision = None
                version = line_version

            m = GoModule(name, version, revision)
            yield m

def get_go_redirect(html_data):
    class GoImportParser(HTMLParser):
        _repo = None

        def handle_starttag(self, tag, attrs):
            if self._repo is not None:
                return

            # Make a dict of the attribute name/values
            # since it's easier to work with and understand.
            _attrs = {}
            for attr, value in attrs:
                _attrs[attr] = value

            name_attr = _attrs.get('name')
            if name_attr != 'go-import':
                return
            content = _attrs.get('content')
            if content is not None:
                self._repo = content.split(' ')[-1]

        def get_repo(self):
            return self._repo

    parser = GoImportParser()
    parser.feed(html_data)
    return parser.get_repo()


def go_module_to_flatpak(m):
    if not m.name.startswith("github.com"):
        url = m.name
    else:
        splits = m.name.split('/')
        if len(splits) > 3:
            url = '/'.join(splits[:3])
        else:
            url = m.name
    url = "https://" + url

    print('Checking {}...'.format(url), file=sys.stderr)

    try:
        with urllib.request.urlopen(url + '?go-get=1') as response:
            page_contents = str(response.read())
    except urllib.request.URLError as e:
        print('Failed to check {}: {}'.format(url, e), file=sys.stderr)
        sys.exit(1)
    else:
        repo = get_go_redirect(page_contents)
        url_found = repo
        if url_found != url:
            print(' got {}'.format(url_found), file=sys.stderr)
        else:
            print(' done', file=sys.stderr)
        url = url_found

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
