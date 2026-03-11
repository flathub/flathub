#!/bin/bash
#
# Update to a new Grist version
#

# sqlite3 version we last saw, to check it hasn't changed yet
PREV_SQLITE3_VERSION="5.1.4-grist.8"

# Command-line arguments
DESKTOP_VERSION="$1"
CORE_VERSION="$2"

if [ ! "$DESKTOP_VERSION" -o ! "$CORE_VERSION"  ]; then
  echo "Usage: $0 <grist-desktop-version-tag> <grist-core-version-tag>" 2>&1
  exit 1
fi

# Script dependencies
if ! curl --version >/dev/null; then
  echo "Please install curl, see https://curl.se" 2>&1
  exit 1
fi
if ! flatpak-node-generator --help >/dev/null; then
  echo "Please install flatpak-node-generator, see https://github.com/flatpak/flatpak-builder-tools/tree/master/node" 2>&1
  exit 1
fi
if ! jq --version >/dev/null; then
  echo "Please install jq, see https://jqlang.org/" 2>&1
  exit 1
fi
if ! yq --version >/dev/null; then
  echo "Please install yq, see "https://kislyuk.github.io/yq/ 2>&1
  exit 1
fi

# Update
DESKTOP_ARCHIVE_URL="https://github.com/gristlabs/grist-desktop/archive/refs/tags/${DESKTOP_VERSION}.tar.gz"
CORE_ARCHIVE_URL="https://github.com/gristlabs/grist-core/archive/refs/tags/${CORE_VERSION}.tar.gz"

echo "** Downloading source archives" 2>&1
curl -L -s "${DESKTOP_ARCHIVE_URL}" >_grist-desktop.tar.gz
curl -L -s "${CORE_ARCHIVE_URL}" >_grist-core.tar.gz

echo "** Updating desktop yarn sources" 2>&1
tar -x -z -f _grist-desktop.tar.gz --wildcards 'grist-desktop-*/yarn.lock' -O >_grist-desktop-yarn.lock
flatpak-node-generator yarn -o generated-sources-desktop.json _grist-desktop-yarn.lock

echo "** Updating core yarn sources" 2>&1
tar -x -z -f _grist-core.tar.gz --wildcards 'grist-core-*/yarn.lock' -O >_grist-core-yarn.lock
flatpak-node-generator yarn -o generated-sources-core.json _grist-core-yarn.lock

echo "** Updating worker yarn sources (deno missing is ok)" 2>&1
mkdir _worker
tar -x -z -f _grist-core.tar.gz --strip-components 3 -C _worker --wildcards 'grist-core-*/sandbox/pyodide'
jq '.dependencies = []' <pyodide-worker-package.json >_worker/package.json
(cd _worker && ./setup.sh)
cp _worker/_build/worker/package.json pyodide-worker-package.json
cp _worker/_build/worker/yarn.lock pyodide-worker-yarn.lock
flatpak-node-generator yarn -o generated-sources-worker.json pyodide-worker-yarn.lock

echo "** Downloading pyodide compiled WASM wheels" 2>&1
echo "" >_pyodide-packages.jsonl
for f in `cat _worker/package_filenames.json | jq -r '.[]'`; do
  echo "- ${f}" 2>&1
  # manifest
  m=`echo "$f" | sed 's/-cp[0-9].*$/.json/; s/_/-/g'`
  url1="https://s3.amazonaws.com/grist-pynbox/pyodide/packages/v3/${m}"
  hash1=`curl -s "$url1" | openssl sha256 | sed 's/^.*= //'`
  echo '{ "type": "file", "url": "'"$url1"'", "sha256": "'"$hash1"'", "dest": "pyodide-packages" }' >>_pyodide-packages.jsonl
  # wheel
  url2="https://s3.amazonaws.com/grist-pynbox/pyodide/packages/v3/${f}"
  hash2=`curl -s "$url2" | openssl sha256 | sed 's/^.*= //'`
  echo '{ "type": "file", "url": "'"$url2"'", "sha256": "'"$hash2"'", "dest": "pyodide-packages" }' >>_pyodide-packages.jsonl
done
cat _pyodide-packages.jsonl | jq -s >pyodide-packages.json

# node-sqlite3 compiled modules (can drop this after source build works again the manifest is adapted)
echo "** Checking node-sqlite3"
NEW_SQLITE3_VERSION=`cat generated-sources-core.json | sed 's/^.*@gristlabs\/sqlite3\/-\/sqlite3-\([^#]\+\)\.tgz.*$/\1/p;d'`
if [ "${NEW_SQLITE3_VERSION}" != "${PREV_SQLITE3_VERSION}" ]; then
  echo ""
  echo "  IMPORTANT: update node-sqlite3 manually in com.getgrist.grist.yml" 2>&1
  echo "             and update PREV_SQLITE3_VERSION to ${NEW_SQLITE3_VERSION} in update.sh" 2>&1
  echo ""
fi

# update manifest
echo "** Updating manifest" 2>&1
DESKTOP_ARCHIVE_HASH=`openssl sha256 _grist-desktop.tar.gz | sed 's/^.*=\\s*//'`
yq -i '(.modules | filter(.name == "grist") | .[].sources | filter(.url | contains("gristlabs/grist-desktop")) | .[].url) |= "'"${DESKTOP_ARCHIVE_URL}"'"' com.getgrist.grist.yml
yq -i '(.modules | filter(.name == "grist") | .[].sources | filter(.url | contains("gristlabs/grist-desktop")) | .[].sha256) |= "'"${DESKTOP_ARCHIVE_HASH}"'"' com.getgrist.grist.yml
CORE_ARCHIVE_HASH=`openssl sha256 _grist-core.tar.gz | sed 's/^.*=\\s*//'`
yq -i '(.modules | filter(.name == "grist") | .[].sources | filter(.url | contains("gristlabs/grist-core")) | .[].url) |= "'"${CORE_ARCHIVE_URL}"'"' com.getgrist.grist.yml
yq -i '(.modules | filter(.name == "grist") | .[].sources | filter(.url | contains("gristlabs/grist-core")) | .[].sha256) |= "'"${CORE_ARCHIVE_HASH}"'"' com.getgrist.grist.yml

# cleanup intermediate files
echo "** Cleaning up" 2>&1
rm -Rf _worker
rm -f _grist-desktop.tar.gz _grist-core.tar.gz _grist-desktop-yarn.lock _grist-core-yarn.lock _pyodide-packages.jsonl
