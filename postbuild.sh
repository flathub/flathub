#!/bin/bash
set -e

mkdir -p /app/lib/pdf4qt
mv -f /app/lib/*Plugin.so*  /app/lib/pdf4qt/

cp /app/share/icons/io.github.JakubMelka.Pdf4qt.Pdf4QtViewerProfi.svg /app/share/icons/io.github.JakubMelka.Pdf4qt.svg
cp /app/share/icons/hicolor/128x128/apps/io.github.JakubMelka.Pdf4qt.Pdf4QtViewerProfi.png /app/share/icons/hicolor/128x128/apps/io.github.JakubMelka.Pdf4qt.png
