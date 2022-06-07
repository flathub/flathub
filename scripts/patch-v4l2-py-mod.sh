#!/bin/bash

# Author: Dylan Turner
# Description: Patches v4l2 python module to work with modern python

PY_VERS_STR=($(python --version))
PY_VERS_W_MIN=${PY_VERS_STR[1]}
PY_VERS=$(echo $PY_VERS_W_MIN | rev | cut -c3- | rev)
cp scripts/v4l2\ \(patch\).py .venv/lib/python$PY_VERS/site-packages/v4l2.py
