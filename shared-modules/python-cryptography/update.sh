#!/bin/bash

_FPID=python-cryptography

[ -f ${_FPID}.json ] || { echo "Can't find ${_FPID}.json"; exit 1; }

../../tools/cargo-updater $_FPID src/rust/Cargo.lock
../../tools/pip-updater
