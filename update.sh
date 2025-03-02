#!/bin/bash

set -e

# Run flatpak-external-data-checker and capture the output
output=$(flatpak run org.flathub.flatpak-external-data-checker ./io.github.mlm_games.pomodorot.yml)

# Check if the output contains the "OUTDATED" message
if echo "$output" | grep -q "OUTDATED: pomodorot"; then


    # Extract the new version from the output
    new_version=$(echo "$output" | grep -oP 'Version:\s+\K\d+\.\d+\.\d+' | head -n 1)
    echo "New version:" $new_version

    # Extract the new SHA256 checksums from the output
    new_sha256_x86_64=$(echo "$output" | grep -oP 'SHA256:\s+\K\w+' | head -n 1)
    new_sha256_aarch64=$(echo "$output" | grep -oP 'SHA256:\s+\K\w+' | tail -n 1)
    echo "New sha for x86_64:" $new_sha256_x86_64
    echo "New sha for arm64:" $new_sha256_aarch64

    # Update the version in the URLs
    sed -i "s|\(url: https://github.com/mlm-games/pomodorot/releases/download/\)\([0-9]\+\.[0-9]\+\.[0-9]\+\)\?|\1$new_version|g" io.github.mlm_games.pomodorot.yml

    # Update the SHA256 for x86_64
    sed -i "/.x86_64/,/- x86_64/{ /sha256:/s/sha256: [a-f0-9]\+/sha256: $new_sha256_x86_64/ }" io.github.mlm_games.pomodorot.yml

    # Update the SHA256 for aarch64
    sed -i "/.arm64/,/- aarch64/{ /sha256:/s/sha256: [a-f0-9]\+/sha256: $new_sha256_aarch64/ }" io.github.mlm_games.pomodorot.yml

    # Get the current date in YYYY-MM-DD format
    current_date=$(date +%Y-%m-%d)

    # Update the release tag in io.github.mlm_games.pomodorot.metainfo.xml
    sed -i "/<releases>/a \    <release version=\"$new_version\" date=\"$current_date\">\\n      <url type="details">https://github.com/mlm-games/pomodorot/releases</url>\\n    </release>" io.github.mlm_games.pomodorot.metainfo.xml

    git checkout -B $new_version
    git add .
    git commit -m "New branch for $new_version"
    git push -u origin $new_version
    git push
    gh pr create --base master --head $new_version --title "New update: v$new_version" --body "New update to v$new_version"
    
    echo "io.github.mlm_games.pomodorot.yml updated with new version $new_version and SHA256 checksums, merge branch to integrate changes."

else
    echo "No new update available."
fi
