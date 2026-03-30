#!/bin/bash

# OrcaSlicer Flatpak Build Script
# Builds OrcaSlicer from this Flathub repo using Docker with the same
# container image as the Flathub CI.
#
# Requirements: Docker (or Podman with docker compatibility)

set -euo pipefail
SECONDS=0

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
ARCH=$(uname -m)
FORCE_PULL=false
CONTAINER_IMAGE="ghcr.io/flathub-infra/flatpak-github-actions:gnome-50"

MANIFEST="com.orcaslicer.OrcaSlicer.yml"
APP_ID="com.orcaslicer.OrcaSlicer"

normalize_arch() {
    case "$1" in
        arm64|aarch64)  echo "aarch64" ;;
        x86_64|amd64)   echo "x86_64" ;;
        *)              echo "$1" ;;
    esac
}

# Help function
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Build OrcaSlicer as a Flatpak package using Docker"
    echo ""
    echo "Options:"
    echo "  -a, --arch ARCH        Target architecture (x86_64, aarch64) [default: $ARCH]"
    echo "  --pull                 Force pull the container image"
    echo "  --image IMAGE          Override container image [default: $CONTAINER_IMAGE]"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                     # Build for current architecture"
    echo "  $0 --pull              # Force pull latest container image"
    echo "  $0 -a aarch64          # Cross-build for aarch64"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        --pull)
            FORCE_PULL=true
            shift
            ;;
        --image)
            CONTAINER_IMAGE="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Error: Unknown option $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

ARCH="$(normalize_arch "$ARCH")"

# Validate architecture
if [[ "$ARCH" != "x86_64" && "$ARCH" != "aarch64" ]]; then
    echo -e "${RED}Error: Unsupported architecture '$ARCH'. Supported: x86_64, aarch64${NC}"
    exit 1
fi

cd "$SCRIPT_DIR"

# Check manifest exists
if [[ ! -f "$MANIFEST" ]]; then
    echo -e "${RED}Error: Flatpak manifest not found at $MANIFEST${NC}"
    exit 1
fi

# Extract version from manifest git tag
VER=$(grep -A2 'github.com/OrcaSlicer/OrcaSlicer' "$MANIFEST" | grep 'tag:' | head -1 | sed 's/.*tag: *//' || true)
BUNDLE_NAME="OrcaSlicer-Linux-flatpak_${VER:-dev}_${ARCH}.flatpak"

echo -e "${BLUE}OrcaSlicer Flatpak Build Script (Docker)${NC}"
echo -e "${BLUE}==========================================${NC}"
echo -e "  Version:      ${GREEN}${VER:-unknown}${NC}"
echo -e "  Architecture: ${GREEN}$ARCH${NC}"
echo -e "  Image:        ${GREEN}$CONTAINER_IMAGE${NC}"
echo -e "  Bundle:       ${GREEN}$BUNDLE_NAME${NC}"
echo -e "  Pull mode:    $([ "$FORCE_PULL" = true ] && echo -e "${YELLOW}force${NC}" || echo -e "${GREEN}auto${NC}")"
echo -e "  ccache:       ${GREEN}enabled${NC}"
echo ""

# Check Docker is available
DOCKER="${DOCKER:-docker}"
if ! command -v "$DOCKER" &> /dev/null; then
    echo -e "${RED}Error: Docker is not installed. Please install Docker first.${NC}"
    echo "See: https://docs.docker.com/get-docker/"
    exit 1
fi

BUILD_MANIFEST="$MANIFEST"

# Pull container image
if [[ "$FORCE_PULL" == true ]]; then
    echo -e "${YELLOW}Pulling container image (--pull requested)...${NC}"
    "$DOCKER" pull "$CONTAINER_IMAGE"
elif ! "$DOCKER" image inspect "$CONTAINER_IMAGE" &>/dev/null; then
    echo -e "${YELLOW}Pulling container image (not found locally)...${NC}"
    "$DOCKER" pull "$CONTAINER_IMAGE"
else
    echo -e "${GREEN}Using cached container image (use --pull to update)${NC}"
fi

# Determine which manifest to use inside the container
CONTAINER_MANIFEST="$(basename "$BUILD_MANIFEST")"

rm -f "$BUNDLE_NAME"

echo ""
echo -e "${YELLOW}Starting Flatpak build inside container...${NC}"
echo -e "This may take a while (30+ minutes depending on your system)..."
echo ""

DOCKER_RUN_ARGS=(run --rm -i --privileged)

"$DOCKER" "${DOCKER_RUN_ARGS[@]}" \
    -v "$SCRIPT_DIR":/src:Z \
    -w /src \
    -e "BUILD_ARCH=$ARCH" \
    -e "BUNDLE_NAME=$BUNDLE_NAME" \
    -e "CONTAINER_MANIFEST=$CONTAINER_MANIFEST" \
    -e "APP_ID=$APP_ID" \
    "$CONTAINER_IMAGE" \
    bash -s <<'EOF'
set -euo pipefail

format_duration() {
    local total_seconds="$1"
    local hours=$((total_seconds / 3600))
    local minutes=$(((total_seconds % 3600) / 60))
    local seconds=$((total_seconds % 60))
    printf "%02d:%02d:%02d" "$hours" "$minutes" "$seconds"
}

overall_start=$(date +%s)
install_start=$overall_start

# Fix git ownership mismatch inside the container
git config --global --add safe.directory '*'

# Clean stale per-module build dirs to prevent cp -al collisions
# (type:git sources fail on repeat builds when .git/ already exists)
rm -rf .flatpak-builder/build

# Install required SDK extensions
flatpak install -y --noninteractive --arch="$BUILD_ARCH" flathub \
    org.gnome.Platform//50 \
    org.gnome.Sdk//50 \
    org.freedesktop.Sdk.Extension.llvm21//25.08 || true

install_end=$(date +%s)
install_duration=$((install_end - install_start))

builder_start=$(date +%s)
flatpak-builder --force-clean \
    --verbose \
    --ccache \
    --disable-rofiles-fuse \
    --state-dir=.flatpak-builder \
    --arch="$BUILD_ARCH" \
    --repo=flatpak-repo \
    flatpak-build \
    "$CONTAINER_MANIFEST"
builder_end=$(date +%s)
builder_duration=$((builder_end - builder_start))

bundle_start=$(date +%s)
flatpak build-bundle \
    --arch="$BUILD_ARCH" \
    flatpak-repo \
    "$BUNDLE_NAME" \
    "$APP_ID"
bundle_end=$(date +%s)
bundle_duration=$((bundle_end - bundle_start))

# Fix ownership so output files are not root-owned on the host
owner="$(stat -c %u:%g /src)"
chown -R "$owner" .flatpak-builder flatpak-build flatpak-repo "$BUNDLE_NAME" 2>/dev/null || true

overall_end=$(date +%s)
overall_duration=$((overall_end - overall_start))

echo ""
echo "=== Build Stats ==="
echo "  Runtime install: $(format_duration "$install_duration")"
echo "  flatpak-builder: $(format_duration "$builder_duration")"
echo "  Bundle export:   $(format_duration "$bundle_duration")"
echo "  Overall:         $(format_duration "$overall_duration")"
EOF

# Success message
echo ""
echo -e "${GREEN}Flatpak build completed successfully!${NC}"
echo -e "Bundle created: ${GREEN}$BUNDLE_NAME${NC}"
echo -e "Size: ${GREEN}$(du -h "$BUNDLE_NAME" | cut -f1)${NC}"
echo ""
echo -e "${BLUE}To install:${NC}"
echo -e "  flatpak install --user $BUNDLE_NAME"
echo ""
echo -e "${BLUE}To run:${NC}"
echo -e "  flatpak run $APP_ID"
echo ""
echo -e "${BLUE}To uninstall:${NC}"
echo -e "  flatpak uninstall --user $APP_ID"

elapsed=$SECONDS
printf "\nBuild completed in %dh %dm %ds\n" $((elapsed/3600)) $((elapsed%3600/60)) $((elapsed%60))
