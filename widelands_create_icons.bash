#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'create icons';
icon="${app_name}.svg";
for s in {16,22,24,32,48,64,72,96,128,192,256,512}; do
  size="${s}x${s}";
  echo "- ${size}";
  mkdir -p "icons/${size}/";
  rsvg-convert "${icon}" -w "${s}" -h "${s}" -a -f png -o "icons/${size}/${app_name}.png";
done;
echo;

