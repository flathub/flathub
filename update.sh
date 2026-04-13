#!/bin/bash
#
# Update to a new Grist version
#

# pyodide versions we are using (different from upstream, for packaging reasons)
PYODIDE_VERSION=0.29.3

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
  echo "Please install yq, see https://kislyuk.github.io/yq/" 2>&1
  exit 1
fi
if ! yarn --version >/dev/null; then
  echo "Please install yarn" 2>&1
  exit 1
fi
if ! flatpak_pip_generator -h >/dev/null; then
  echo "Please install flatpak_pip_generator, see https://pypi.org/project/flatpak-pip-generator/" 2>&1
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

echo "** Updating worker yarn sources" 2>&1
mkdir _worker
tar -x -z -f _grist-core.tar.gz --strip-components 3 -C _worker --wildcards 'grist-core-*/sandbox/pyodide'
jq '.dependencies = {}' <pyodide-worker-package.json >_worker/package.json
(cd _worker && ./setup.sh)
(cd _worker/_build/worker && cat package.json | jq ".dependencies.pyodide |= \"${PYODIDE_VERSION}\"" > package.json.new && mv package.json.new package.json)
yarn install --cwd _worker/_build/worker
cp _worker/_build/worker/package.json pyodide-worker-package.json
cp _worker/_build/worker/yarn.lock pyodide-worker-yarn.lock
flatpak-node-generator yarn -o generated-sources-worker.json pyodide-worker-yarn.lock

# update manifest
echo "** Updating manifest" 2>&1
DESKTOP_ARCHIVE_HASH=`openssl sha256 _grist-desktop.tar.gz | sed 's/^.*=\\s*//'`
yq -i '(.modules | filter(.name == "grist") | .[].sources | filter(.url | contains("gristlabs/grist-desktop")) | .[].url) |= "'"${DESKTOP_ARCHIVE_URL}"'"' com.getgrist.grist.yml
yq -i '(.modules | filter(.name == "grist") | .[].sources | filter(.url | contains("gristlabs/grist-desktop")) | .[].sha256) |= "'"${DESKTOP_ARCHIVE_HASH}"'"' com.getgrist.grist.yml
CORE_ARCHIVE_HASH=`openssl sha256 _grist-core.tar.gz | sed 's/^.*=\\s*//'`
yq -i '(.modules | filter(.name == "grist") | .[].sources | filter(.url | contains("gristlabs/grist-core")) | .[].url) |= "'"${CORE_ARCHIVE_URL}"'"' com.getgrist.grist.yml
yq -i '(.modules | filter(.name == "grist") | .[].sources | filter(.url | contains("gristlabs/grist-core")) | .[].sha256) |= "'"${CORE_ARCHIVE_HASH}"'"' com.getgrist.grist.yml

# pyodide packages build step
# note dependencies are pip-compiled, so have their version specified with ==
echo "** Resolving pyodide package dependencies" 2>&1
tar -x -z -f _grist-core.tar.gz --wildcards 'grist-core-*/sandbox/requirements.txt' -O >_requirements.txt
for spec in `cat _requirements.txt | sed 's/\\s*#.*$//'`; do
  package=`echo "$spec" | sed 's/\\s*==.*$//'`
  version=`echo "$spec" | sed 's/^.*==\\s*//'`
  curl -s "https://pypi.org/pypi/${package}/json" | \
    jq ".releases[\"${version}\"] | map(select(.packagetype == \"sdist\"))[0] | { type: \"archive\", url: .url, sha256: .digests.sha256, dest: \"pydists/${package}\" }"
done | jq -s >generated-sources-pyodide-packages.json
flatpak_pip_generator packaging setuptools_scm flit_core -o generated-sources-pyodide-packages-build-deps

# cleanup intermediate files
echo "** Cleaning up" 2>&1
rm -Rf _worker
rm -f _grist-desktop.tar.gz _grist-core.tar.gz _grist-desktop-yarn.lock _grist-core-yarn.lock _pyodide-packages.jsonl _requirements.txt
