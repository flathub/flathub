#!/bin/sh

export LV2_PATH=$HOME/.lv2:/app/extensions/Plugins/lv2:/app/lib/lv2

exec qtractor.bin $*
