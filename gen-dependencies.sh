#!/bin/bash
pip_generator=./flatpak-builder-tools/pip/flatpak-pip-generator
${pip_generator} -r kcc-requirements.txt --output kcc-pypi-dependencies
${pip_generator} -r pyqt5-requirements.txt --output pyqt5-pypi-dependencies
sed -i "s/install/install --no-build-isolation/g" pyqt5-pypi-dependencies.json
