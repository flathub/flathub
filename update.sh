#!/usr/bin/env bash

# THIS IS FOR RELEASE PURPOSES ONLY. USED FOR AUTO-UPDATING com.dygma.bazecor.yml FOR SUBMITTING A NEW RELEASE.

set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

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

get_release() {
	local version_ref="$1"
	if [ "$version_ref" = "latest" ]; then
		api_url="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/releases/latest"
	else
		api_url="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/releases/tags/$version_ref"
	fi
	curl -fsSL "$api_url"
}

get_ref() {
	local tag_name="$1"
	api_url="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/git/ref/tags/$tag_name"
	curl -fsSL "$api_url"
}

# Downloads and extracts the selected app version from GitHub releases.
# Arguments:
#   $1: [OUTPUT] Name of the variable to store the extracted release tag name
#   $2: [OUTPUT] Name of the variable to store the extracted commit SHA
download_and_extract_selected_app_version() {
	local -n tag_name_ref="$1"
	local -n commit_sha_ref="$2"

	local asset_name
	local asset_url
	local release_json
	local ref_json
	local release_name

	log "Fetching release metadata for \"$version_ref\""
	release_json="$(get_release "$version_ref")" && success || die "Failed to fetch release data from GitHub API."
	echo "${release_json}" >"release.json" # Save the release JSON for debugging purposes

	release_name="$(printf '%s' "$release_json" | jq -r '.name // empty')"
	[ -n "$release_name" ] || die "Release name is missing in API response."

	tag_name_ref="$(printf '%s' "$release_json" | jq -r '.tag_name // empty')"
	[ -n "$tag_name_ref" ] || die "Release tag_name is missing in API response."

	log "Fetching reference metadata for \"$tag_name\""
	ref_json="$(get_ref "$tag_name")" && success || die "Failed to fetch release reference from GitHub API."
	echo "${ref_json}" >"ref.json" # Save the ref JSON for debugging purposes

	commit_sha_ref="$(printf '%s' "$ref_json" | jq -r '.object.sha // empty')"
	[ -n "$commit_sha_ref" ] || die "Commit SHA is missing in reference API response."

	asset_name="$tag_name.tar.gz"
	asset_url="https://github.com/$REPO_OWNER/$REPO_NAME/archive/refs/tags/$asset_name"

	log "Downloading $asset_url into $tmpDownloadDir/$asset_name..."
	curl -fL# "$asset_url" -o "$tmpDownloadDir/$asset_name" && success || die "Failed to download $asset_name from $asset_url"

	log "Extracting $tmpDownloadDir/$asset_name..."
	tar -xzf "$tmpDownloadDir/$asset_name" -C "$tmpDownloadDir" --strip-components=1 && success || die "Failed to extract $asset_name"

	echo "Name:           $release_name"
	echo "tag:            $tag_name"
	echo "Commit:         $commit_sha"
	echo
	read -r -p "Proceed? [Y/n] " prompt
	if [[ "${prompt:-Y}" =~ ^[Nn]$ ]]; then
		echo "Aborted by user."
		exit 1
	fi
}

# This function updates the Flatpak manifest file with the new tag name and commit SHA.
# Arguments:
#   $1: The new tag name to set in the manifest.
#   $2: The new commit SHA to set in the manifest.
#
# It updates the following section in the manifest file to the selected tag name and commit SHA:
#
#     sources:
#       - type: git
#         url: "https://github.com/Dygmalab/Bazecor"
#         tag: <NEW TAG NAME HERE>
#         commit: <NEW COMMIT SHA HERE>
#
update_flatpak_manifest() {
	local tag_name="$1"
	local commit_sha="$2"
	[ -f "$MANIFEST_FILE" ] || die "Could not find $MANIFEST_FILE"
	log "Updating $MANIFEST_FILE with tag: $tag_name and commit: $commit_sha..."
	tag_name="$tag_name" commit_sha="$commit_sha" yq eval -i '(.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "git")) 
		|= (
			.tag = strenv(tag_name) |
			.tag line_comment = "CODE-GENERATED: Do not manually change. Use ./update.sh instead" |
			.commit = strenv(commit_sha) |
			.commit line_comment = "CODE-GENERATED: Do not manually change. Use ./update.sh instead"
		)' "$MANIFEST_FILE" && success || die "Failed to update $MANIFEST_FILE using yq."
}

# This function generates a new generated-sources.json file from the downloaded and extracted yarn.lock file.
# using the flatpak-node-generator tool.
update_generated_sources() {
	local yarn_lock_file="$tmpDownloadDir/yarn.lock"
	[ -f "$yarn_lock_file" ] || die "Yarn lock file not found: $yarn_lock_file"
	log "Regenerating generated-sources.json from $yarn_lock_file..."
	flatpak-node-generator --electron-node-headers yarn "$yarn_lock_file" && success || die "Failed to generate generated-sources.json from $yarn_lock_file"
}

main() {
	local commit_sha
	local tag_name
	local version_ref="latest"

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

	cd "$(dirname "${BASH_SOURCE[0]}")"
	setup_flathub_builder
	download_and_extract_selected_app_version tag_name commit_sha
	update_flatpak_manifest "$tag_name" "$commit_sha"
	update_generated_sources
	validate_flatpak_manifest "$MANIFEST_FILE"
	completed
}

main "$@"
