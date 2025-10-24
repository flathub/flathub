#!/bin/bash

# CliA Build Script

set -e

echo ""
echo "╔════════════════════════════════════════╗"
echo "║                                         ║"
echo "║ Building Flatpak for com.github.luke.clia ║"
echo "║                                         ║"
echo "║  CliA - AI Terminal Agent         ║"
echo "║                                         ║"
echo "╚═════════════════════════════════════════╝"
echo ""

# Check if flatpak is installed
if ! command -v flatpak &> /dev/null; then
    echo "❌ Flatpak이 설치되어 있지 않습니다."
    echo ""
    echo "설치 방법:"
    echo "  sudo pacman -S flatpak"
    echo ""
    exit 1
fi

# Check if flatpak-builder is installed
if ! command -v flatpak-builder &> /dev/null; then
    echo "❌ flatpak-builder가 설치되어 있지 않습니다."
    echo ""
    echo "설치 방법:"
    echo "  sudo pacman -S flatpak-builder"
    echo ""
    exit 1
fi

# Check if flathub is added
if ! flatpak remotes | grep -q "flathub"; then
    echo "📦 Flathub 저장소 추가 중..."
    flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
fi

# Check if GNOME SDK is installed
if ! flatpak list | grep -q "org.gnome.Sdk//47"; then
    echo "📦 GNOME SDK 47 설치 중... (처음 한 번만 필요)"
    echo "   크기가 크므로 시간이 걸릴 수 있습니다."
    echo ""
    flatpak install -y flathub org.gnome.Platform//47 org.gnome.Sdk//47
fi

echo ""
echo "🔨 CliA 빌드 중..."
echo "   (모든 의존성이 자동으로 포함됩니다)"
echo ""
echo "⚠️  캐시 정리 중... (손상된 빌드 방지)"
echo ""

# Clean up old caches completely
rm -rf /tmp/clia-build-* /tmp/clia-state-*
rm -rf ~/.cache/flatpak-builder

# Use /tmp for build directory to avoid issues with Google Drive/network filesystems
TIMESTAMP=$(date +%s)
BUILD_DIR="/tmp/clia-build-$TIMESTAMP"
STATE_DIR="/tmp/clia-state-$TIMESTAMP"

# Build the flatpak with --rebuild-on-sdk-change to ensure clean build
flatpak-builder --user --install --force-clean --rebuild-on-sdk-change --state-dir="$STATE_DIR" "$BUILD_DIR" net.bloupla.clia.json

# Clean up
rm -rf "$BUILD_DIR" "$STATE_DIR"

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  ✅ 설치 완료!                          ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "실행 방법:"
echo "  ./run.sh"
echo ""
echo "또는:"
echo "  flatpak run net.bloupla.clia"
echo ""

