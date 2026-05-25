#!/bin/bash

set -e # Exit on any error

if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    exit 1
fi

VERSION="$1"
MANIFEST="org.praat.Praat.yml"
METAINFO="org.praat.Praat.metainfo.xml"

echo "Fetching release data for v$VERSION..."
DATA=$(gh release view "v$VERSION" --json name,url,body --repo praat/praat.github.io 2>/dev/null || true)

if [ -z "$DATA" ]; then
    echo "Error: Release v$VERSION not found."
    exit 1
fi

# Get the actual commit hash of the tag, since the `gh release view` just gives `master`
echo "Fetching exact commit hash for tag v$VERSION..."
COMMIT=$(gh api repos/praat/praat.github.io/git/ref/tags/v"$VERSION" --jq '.object.sha' 2>/dev/null)

# Handle annotated tag redirection if necessary
TAG_TYPE=$(gh api repos/praat/praat.github.io/git/ref/tags/v"$VERSION" --jq '.object.type' 2>/dev/null)
if [ "$TAG_TYPE" = "tag" ]; then
    COMMIT=$(gh api repos/praat/praat.github.io/git/tags/"$COMMIT" --jq '.object.sha' 2>/dev/null)
fi

RAW_DATE=$(echo "$DATA" | jq -r '.name' | sed 's/^[^,]*, //')
FORMATTED_DATE=$(date -d "$RAW_DATE" "+%Y-%m-%d")

echo "Found commit: $COMMIT"
echo "Found date:   $FORMATTED_DATE"

echo "Updating $MANIFEST..."
sed -i "/name: praat/,/commit:/ s/commit: .*/commit: $COMMIT/" "$MANIFEST"

# convert the Github changelog to AppStream xml
XML_FRAGMENT=$(echo "$DATA" | jq -r --arg ver "$VERSION" --arg date "$FORMATTED_DATE" '
    def strip_links: gsub("\\[(?<txt>[^\\]]+)\\]\\([^\\)]+\\)"; "\(.txt)");
    def clean_body: .body 
        | gsub("\r"; "") 
        | gsub("&"; "&amp;") 
        | gsub("<"; "&lt;") 
        | gsub(">"; "&gt;")
        | strip_links
        | split("\n") 
        | map(select(length > 0) | "          <li>" + sub("^ *[-*•] "; "") + "</li>") 
        | join("\n");

    "    <release version=\"" + $ver + "\" date=\"" + $date + "\">\n      <url type=\"details\">" + .url + "</url>\n      <description>\n        <ul>\n" + clean_body + "\n        </ul>\n      </description>\n    </release>"
')

echo "Updating $METAINFO..."
echo "$XML_FRAGMENT" > .release.tmp

# Check if version already exists in the metainfo file to prevent duplicates
if grep -q "<release version=\"$VERSION\"" "$METAINFO"; then
    echo "Version $VERSION already exists in $METAINFO. Skipping update."
else
    # This matches the line containing <releases> (allowing for leading spaces) 
    # and reads the temp file directly in right below it.
    sed -i "/^[[:space:]]*<releases>/r .release.tmp" "$METAINFO"
fi

rm .release.tmp
echo "Done! Manifest and Metainfo updated."

# Ask user if they want to attempt the build
read -p "Do you want to attempt building the Flatpak now? (y/n) " -n 1 -r
echo    # move to a new line
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Attempting to build the Flatpak..."
    flatpak-builder --force-clean --user --install-deps-from=flathub --repo=system --install builddir org.praat.Praat.yml
else
    echo "Build skipped. You can run the build command manually when you're ready."
fi

# Ask user if they want to run the Praat test suite
read -p "Do you want to run the Praat test suite now? (y/n) " -n 1 -r
echo    # move to a new line
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Running the Praat test suite..."
    # if the test suite isn't already cloned, clone it from https://github.com/praat/praat.github.io
    if [ ! -d ".tests" ]; then
        echo "Cloning the Praat test suite repository..."
        # clone only the `test` directory to save time and space
        git clone --depth 1 --filter=blob:none --sparse https://github.com/praat/praat.github.io.git .tests
        cd .tests
        git sparse-checkout set test
        cd ..
    else
        echo "Test suite already cloned. Pulling latest changes..."
        cd .tests
        git pull origin master
        cd ..
    fi
    # run the test suite using the flatpak
    flatpak run org.praat.Praat --run .tests/test/runAllTests_batch.praat
else
    echo "Test suite skipped. You can run the tests manually when you're ready."
fi
