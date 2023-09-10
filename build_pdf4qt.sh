#!/bin/bash
set -e

# Install VCPKG
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh -disableMetrics
cd ..
export VCPKG_ROOT=$(pwd)/vcpkg

# Install tbb
${VCPKG_ROOT}/vcpkg install tbb

# Build
cd pdf4qt
export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DPDF4QT_INSTALL_QT_DEPENDENCIES=0 -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=/app
cmake --build build
cmake --install build

# Move icons and desktop files to the right place
cd ..

mkdir -p /app/lib/pdf4qt
mkdir -p /app/share/icons
mkdir -p /app/share/applications
mkdir -p /app/share/metainfo

mv -f /app/lib/*Plugin.so*  /app/lib/pdf4qt/
mv -f /app/usr/share/icons/*.svg /app/share/icons/
mv -f /app/usr/share/applications/*.desktop /app/share/applications/
mv -f .appdata.xml /app/share/metainfo/

rmdir /app/usr/share/icons
rmdir /app/usr/share/applications
rmdir /app/usr/share
rmdir /app/usr

# Edit the files

sed -i 's|/usr/bin/||g' /app/share/applications/*.desktop
sed -i 's|/usr/share/icons/||g' /app/share/applications/*.desktop
#sed -i 's|.svg||g' /app/share/applications/*.desktop

# Rename files - flatpak needs prefix io.github.JakubMelka.Pdf4qt, if
# file has to be exported.
mv -f /app/share/applications/Pdf4QtDocDiff.desktop /app/share/applications/io.github.JakubMelka.Pdf4qt.Pdf4QtDocDiff.desktop
mv -f /app/share/applications/Pdf4QtDocPageOrganizer.desktop /app/share/applications/io.github.JakubMelka.Pdf4qt.Pdf4QtDocPageOrganizer.desktop
mv -f /app/share/applications/Pdf4QtViewerLite.desktop /app/share/applications/io.github.JakubMelka.Pdf4qt.Pdf4QtViewerLite.desktop
mv -f /app/share/applications/Pdf4QtViewerProfi.desktop /app/share/applications/io.github.JakubMelka.Pdf4qt.Pdf4QtViewerProfi.desktop

mv -f /app/share/icons/Pdf4QtDocDiff.svg /app/share/icons/io.github.JakubMelka.Pdf4qt.Pdf4QtDocDiff.svg
mv -f /app/share/icons/Pdf4QtDocPageOrganizer.svg /app/share/icons/io.github.JakubMelka.Pdf4qt.Pdf4QtDocPageOrganizer.svg
mv -f /app/share/icons/Pdf4QtViewerLite.svg /app/share/icons/io.github.JakubMelka.Pdf4qt.Pdf4QtViewerLite.svg
mv -f /app/share/icons/Pdf4QtViewerProfi.svg /app/share/icons/io.github.JakubMelka.Pdf4qt.Pdf4QtViewerProfi.svg

# Rename icons in desktop files
sed -i 's|Pdf4QtDocDiff.svg|io.github.JakubMelka.Pdf4qt.Pdf4QtDocDiff.svg|g' /app/share/applications/io.github.JakubMelka.Pdf4qt.Pdf4QtDocDiff.desktop
sed -i 's|Pdf4QtDocPageOrganizer.svg|io.github.JakubMelka.Pdf4qt.Pdf4QtDocPageOrganizer.svg|g' /app/share/applications/io.github.JakubMelka.Pdf4qt.Pdf4QtDocPageOrganizer.desktop
sed -i 's|Pdf4QtViewerLite.svg|io.github.JakubMelka.Pdf4qt.Pdf4QtViewerLite.svg|g' /app/share/applications/io.github.JakubMelka.Pdf4qt.Pdf4QtViewerLite.desktop
sed -i 's|Pdf4QtViewerProfi.svg|io.github.JakubMelka.Pdf4qt.Pdf4QtViewerProfi.svg|g' /app/share/applications/io.github.JakubMelka.Pdf4qt.Pdf4QtViewerProfi.desktop
