#!/bin/sh

if ! [ -x "${HOME}/.albiononline/launcher/Albion-Online" ]; then
  mkdir -p "${HOME}/.albiononline"
  cp -r /app/extra/data/launcher "${HOME}/.albiononline/"
fi
cd "${HOME}/.albiononline/"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/app/extra/data/launcher"
exec "${HOME}/.albiononline/launcher/Albion-Online"
