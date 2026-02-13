#!/bin/bash
set -euo pipefail

#===============================================================================
# Disgaea DS Manager - x64 Flatpak Build Script
# Builds, bundles, and optionally installs the x64 version
#===============================================================================

# Configuration
readonly APP_ID="com.LandonAndEmma.DisgaeaDSManager"
readonly MANIFEST="com.LandonAndEmma.DisgaeaDSManager.x64.yml"
readonly BUILD_DIR="build-dir-x64"
readonly REPO_DIR="repo-x64"
readonly BUNDLE_NAME="DisgaeaDSManager-x64.flatpak"
readonly ARCH="x86_64"
readonly BRANCH="master"

# Colors for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

#===============================================================================
# Functions
#===============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

cleanup_on_error() {
    log_error "Build failed! Cleaning up partial build artifacts..."
    rm -rf "$BUILD_DIR" "$REPO_DIR" "$BUNDLE_NAME"
    exit 1
}

check_prerequisites() {
    log_info "Checking prerequisites..."
    
    local missing_deps=()
    
    command -v flatpak-builder >/dev/null 2>&1 || missing_deps+=("flatpak-builder")
    command -v flatpak >/dev/null 2>&1 || missing_deps+=("flatpak")
    command -v ostree >/dev/null 2>&1 || missing_deps+=("ostree")
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_error "Missing required dependencies: ${missing_deps[*]}"
        exit 1
    fi
    
    if [ ! -f "$MANIFEST" ]; then
        log_error "Manifest file not found: $MANIFEST"
        exit 1
    fi
    
    log_success "All prerequisites satisfied"
}

clean_build_artifacts() {
    log_info "Cleaning previous build artifacts..."
    
    rm -rf "$BUILD_DIR" "$REPO_DIR" "$BUNDLE_NAME"
    
    log_success "Cleanup complete"
}

build_flatpak() {
    log_info "Building Flatpak application..."
    
    if ! flatpak-builder \
        --force-clean \
        --repo="$REPO_DIR" \
        --ccache \
        --disable-rofiles-fuse \
        "$BUILD_DIR" \
        "$MANIFEST"; then
        log_error "flatpak-builder failed"
        cleanup_on_error
    fi
    
    log_success "Flatpak build complete"
}

verify_repo() {
    log_info "Verifying repository structure..."
    
    if [ ! -d "$REPO_DIR" ]; then
        log_error "Repository directory not created"
        cleanup_on_error
    fi
    
    # Check if the expected ref exists
    local expected_ref="app/$APP_ID/$ARCH/$BRANCH"
    if ! ostree refs --repo="$REPO_DIR" | grep -q "$expected_ref"; then
        log_error "Expected ref not found: $expected_ref"
        log_info "Available refs:"
        ostree refs --repo="$REPO_DIR"
        cleanup_on_error
    fi
    
    log_success "Repository structure verified"
}

create_bundle() {
    log_info "Creating Flatpak bundle..."
    
    if ! flatpak build-bundle \
        "$REPO_DIR" \
        "$BUNDLE_NAME" \
        "$APP_ID" \
        "$BRANCH"; then
        log_error "flatpak build-bundle failed"
        cleanup_on_error
    fi
    
    if [ ! -f "$BUNDLE_NAME" ]; then
        log_error "Bundle file was not created"
        cleanup_on_error
    fi
    
    local bundle_size=$(du -h "$BUNDLE_NAME" | cut -f1)
    log_success "Bundle created: $BUNDLE_NAME ($bundle_size)"
}

install_and_run() {
    log_info "Installing Flatpak bundle..."
    
    if flatpak install --user --reinstall "$BUNDLE_NAME" -y; then
        log_success "Installation complete"
        
        log_info "Launching application..."
        if flatpak run "$APP_ID"; then
            log_success "Application launched successfully"
        else
            log_warning "Application exited with non-zero status"
        fi
    else
        log_warning "Installation failed or was cancelled"
        return 1
    fi
}

show_summary() {
    echo ""
    echo "════════════════════════════════════════════════════════════"
    log_success "Build Summary"
    echo "════════════════════════════════════════════════════════════"
    echo "  App ID:      $APP_ID"
    echo "  Architecture: $ARCH"
    echo "  Branch:      $BRANCH"
    echo "  Bundle:      $BUNDLE_NAME"
    echo "  Location:    $(pwd)/$BUNDLE_NAME"
    echo "════════════════════════════════════════════════════════════"
    echo ""
}

#===============================================================================
# Main Execution
#===============================================================================

main() {
    echo ""
    echo "════════════════════════════════════════════════════════════"
    log_info "Disgaea DS Manager - x64 Flatpak Builder"
    echo "════════════════════════════════════════════════════════════"
    echo ""
    
    # Set trap for cleanup on error
    trap cleanup_on_error ERR
    
    # Build pipeline
    check_prerequisites
    clean_build_artifacts
    build_flatpak
    verify_repo
    create_bundle
    
    # Optional: Install and run
    if [ "${SKIP_INSTALL:-0}" != "1" ]; then
        install_and_run
    else
        log_info "Skipping installation (SKIP_INSTALL=1)"
    fi
    
    # Show summary
    show_summary
    
    log_success "All operations completed successfully!"
}

# Run main function
main "$@"
