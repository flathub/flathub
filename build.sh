#!/bin/sh
set -eu

VERSION=1.3.0

rm -f NuGet.config

dotnet publish RDPilot.Client/RDPilot.Client.csproj \
    -c Release \
    -r linux-x64 \
    --self-contained false \
    --source ./nuget-sources \
    --source /usr/lib/sdk/dotnet10/nuget/packages \
    -p:Version="${VERSION}" \
    -p:AssemblyVersion="${VERSION}.0" \
    -p:FileVersion="${VERSION}.0" \
    -p:InformationalVersion="${VERSION}" \
    -p:NativeWrapperUseVcpkg=false

install -d "${FLATPAK_DEST}/lib/rdpilot"
cp -a RDPilot.Client/bin/Release/net10.0/linux-x64/publish/. "${FLATPAK_DEST}/lib/rdpilot/"
install -Dm755 rdpilot.sh "${FLATPAK_DEST}/bin/rdpilot"

install -Dm644 "${FLATPAK_ID}.desktop" \
    "${FLATPAK_DEST}/share/applications/${FLATPAK_ID}.desktop"
install -Dm644 "${FLATPAK_ID}.metainfo.xml" \
    "${FLATPAK_DEST}/share/metainfo/${FLATPAK_ID}.metainfo.xml"
install -Dm644 RDPilot.Client/Assets/rdpilot-app-icon.svg \
    "${FLATPAK_DEST}/share/icons/hicolor/scalable/apps/${FLATPAK_ID}.svg"
install -Dm644 RDPilot.Client/Assets/rdpilot-app-icon-256.png \
    "${FLATPAK_DEST}/share/icons/hicolor/256x256/apps/${FLATPAK_ID}.png"
install -Dm644 LICENSE "${FLATPAK_DEST}/share/licenses/${FLATPAK_ID}/LICENSE"
