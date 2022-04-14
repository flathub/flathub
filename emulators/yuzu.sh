#!/bin/bash
ARG=${1//[\\]/}
flatpak run org.yuzu_emu.yuzu -f -g "$ARG"