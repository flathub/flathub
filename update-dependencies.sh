#!/bin/bash

mkdir -p pydep.build

# Delete any old virtualenv to be sure te recreate a clean one
if [ -d pydep.build/__env__ ] ; then
    rm -rf pydep.build/__env__
fi

# Create and activate a virtualenv
python3 -m venv pydep.build/__env__
source pydep.build/__env__/bin/activate

# Install CalCleaner and export its dependencies
pip install calcleaner
pip freeze --all | grep -v pkg-resources > pydep.build/requirements.txt

# Get / Install flatpak-pip-generator
pip install requirements-parser
wget \
    -O pydep.build/flatpak-pip-generator.py \
    https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/pip/flatpak-pip-generator

# Generate the dependency file
python pydep.build/flatpak-pip-generator.py \
    --requirements-file=pydep.build/requirements.txt \
    --output=dependencies

# Extract the sources from the generated file
python3 << EOF

import json

with open("./dependencies.json", "r") as f:
    data = json.loads(f.read())

sources = {}

for mod in data["modules"]:
    for source in mod["sources"]:
        if "calcleaner" in source["url"]:
            continue  # Skip calcleaner itself
        sources[source["sha256"]] = source

with open("./dependencies.json", "w") as f:
    json.dump(list(sources.values()), f, indent=2)

EOF
