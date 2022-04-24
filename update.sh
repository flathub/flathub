#!/bin/bash

_FPID=org.qutebrowser.qutebrowser.Userscripts

[ -f ${_FPID}.yaml ] || { echo "Can't find ${_FPID}.yaml"; exit 1; }

_TOOLSDIR=$PWD/tools

flatpak-external-data-checker --edit-only ${_FPID}.yaml

for _mod in python-stem/python-cryptography:src/rust/Cargo.lock; do
  (
    cd ${_mod%:*}
    case $_mod in
      *:*Cargo.lock)
        ${_TOOLSDIR}/cargo-updater $(basename ${_mod%:*}) ${_mod#*:}
        ;;
      *)
        ${_TOOLSDIR}/cargo-updater $(basename ${_mod%:*})
        ;;
    esac
  )
done

# python modules that have multiple dependencies and a requirements.txt file
for _mod in python-{beautifulsoup4,pocket-api,pykeepass,readability-lxml,stem/python-cryptography/python-setuptools-rust,tldextract}; do
  (
    cd $_mod
    ${_TOOLSDIR}/pip-updater $(basename $_mod)
  )
done
