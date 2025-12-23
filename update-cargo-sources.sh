#!/usr/bin/env bash
#
# Script to update cargo sources for several modules in a Flatpak manifest.
#
# Dependencies: jq, git, python3, python3-toml and python3-aiohttp.

# The manifest file.
MANIFEST="cx.modal.Reflection.json"
# The names of the modules to update.
MODULES=("reflection")
# The directory where the sources are located.
SOURCES_DIR="ci_sources"
# The repository of the Flatpak Builder Tools.
FLATPAK_BUILDER_TOOLS_REPO="https://github.com/flatpak/flatpak-builder-tools.git"
# The directory where the Flatpak Builder Tools are cloned.
FLATPAK_BUILDER_TOOLS_DIR="$SOURCES_DIR/flatpak-builder-tools"

# Update the cargo sources for the given module in the manifest.
#
# Argument: the name of the module in the manifest.
update_module_cargo_sources() {
    MODULE_NAME="$1"

    if [[ -z "$MODULE_NAME" ]]; then
        echo "Name of the module to update is missing"
        exit 1
    fi

    # Get the module repository variables from the manifest.
    MODULE_SOURCE=$(cat $MANIFEST | jq --arg module_name "$MODULE_NAME" -r '.modules[] | select(.name == $module_name) | .sources[0]')
    MODULE_REPO=$(echo $MODULE_SOURCE | jq -r '.url')
    MODULE_TAG=$(echo $MODULE_SOURCE | jq -r '.tag')
    MODULE_COMMIT=$(echo $MODULE_SOURCE | jq -r '.commit')

    # Clone the module repository at the given tag.
    MODULE_SOURCE_DIR="$SOURCES_DIR/$MODULE_NAME"
    git clone -b "$MODULE_TAG" --depth 1 "$MODULE_REPO" "$MODULE_SOURCE_DIR"

    # Check that the tag commit matches the one in the Flatpak manifest.
    cd "$MODULE_SOURCE_DIR"
    TAG_COMMIT=$(git rev-parse HEAD)
    if [[ "$TAG_COMMIT" != "$MODULE_COMMIT" ]]; then
        echo "Commit of tag $MODULE_TAG does not match Flatpak manifest commit: $TAG_COMMIT"
        exit 1
    else
        echo "Commit of tag $MODULE_TAG matches Flatpak manifest commit: $TAG_COMMIT"
    fi

    # Return to the original directory.
    cd ../..

    # Update the cargo sources.
    python3 "./$FLATPAK_BUILDER_TOOLS_DIR/cargo/flatpak-cargo-generator.py" "./$MODULE_SOURCE_DIR/Cargo.lock" -o "$MODULE_NAME-cargo-sources.json"
}

# Clone the Flatpak Builder Tools.
git clone --depth 1 "$FLATPAK_BUILDER_TOOLS_REPO" "$FLATPAK_BUILDER_TOOLS_DIR"

# Update the cargo sources for all the modules.
for MODULE_NAME in "${MODULES[@]}"; do
    echo "Updating module $MODULE_NAME…"

    update_module_cargo_sources "$MODULE_NAME"

    # Queue the updated cargo sources file for committing.
    git add "$MODULE_NAME-cargo-sources.json"

    echo ""
    echo "Module $MODULE_NAME successfully updated!"
    echo ""
done
