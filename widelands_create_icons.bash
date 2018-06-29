#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'create icons';
imagemagick_home='/app/opt/imagemagick';
imagemagick_lib="${imagemagick_home}/lib";
imagemagick_bin="${imagemagick_home}/bin";
if [[ -n "${LD_LIBRARY_PATH}" ]]; then
  LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${imagemagick_lib}";
else
  LD_LIBRARY_PATH="${imagemagick_lib}";
fi;
export LD_LIBRARY_PATH;
if [[ -n "${PATH}" ]]; then
  PATH="${PATH}:${imagemagick_bin}";
else
  PATH="${imagemagick_bin}";
fi;
export PATH;
icon="${app_name}.svg";
for s in {16,22,24,32,48,64,72,96,128,192,256,512}; do
  size="${s}x${s}";
  echo "- ${size}";
  mkdir -p "icons/${size}/";
  convert -background none -density 1024 -resize "${size}" "${icon}" "icons/${size}/${app_name}.png";
done;
echo;

