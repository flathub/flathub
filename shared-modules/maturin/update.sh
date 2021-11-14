#!/bin/bash

_FPID=maturin

[ -f ${_FPID}.json ] || { echo "Can't find ${_FPID}.json"; exit 1; }

../../tools/cargo-updater $_FPID
