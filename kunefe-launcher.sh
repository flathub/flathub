#!/bin/bash
# KunefeSMP Launcher baslatici scripti (Flatpak sandbox icinden calisir)
set -e

# Java 21 yolunu ayarla
export JAVA_HOME=/app/jre
export PATH="$JAVA_HOME/bin:$PATH"

# pip ile /app altina kurulan Python paketlerini bul
for d in /app/lib/python3.*/site-packages; do
    if [ -d "$d" ]; then
        export PYTHONPATH="$d${PYTHONPATH:+:$PYTHONPATH}"
        break
    fi
done

# Flatpak veri dizini kontrolu (XDG_DATA_HOME)
KUNEFE_DATA="${XDG_DATA_HOME:-$HOME/.local/share}/kunefesmp"
mkdir -p "$KUNEFE_DATA"

# Flatpak icindeki paylasilan kaynaklari veri dizinine hazirla (eger yoksa)
if [ -d "/app/share/kunefesmp/resourcepacks" ] && [ ! -d "$KUNEFE_DATA/resourcepacks" ]; then
    mkdir -p "$KUNEFE_DATA/resourcepacks"
    cp -rn /app/share/kunefesmp/resourcepacks/* "$KUNEFE_DATA/resourcepacks/" 2>/dev/null || true
fi

if [ -d "/app/share/kunefesmp/skins" ] && [ ! -d "$KUNEFE_DATA/skins" ]; then
    mkdir -p "$KUNEFE_DATA/skins"
    cp -rn /app/share/kunefesmp/skins/* "$KUNEFE_DATA/skins/" 2>/dev/null || true
fi

# Launcher'i Python ile calistir
exec python3 /app/bin/launcher.py "$@"
