#!/bin/bash
# Wrapper ini menggunakan zypak-wrapper, yaitu sistem Flatpak khusus untuk menghindari bentrok 
# antara sandbox Chromium bawaan Electron dan sandbox Flatpak.
exec zypak-wrapper /app/main/truckers-tool-linux "$@"
