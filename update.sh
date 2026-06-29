#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/common.sh"

tmpDownloadDir=$(mktemp -d)
trap 'rm -rf "$tmpDownloadDir"' EXIT

usage() {
    cat <<'EOF'
Usage:
  ./update.sh [--version <latest|VERSION-TAG>]

Examples:
  ./update.sh
  ./update.sh --version latest
  ./update.sh --version v1.9.0
EOF
}

update_flatpak_manifest() {
    local NEW_ARCHIVE_URL="$1"
    local NEW_ARCHIVE_SHA256="$2"
    local NEW_TAG="$3"

    local NEW_ICON_URL="https://raw.githubusercontent.com/$REPO_OWNER/$REPO_NAME/refs/tags/$NEW_TAG/build/logo.png"
    local NEW_APPSTREAM_URL="https://raw.githubusercontent.com/$REPO_OWNER/$REPO_NAME/refs/tags/$NEW_TAG/build/com.dygma.bazecor.metainfo.xml"
    local NEW_DESKTOP_URL="https://raw.githubusercontent.com/$REPO_OWNER/$REPO_NAME/refs/tags/$NEW_TAG/build/com.dygma.bazecor.desktop"

    log "Downloading logo.png from $NEW_ICON_URL..."
    curl -fL# "$NEW_ICON_URL" -o "$tmpDownloadDir/logo.png" && success || die "Failed to download logo.png from $NEW_ICON_URL"
    [ -f "$tmpDownloadDir/logo.png" ] || die "Downloaded logo.png file not found: $tmpDownloadDir/logo.png"
    [ -s "$tmpDownloadDir/logo.png" ] || die "Downloaded logo.png file is empty: $tmpDownloadDir/logo.png"
    local NEW_ICON_SHA256=$(sha256sum "$tmpDownloadDir/logo.png" | awk '{print $1}')
    [ -n "$NEW_ICON_SHA256" ] || die "Failed to compute SHA256 for logo.png"

    log "Downloading com.dygma.bazecor.metainfo.xml from $NEW_APPSTREAM_URL..."
    curl -fL# "$NEW_APPSTREAM_URL" -o "$tmpDownloadDir/com.dygma.bazecor.metainfo.xml" && success || die "Failed to download com.dygma.bazecor.metainfo.xml from $NEW_APPSTREAM_URL"
    [ -f "$tmpDownloadDir/com.dygma.bazecor.metainfo.xml" ] || die "Downloaded com.dygma.bazecor.metainfo.xml file not found: $tmpDownloadDir/com.dygma.bazecor.metainfo.xml"
    [ -s "$tmpDownloadDir/com.dygma.bazecor.metainfo.xml" ] || die "Downloaded com.dygma.bazecor.metainfo.xml file is empty: $tmpDownloadDir/com.dygma.bazecor.metainfo.xml"
    local NEW_APPSTREAM_SHA256=$(sha256sum "$tmpDownloadDir/com.dygma.bazecor.metainfo.xml" | awk '{print $1}')
    [ -n "$NEW_APPSTREAM_SHA256" ] || die "Failed to compute SHA256 for com.dygma.bazecor.metainfo.xml"

    log "Downloading com.dygma.bazecor.desktop from $NEW_DESKTOP_URL..."
    curl -fL# "$NEW_DESKTOP_URL" -o "$tmpDownloadDir/com.dygma.bazecor.desktop" && success || die "Failed to download com.dygma.bazecor.desktop from $NEW_DESKTOP_URL"
    [ -f "$tmpDownloadDir/com.dygma.bazecor.desktop" ] || die "Downloaded com.dygma.bazecor.desktop file not found: $tmpDownloadDir/com.dygma.bazecor.desktop"
    [ -s "$tmpDownloadDir/com.dygma.bazecor.desktop" ] || die "Downloaded com.dygma.bazecor.desktop file is empty: $tmpDownloadDir/com.dygma.bazecor.desktop"
    local NEW_DESKTOP_SHA256=$(sha256sum "$tmpDownloadDir/com.dygma.bazecor.desktop" | awk '{print $1}')
    [ -n "$NEW_DESKTOP_SHA256" ] || die "Failed to compute SHA256 for com.dygma.bazecor.desktop"

    NEW_ARCHIVE_URL="$NEW_ARCHIVE_URL" NEW_ARCHIVE_SHA256="$NEW_ARCHIVE_SHA256" NEW_ICON_URL="$NEW_ICON_URL" NEW_ICON_SHA256="$NEW_ICON_SHA256" NEW_APPSTREAM_URL="$NEW_APPSTREAM_URL" NEW_DESKTOP_URL="$NEW_DESKTOP_URL" NEW_DESKTOP_SHA256="$NEW_DESKTOP_SHA256" yq eval -i '
        (.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "file" and .dest-filename == "Bazecor.AppImage"))               |= (.url = strenv(NEW_ARCHIVE_URL)   | .sha256 = strenv(NEW_ARCHIVE_SHA256))
      | (.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "file" and .dest-filename == "com.dygma.bazecor.png"))          |= (.url = strenv(NEW_ICON_URL)      | .sha256 = strenv(NEW_ICON_SHA256))
      | (.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "file" and .dest-filename == "com.dygma.bazecor.metainfo.xml")) |= (.url = strenv(NEW_APPSTREAM_URL) | .sha256 = strenv(NEW_APPSTREAM_SHA256))
      | (.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "file" and .dest-filename == "com.dygma.bazecor.desktop"))      |= (.url = strenv(NEW_DESKTOP_URL)   | .sha256 = strenv(NEW_DESKTOP_SHA256))
    ' "$MANIFEST_FILE" || die "Failed to update $MANIFEST_FILE using yq."

    validate_flatpak_manifest "$MANIFEST_FILE"
}

update_generated_sources() {

    local yarn_lock_file="$tmpDownloadDir/yarn.lock"

    if [ ! -f "$yarn_lock_file" ]; then
        die "Yarn lock file not found: $yarn_lock_file"
    fi

    log "Generating generated-sources.json from $yarn_lock_file..."
    flatpak-node-generator --electron-node-headers yarn "$yarn_lock_file" && success || die "Failed to generate generated-sources.json from $yarn_lock_file"
}

main() {
    local version_ref="latest"
    local api_url release_json asset_name asset_url asset_size asset_sha256
    local local_info

    while [ "$#" -gt 0 ]; do
        case "$1" in
        --version)
            shift
            [ "$#" -gt 0 ] || die "Missing value after --version"
            version_ref="$1"
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            die "Unknown argument: $1"
            ;;
        esac
        shift
    done

    require_cmd appstreamcli
    require_cmd awk
    require_cmd curl
    require_cmd flatpak
    require_cmd flatpak-node-generator
    require_cmd git
    require_cmd jq
    require_cmd mktemp
    require_cmd sed
    require_cmd sha256sum
    require_cmd yq

    # Validate that the installed yq is Mike Farah's yq v4, which is required for this script to work correctly.
    yq --version | grep -q 'mikefarah/yq' || die "This script requires Mike Farah yq v4 (https://github.com/mikefarah/yq)."

    [ -f "$MANIFEST_FILE" ] || die "Could not find $MANIFEST_FILE"

    setup_flathub_builder

    if [ "$version_ref" = "latest" ]; then
        api_url="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/releases/latest"
    else
        api_url="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/releases/tags/$version_ref"
    fi

    log "Fetching release metadata from GitHub API: $api_url"
    release_json="$(curl -fsSL "$api_url")" && success || die "Failed to fetch release data from GitHub API."
    # echo "${release_json}" >> "release.json"  # Save the release JSON for debugging purposes

    tag_name="$(printf '%s' "$release_json" | jq -r '.tag_name // empty')"
    [ -n "$tag_name" ] || die "Release tag_name is missing in API response."

    asset_name="$tag_name.tar.gz"
    asset_url="https://github.com/$REPO_OWNER/$REPO_NAME/archive/refs/tags/$asset_name"
    [ -n "$asset_url" ] || die "Missing tarball_url for asset '$asset_name'."

    log "Downloading $asset_url into $tmpDownloadDir/$asset_name..."
    curl -fL# "$asset_url" -o "$tmpDownloadDir/$asset_name" && success || die "Failed to download $asset_name from $asset_url"

    log "Computing SHA256 for $tmpDownloadDir/$asset_name..."
    asset_sha256=$(sha256sum "$tmpDownloadDir/$asset_name" | awk '{print $1}')
    [ -n "$asset_sha256" ] && success || die "Failed to compute SHA256 for asset '$asset_name'."

    log "Extracting $tmpDownloadDir/$asset_name..."
    tar -xzf "$tmpDownloadDir/$asset_name" -C "$tmpDownloadDir" --strip-components=1 && success || die "Failed to extract $asset_name"

    extracted_version=$(printf '%s' "$tag_name" | sed -E 's/^v//; s/[^0-9A-Za-z.-]//g')
    [ -n "$extracted_version" ] || die "Failed to extract version from tag_name '$tag_name'."

    echo "AppImage asset: $asset_name"
    echo "Version:        $extracted_version"
    echo "SHA256:         $asset_sha256"
    echo

    read -r -p "Proceed? [Y/n] " prompt
    if [[ "${prompt:-Y}" =~ ^[Nn]$ ]]; then
        echo "Aborted by user."
        exit 0
    fi

    update_flatpak_manifest "$asset_url" "$asset_sha256" "$tag_name"
    update_generated_sources

    completed
}

main "$@"
