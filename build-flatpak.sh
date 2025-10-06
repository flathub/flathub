#!/bin/bash

# Flatpak build script for Pineapple Steam Recording Exporter
set -e

# Configuration
BUILD_DIR="build-flatpak"
MANIFEST_FILE_PATH="net.blumia.pineapple-steam-recording-exporter.yml"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check required tools
check_dependencies() {
    print_status "Checking dependencies..."

    local missing_deps=()

    if ! command -v flatpak-builder &> /dev/null; then
        missing_deps+=("flatpak-builder")
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_error "Please install the required dependencies and try again."
        exit 1
    fi

    print_status "All required dependencies found."
}

flatpakbuilder_download() {
    print_status "Downloading resources listed in the manifest for building..."
    flatpak-builder build-flatpak "$MANIFEST_FILE_PATH" --download-only --force-clean
}

flatpakbuilder_build() {
    print_status "Building flatpak package..."
    flatpak-builder build-flatpak "$MANIFEST_FILE_PATH" --force-clean --disable-cache --disable-download
}

flatpakbuilder_install() {
    print_status "Install to current user for testing..."
    flatpak-builder build-flatpak "$MANIFEST_FILE_PATH" --force-clean --disable-download --install --user
}

# Main execution
main() {
    print_status "Starting Flatpak build process..."
    print_status "Build directory: $BUILD_DIR"

    # Change to script directory
    cd "$(dirname "$0")"

    check_dependencies
    flatpakbuilder_download
    flatpakbuilder_install

    print_status "Flatpak build completed successfully!"
}

# Handle script arguments
case "$1" in
    --help|-h)
        echo "Usage: $0 [--help]"
        echo ""
        echo "Options:"
        echo "  --help, -h      Show this help message"
        exit 0
        ;;
    *)
        main "$@"
        ;;
esac
