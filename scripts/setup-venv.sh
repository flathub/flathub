#!/bin/bash

# Author: Dylan Turner
# Description: Creates and installs all needed packages

python -m pip install virtualenv

python -m virtualenv .venv
chmod +x .venv/bin/activate
source .venv/bin/activate
pip3 install -U pip
pip3 install -r bgrm/requirements.txt --no-cache-dir
deactivate
