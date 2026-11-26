#!/bin/sh
# Prints the JVM options built into upstream's launcher.
#
# Before merging a version bump:   tools/launcher-options.sh | diff tools/launcher-options.txt -
# After porting a change to platform/adalet-eimza-tray.sh:
#                                  tools/launcher-options.sh > tools/launcher-options.txt
set -e
cd "$(dirname "$0")/.."

url=${1:-$(grep -o 'https://cdn\.uyap\.gov\.tr/[^ ]*\.deb' tr.gov.adalet.Eimza.yaml)}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

curl -sSfL -o "$tmp/pkg.deb" "$url"
(cd "$tmp" && ar x pkg.deb && tar -xf data.tar.* --wildcards '*/bin/adalet-eimza-tray')

# the version and class path change with every release
strings -n 5 "$tmp"/usr/lib/adalet-eimza-tray/bin/adalet-eimza-tray |
    grep -E '^(-D|-X|--add)' |
    grep -vE 'app\.version|java\.class\.path' |
    sort -u
