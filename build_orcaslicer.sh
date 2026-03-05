#!/bin/bash

# OrcaSlicer Flatpak Build Script
# Builds OrcaSlicer from this Flathub repo using flatpak-builder

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
ARCH=$(uname -m)
BUILD_DIR="build_flatpak"
CLEANUP=false
INSTALL_RUNTIME=false
JOBS=$(nproc)
FORCE_CLEAN=false
ENABLE_CCACHE=true
DISABLE_ROFILES_FUSE=true
NO_DEBUGINFO=true
CACHE_DIR=".flatpak-builder"

MANIFEST="io.github.orcaslicer.OrcaSlicer.yml"
APP_ID="io.github.orcaslicer.OrcaSlicer"

# Help function
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Build OrcaSlicer as a Flatpak package"
    echo ""
    echo "Options:"
    echo "  -a, --arch ARCH        Target architecture (x86_64, aarch64) [default: $ARCH]"
    echo "  -d, --build-dir DIR    Build directory [default: $BUILD_DIR]"
    echo "  -j, --jobs JOBS        Number of parallel build jobs [default: $JOBS]"
    echo "  -c, --cleanup          Clean build directory before building"
    echo "  -f, --force-clean      Force clean build (disables caching)"
    echo "  --no-ccache            Disable ccache"
    echo "  --enable-rofiles-fuse  Enable rofiles-fuse (disabled by default due to FUSE issues)"
    echo "  --with-debuginfo       Include debug info (slower builds, needed for Flathub)"
    echo "  --cache-dir DIR        Flatpak builder cache directory [default: $CACHE_DIR]"
    echo "  -i, --install-runtime  Install required Flatpak runtime and SDK"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                     # Build for current architecture with caching enabled"
    echo "  $0 -f                  # Force clean build (no caching)"
    echo "  $0 -j 8                # Build with 8 parallel jobs (ccache enabled by default)"
    echo "  $0 -i -j 16           # Install runtime, build with 16 jobs"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        -d|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -c|--cleanup)
            CLEANUP=true
            shift
            ;;
        -f|--force-clean)
            FORCE_CLEAN=true
            shift
            ;;
        --no-ccache)
            ENABLE_CCACHE=false
            shift
            ;;
        --enable-rofiles-fuse)
            DISABLE_ROFILES_FUSE=false
            shift
            ;;
        --with-debuginfo)
            NO_DEBUGINFO=false
            shift
            ;;
        --cache-dir)
            CACHE_DIR="$2"
            shift 2
            ;;
        -i|--install-runtime)
            INSTALL_RUNTIME=true
            shift
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

# Validate architecture
if [[ "$ARCH" != "x86_64" && "$ARCH" != "aarch64" ]]; then
    echo -e "${RED}Error: Unsupported architecture '$ARCH'. Supported: x86_64, aarch64${NC}"
    exit 1
fi

# Validate jobs parameter
if ! [[ "$JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo -e "${RED}Error: Jobs must be a positive integer, got '$JOBS'${NC}"
    exit 1
fi

echo -e "${BLUE}OrcaSlicer Flatpak Build Script${NC}"
echo -e "${BLUE}================================${NC}"
echo -e "Architecture: ${GREEN}$ARCH${NC}"
echo -e "Build directory: ${GREEN}$BUILD_DIR${NC}"
echo -e "Cache directory: ${GREEN}$CACHE_DIR${NC}"
echo -e "Parallel jobs: ${GREEN}$JOBS${NC}"
if [[ "$FORCE_CLEAN" == true ]]; then
    echo -e "Cache mode: ${RED}DISABLED (force clean)${NC}"
else
    echo -e "Cache mode: ${GREEN}ENABLED${NC}"
fi
if [[ "$ENABLE_CCACHE" == true ]]; then
    echo -e "Ccache: ${GREEN}ENABLED${NC}"
else
    echo -e "Ccache: ${YELLOW}DISABLED${NC}"
fi
echo ""

# Check available disk space (flatpak builds need several GB)
AVAILABLE_SPACE=$(df . | awk 'NR==2 {print $4}')
REQUIRED_SPACE=$((5 * 1024 * 1024))  # 5GB in KB

if [[ $AVAILABLE_SPACE -lt $REQUIRED_SPACE ]]; then
    echo -e "${YELLOW}Warning: Low disk space detected.${NC}"
    echo -e "Available: $(( AVAILABLE_SPACE / 1024 / 1024 ))GB, Recommended: 5GB+"
    echo -e "Continue anyway? (y/N)"
    read -r response
    if [[ ! "$response" =~ ^[Yy]$ ]]; then
        echo "Build cancelled by user"
        exit 1
    fi
fi

# Check if flatpak is installed
if ! command -v flatpak &> /dev/null; then
    echo -e "${RED}Error: Flatpak is not installed. Please install it first.${NC}"
    echo "On Ubuntu/Debian: sudo apt install flatpak"
    echo "On Fedora: sudo dnf install flatpak"
    echo "On Arch: sudo pacman -S flatpak"
    exit 1
fi

# Check if flatpak-builder is installed
if ! command -v flatpak-builder &> /dev/null; then
    echo -e "${RED}Error: flatpak-builder is not installed. Please install it first.${NC}"
    echo "On Ubuntu/Debian: sudo apt install flatpak-builder"
    echo "On Fedora: sudo dnf install flatpak-builder"
    echo "On Arch: sudo pacman -S flatpak-builder"
    exit 1
fi

# Install runtime and SDK if requested
if [[ "$INSTALL_RUNTIME" == true ]]; then
    echo -e "${YELLOW}Installing GNOME runtime and SDK...${NC}"
    flatpak install --user -y flathub org.gnome.Platform//48
    flatpak install --user -y flathub org.gnome.Sdk//48
fi

# Check if required runtime is available
if ! flatpak info --user org.gnome.Platform//48 &> /dev/null; then
    echo -e "${RED}Error: GNOME Platform 48 runtime is not installed.${NC}"
    echo "Run with -i flag to install it automatically, or install manually:"
    echo "flatpak install --user flathub org.gnome.Platform//48"
    exit 1
fi

if ! flatpak info --user org.gnome.Sdk//48 &> /dev/null; then
    echo -e "${RED}Error: GNOME SDK 48 is not installed.${NC}"
    echo "Run with -i flag to install it automatically, or install manually:"
    echo "flatpak install --user flathub org.gnome.Sdk//48"
    exit 1
fi

# Check manifest exists
if [[ ! -f "$MANIFEST" ]]; then
    echo -e "${RED}Error: Flatpak manifest not found at $MANIFEST${NC}"
    exit 1
fi

# Extract version from manifest git tag
VER=$(grep -A2 'github.com/OrcaSlicer/OrcaSlicer' "$MANIFEST" | grep 'tag:' | head -1 | sed 's/.*tag: *//')
echo -e "OrcaSlicer version: ${GREEN}${VER:-unknown}${NC}"

# Cleanup build directory if requested
if [[ "$CLEANUP" == true ]]; then
    echo -e "${YELLOW}Cleaning up build directory...${NC}"
    rm -rf "$BUILD_DIR"

    if [[ "$FORCE_CLEAN" == true ]]; then
        echo -e "${YELLOW}Cleaning up build cache...${NC}"
        rm -rf "$CACHE_DIR"
    else
        echo -e "${BLUE}Preserving build cache at: $CACHE_DIR${NC}"
    fi
fi

# Create build directory
mkdir -p "$BUILD_DIR"
rm -rf "$BUILD_DIR/build-dir"

# Build the Flatpak
echo -e "${YELLOW}Building Flatpak package...${NC}"
echo -e "This may take a while (30+ minutes depending on your system)..."
echo ""

BUNDLE_NAME="OrcaSlicer-Linux-flatpak_${VER:-dev}_${ARCH}.flatpak"
rm -f "$BUNDLE_NAME"

export FLATPAK_BUILDER_N_JOBS=$JOBS

echo -e "${BLUE}Running flatpak-builder...${NC}"
echo -e "Using $JOBS parallel jobs"

FLATPAK_BUILDER_VERSION=$(flatpak-builder --version 2>/dev/null | head -1 | awk '{print $2}' || echo "unknown")
echo -e "flatpak-builder version: $FLATPAK_BUILDER_VERSION"

# Build command
BUILDER_ARGS=(
    --arch="$ARCH"
    --user
    --install-deps-from=flathub
    --repo="$BUILD_DIR/repo"
    --verbose
    --state-dir="$CACHE_DIR"
    --jobs="$JOBS"
    --mirror-screenshots-url=https://dl.flathub.org/media/
)

if [[ "$FORCE_CLEAN" == true ]]; then
    BUILDER_ARGS+=(--force-clean)
    echo -e "${YELLOW}Using --force-clean (caching disabled)${NC}"
else
    echo -e "${GREEN}Using build cache for faster rebuilds${NC}"
fi

if [[ "$ENABLE_CCACHE" == true ]]; then
    BUILDER_ARGS+=(--ccache)
    echo -e "${GREEN}Using ccache for compiler caching${NC}"
fi

if [[ "$DISABLE_ROFILES_FUSE" == true ]]; then
    BUILDER_ARGS+=(--disable-rofiles-fuse)
    echo -e "${YELLOW}rofiles-fuse disabled${NC}"
fi

# Use a temp manifest with no-debuginfo if requested
BUILD_MANIFEST="$MANIFEST"
if [[ "$NO_DEBUGINFO" == true ]]; then
    BUILD_MANIFEST="${MANIFEST%.yml}.no-debug.yml"
    sed '0,/^finish-args:/s//build-options:\n  no-debuginfo: true\n  strip: true\nfinish-args:/' \
        "$MANIFEST" > "$BUILD_MANIFEST"
    echo -e "${YELLOW}Debug info disabled (using temp manifest)${NC}"
fi

if ! flatpak-builder \
    "${BUILDER_ARGS[@]}" \
    "$BUILD_DIR/build-dir" \
    "$BUILD_MANIFEST"; then
    echo -e "${RED}Error: flatpak-builder failed${NC}"
    echo -e "${YELLOW}Check the build log above for details${NC}"
    rm -f "$BUILD_MANIFEST"
    exit 1
fi

# Clean up temp manifest
if [[ "$BUILD_MANIFEST" != "$MANIFEST" ]]; then
    rm -f "$BUILD_MANIFEST"
fi

# Create bundle
echo -e "${YELLOW}Creating Flatpak bundle...${NC}"
if ! flatpak build-bundle \
    "$BUILD_DIR/repo" \
    "$BUNDLE_NAME" \
    "$APP_ID" \
    --arch="$ARCH"; then
    echo -e "${RED}Error: Failed to create Flatpak bundle${NC}"
    exit 1
fi

# Success message
echo ""
echo -e "${GREEN}Flatpak build completed successfully!${NC}"
echo -e "Bundle created: ${GREEN}$BUNDLE_NAME${NC}"
echo -e "Size: ${GREEN}$(du -h "$BUNDLE_NAME" | cut -f1)${NC}"
if [[ "$FORCE_CLEAN" != true ]]; then
    echo -e "Build cache: ${GREEN}$CACHE_DIR${NC} (preserved for faster future builds)"
fi
echo ""
echo -e "${BLUE}To install:${NC}"
echo -e "flatpak install --user $BUNDLE_NAME"
echo ""
echo -e "${BLUE}To run:${NC}"
echo -e "flatpak run $APP_ID"
echo ""
echo -e "${BLUE}To uninstall:${NC}"
echo -e "flatpak uninstall --user $APP_ID"
echo ""
if [[ "$FORCE_CLEAN" != true ]]; then
    echo -e "${BLUE}Cache Management:${NC}"
    echo -e "  Subsequent builds will be faster thanks to caching"
    echo -e "  To force a clean build: $0 -f"
    echo -e "  To clean cache manually: rm -rf $CACHE_DIR"
fi
