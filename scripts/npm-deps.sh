#!/bin/bash

# SPDX-License-Identifier: MIT

export PYTHONPATH="$(pwd)/flatpak-builder-tools/node:$PYTHONPATH"

input_path=$1

#TODO: maybe do all this in a temporary directory?
if [[ $1 == http* ]]; then # is the input a URL?
	curl -o ./package-lock.json "${input_path}/package-lock.json"
	curl -o ./package.json "${input_path}/package.json"
	input_path=./package-lock.json
	sleep 2;
else
	input_path="${input_path}/package-lock.json"
fi

python3 -m flatpak_node_generator -o "$(pwd)/npm-sources.json" npm "$input_path"

if [[ $1 == http* ]]; then # was the input a URL?
	rm ./package-lock.json
	rm ./package.json
fi