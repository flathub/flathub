#!/usr/bin/env bash
set -euo pipefail

export GIT_REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export REPO_OWNER=${REPO_OWNER:-"Dygmalab"}
export REPO_NAME=${REPO_NAME:-"Bazecor"}
export MANIFEST_FILE=${MANIFEST_FILE:-"com.dygma.bazecor.yml"}

# This is just to get rid of the note about flatpak export share folder not bing in the XDG_DATA_DIRS, which
# can be safely ignored, but is annoying and distracting when running the script.
export XDG_DATA_DIRS="${HOME}/.local/share/flatpak/exports/share:${XDG_DATA_DIRS}"

pushd "$GIT_REPO_DIR" >/dev/null || die "Failed to change directory to $GIT_REPO_DIR"


log() {
    printf "📝 [${FUNCNAME[1]}] %s\n" "$*"
}

die() {
    printf "❌ [${FUNCNAME[1]}] Error: %s\n" "$*" >&2
    exit 1
}

success() {
    local message="${*:-Done!}"
    printf "✅ [${FUNCNAME[1]}] %s\n" "$message"
}

completed() {
    local message="${*:-All tasks completed successfully!}"
    printf "🏁 [${FUNCNAME[1]}] %s\n" "$message"
}


require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

html_escape() {
    local s="$1"
    s=${s//&/&amp;/g}
    s=${s//</&lt;/g}
    s=${s//>/&gt;/g}
    s=${s//\"/&quot;/g}
    s=${s//\'/&#39;/g}
    printf '%s\n' "$s"
}

setup_flathub_builder() {
    log "Setting up user flathub remote..."
    flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo && success || die "Failed to add flathub remote. Please check the output for errors."

    if ! flatpak list --user | grep -q org.flatpak.Builder; then
        log "Installing user org.flatpak.Builder..."
        flatpak install --user -y flathub org.flatpak.Builder && success || die "Failed to install org.flatpak.Builder. Please check the output for errors."
    fi
}

validate_flatpak_manifest() {
    local manifest_file="$1"

    log "Linting $manifest_file..."
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest "$manifest_file" && success || die "Failed to lint $manifest_file. Please check the output for errors."
}

validate_flatpak_repo() {
    local repo_dir="${1:-repo}"
    log "Validating flatpak local repo \"$repo_dir\"..."
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo "$repo_dir" && success || die "Failed to lint the local repo \"$repo_dir\". Please check the output for errors."
}