#!/usr/bin/env sh

CWD=$(pwd)
TAG=v3.0.16

set -xe

#--------------------------------------------------------------------------------------------------
#
#--------------------------------------------------------------------------------------------------

after_all () {
  cd "${CWD}"/
  rm -rf flatpak-builder-tools/ spellchecker/ standard-notes/
}

before_all () {
  cd "${CWD}"/
  rm -rf .flatpak-builder/ build/ flatpak-builder-tools/ spellchecker/ standard-notes/
  rm -f generated-sources*.json yarn*.lock
  git clone https://github.com/flatpak/flatpak-builder-tools.git ./flatpak-builder-tools/
  git clone https://github.com/mobitar/node-spellchecker.git ./spellchecker/
  git clone https://github.com/standardnotes/desktop.git ./standard-notes/
}

#--------------------------------------------------------------------------------------------------
#
#--------------------------------------------------------------------------------------------------

spellchecker () {
  cd "${CWD}"/spellchecker/
  cp "${CWD}"/flatpak-builder-tools/yarn/flatpak-yarn-generator.py ./
  sed --in-place "/jasmine-focused/d" package.json
  rm -f package-lock.json # ...because yarn complains about different packaging tools and what not
  yarn install
  yarn check --integrity
  yarn check --verify-tree
  python3 flatpak-yarn-generator.py yarn.lock -o "${CWD}"/generated-sources-spellchecker.json
  cp yarn.lock "${CWD}"/yarn.spellchecker.lock
  # ...and also probably remove by hand, "anything" that contains git+https
}

standard_notes () {
  cd "${CWD}"/standard-notes/
  cp "${CWD}"/flatpak-builder-tools/yarn/flatpak-yarn-generator.py ./
  git checkout "${TAG}"
  rm -f package-lock.json app/package-lock.json # ...because yarn complains about different packaging tools and what not

#  sed --in-place '/spellchecker/d' app/package.json
#  sed --in-place '/semver/a "spellchecker": "^3.5.3",' app/package.json

  yarn install
  yarn check --integrity
  yarn check --verify-tree
  python3 flatpak-yarn-generator.py yarn.lock -o "${CWD}"/generated-sources.json
  cp yarn.lock "${CWD}"/
  python3 flatpak-yarn-generator.py app/yarn.lock -o "${CWD}"/generated-sources-app.json
  cp app/yarn.lock "${CWD}"/yarn.app.lock
}

#--------------------------------------------------------------------------------------------------
#
#--------------------------------------------------------------------------------------------------

before_all
standard_notes
#spellchecker
after_all

unset PWD TAG
