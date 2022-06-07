#!/bin/bash

# Author: Dylan Turner
# Description: Same as bgrm.sh, but  with flatpak file paths

export DEVICE_ARR=(`ls /sys/devices/virtual/video4linux | tr -d 'video'`)
echo "Using loopback device /dev/video${DEVICE_ARR[0]}"
source /app/.venv/bin/activate
python /app/bgrm \
    ${DEVICE_ARR[0]} $1 $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} \
    ${12} ${13} ${14} ${15} ${16} ${17} ${18} ${19} ${20} ${21} ${22} \
    ${23} ${24} ${25} ${26}
deactivate

