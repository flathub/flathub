#!/bin/bash
ARG=${1//[\\]/}
flatpak run org.ryujinx.Ryujinx --fullscreen $ARG