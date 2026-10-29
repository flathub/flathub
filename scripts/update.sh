#!/usr/bin/env bash

set -e

VERSION="${1:?}"

curl -LOJ --clobber https://gitlab.com/ckardaris/klisi/-/releases/$VERSION/downloads/mvn_dependencies_project.yml
curl -LOJ --clobber https://gitlab.com/ckardaris/klisi/-/releases/$VERSION/downloads/mvn_dependencies_tools.yml
curl -LOJ --clobber https://gitlab.com/ckardaris/klisi/-/releases/$VERSION/downloads/com.ckardaris.Klisi.yml
