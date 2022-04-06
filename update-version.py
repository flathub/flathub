#!/usr/bin/env python3
"""
- Checks Github API for newest version of Viper
- Downloads it for checksum calculation 
- Updates Flatpak manifest accordingly
"""
# %%

# Imports
# Only modules in the standard library were chosen to avoid needing to install Python dependencies
import hashlib
import re
import json
import urllib.request

# Pre-defined values
REPO_API_RELEASE_URL = "https://api.github.com/repos/0neGal/viper/releases/latest"
FLATPACK_MANIFEST_NAME = "com.github._0negal.Viper.yaml"


# Get JSON object containing info about all releases
api_response = urllib.request.urlopen(REPO_API_RELEASE_URL).read()

# Get version number of newest release
release_version = json.loads(api_response)["tag_name"].replace("v", "")
print(f"Newest version is: {release_version}")

# The part of the JSON object that refers to the AppImage
appimage_object = [x for x in json.loads(
    api_response)["assets"] if "AppImage" in x["name"]][0]


# Simple check to ensure that versions match up
assert(release_version in appimage_object["name"])

# Download AppImage file for checksum calculation
g = urllib.request.urlopen(appimage_object["browser_download_url"])
with open(appimage_object["name"], "bw") as f:
    f.write(g.read())

# Calculate SHA256 checksum to update in manifest
sha256_hash = hashlib.sha256()
with open(appimage_object["name"], "rb") as f:
    # Read and update hash string value in blocks of 4K
    for byte_block in iter(lambda: f.read(4096), b""):
        sha256_hash.update(byte_block)
    release_checksum = sha256_hash.hexdigest()


# Read Flatpak manifest
with open(FLATPACK_MANIFEST_NAME, "rt") as f:
    file_content = f.read()

# Set version number, size, and checksum
file_content = re.sub(r"(\d+\.\d+\.\d+)", release_version, file_content)
file_content = re.sub(r"size: \d+", f"size: {appimage_object['size']}", file_content, 0, re.MULTILINE)
file_content = re.sub(r"sha256: [0-9a-fA-F]+", f"sha256: {release_checksum}", file_content, 0, re.MULTILINE)

# Write back updated content
with open(FLATPACK_MANIFEST_NAME, "wt") as f:
    f.write(file_content)

print("Done")
print("Don't forget to manually update the AppData XML (com.github._0negal.Viper.appdata.xml) !")
