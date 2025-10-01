#!/bin/bash

# Wrapper script to use host system's lsof command via flatpak-spawn

exec flatpak-spawn --host lsof "$@"