#!/bin/sh

# req2flatpak requires poetry >= 1.2
python3 -m pip install -U --user poetry
poetry export -C ../fwbackups --without-hashes --without-urls -o requirements.txt

pip install --user git+https://github.com/johannesjh/req2flatpak
req2flatpak --requirements-file requirements.txt --target-platforms 310-x86_64 310-aarch64 > python3-main.json
