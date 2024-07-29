#!/bin/sh
#
# Artisan dependency update script
#
# We prefer binary wheels, because compiling scipy from source does not necessarily
# result in a fully functional environment (some scipy tests failing). Some packages
# are installed from source, so that system libraries present in the runtime can
# be used.
#

if [ ! -e requirements.txt ]; then
    echo "Please copy the packaged Artisan's requirements.txt to the directory you're in." 1>&2
    exit 1
fi

if ! which pip-compile >/dev/null; then
    echo "Please pip install pip-compile" 1>&2
    exit 1
fi

if ! which req2flatpak >/dev/null; then
    echo "Please pip install req2flatpak" 1>&2
    exit 1
fi

if [ ! -e flatpak-pip-generator ]; then
    echo "Please download flatpak-pip-generator" 1>&2
    echo "  wget https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/pip/flatpak-pip-generator" 1>&2
    exit 1
fi

BASEAPP_ID=`cat org.artisan_scope.artisan.yml | sed 's/^base:\s*//p;d'`
BASEAPP_VER=`cat org.artisan_scope.artisan.yml | sed 's/^base-version:\s*//p;d' | sed "s/'//g"`

# Get Python version for req2flatpak
PYTHONVER=`flatpak run --command=python3 $BASEAPP_ID//$BASEAPP_VER --version | sed 's/^Python \([0-9]\+\)\.\([0-9]\+\).*$/\1\2/'`

if [ ! "$PYTHONVER" ]; then
    echo "Could not discover Python version, is the BaseApp installed?" 1>&2
    exit 1
fi

cat >requirements-filtered.txt <<EOF
# build-dependencies for matplotlib
meson-python
cppy
pybind11
EOF
cat requirements.txt | \
  grep -v '\(^PyQt\|^qt[0-9]\+-tools\|^pyinstaller\)' | \
  grep -v "\\(python_version\s*<\\|;\\s*sys_platform\\s*==\\s*'darwin'\\|;\\s*platform_system\\s*==\\s*'Windows'\\)" \
    >>requirements-filtered.txt
pip-compile -q -o requirements-filtered.frozen.txt requirements-filtered.txt

cat requirements-filtered.frozen.txt | grep -v '\(^pillow\|^matplotlib\)' >requirements-binary.frozen.txt
cat requirements-filtered.frozen.txt | grep    '\(^pillow\|^matplotlib\)' >requirements-source.frozen.txt
req2flatpak --requirements-file requirements-binary.frozen.txt --target-platforms $PYTHONVER-x86_64 $PYTHONVER-aarch64 >dep-python3-wheels.json

python3 flatpak-pip-generator --runtime "${BASEAPP_ID}//${BASEAPP_VER}" -r requirements-source.frozen.txt -o dep-python3-source-full

# remove source dependencies already present as binary
python3 <<EOF
import json
import re

wheeldata = json.load(open('dep-python3-wheels.json'))
installed = [d for d in wheeldata['build-commands'][0].split()[2:] if not d.startswith('-')]
installed_re = re.compile('^.*/(' + '|'.join(installed) + ')-\d.*')
sourcedata = json.load(open('dep-python3-source-full.json'))
for m in sourcedata['modules']:
  m['sources'] = [s for s in m['sources'] if not installed_re.match(s['url'])]
json.dump(sourcedata, open('dep-python3-source.json', 'w'), indent=4)
EOF

# let matplotlib use system libraries
sed -i 's/\("pip3 install .*matplotlib.*\)"$/\1 --config-settings=setup-args=\\"-Dsystem-freetype=true\\" --config-settings=setup-args=\\"-Dsystem-qhull=true\\""/' dep-python3-source.json

# cleanup (comment when debugging this file)
rm -f requirements-filtered.txt requirements-filtered.frozen.txt requirements-binary.frozen.txt requirements-source.frozen.txt dep-python3-source-full.json

