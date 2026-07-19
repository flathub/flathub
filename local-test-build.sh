#!/bin/bash
set -euo pipefail

# THIS IS FOR TESTING PURPOSES ONLY. DO NOT USE THIS SCRIPT IN PRODUCTION!

BUNDLE_FLATPAK_SINGLE_FILE=${BUNDLE_FLATPAK_SINGLE_FILE:-0}

source "$(dirname ${BASH_SOURCE[0]})/common.sh"

build_and_install_flatpak() {
	local manifest_file="$1"

	if [ "$BUNDLE_FLATPAK_SINGLE_FILE" -eq 1 ]; then
		log "Building Bazecor Flatpak..."
		flatpak run --command=flathub-build org.flatpak.Builder --force-clean "${manifest_file}" && success || die "Failed to build Bazecor Flatpak. Please check the output for errors."

		log "Bundling Bazecor Flatpak. This can take awhile, please be patient..."
		flatpak build-bundle repo com.dygma.bazecor.flatpak com.dygma.bazecor && success || die "Failed to bundle Bazecor Flatpak. Please check the output for errors."

		log "Installing Bazecor flatpak..."
		flatpak install --user -y com.dygma.bazecor.flatpak && success || die "Failed to install Bazecor Flatpak. Please check the output for errors."
	else
		log "Building and Installing Bazecor Flatpak..."
		flatpak run --command=flathub-build org.flatpak.Builder --force-clean --install "${manifest_file}" && success || die "Failed to build and install Bazecor Flatpak from the local repo. Please check the output for errors."
	fi
}

main() {
	cd "$(dirname "${BASH_SOURCE[0]}")"
	require_cmd flatpak
	setup_flathub_builder
	validate_flatpak_manifest "$MANIFEST_FILE"
	build_and_install_flatpak "$MANIFEST_FILE"
	validate_flatpak_repo
	completed
}

main "$@"
