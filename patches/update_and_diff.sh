#!/bin/bash

# Function to log messages
log() {
  echo "$1"
}

# Function to fetch checksum from crates.io
fetch_checksum() {
  local crate=$1
  local version=$2
  response=$(curl -sSf "https://crates.io/api/v1/crates/$crate/$version")
  if [[ $? -ne 0 ]]; then
    log "Error fetching checksum for $crate@$version"
    echo ""
    return 1
  fi
  echo "$response" | jq -r '.version.checksum'
}

# Function to extract version from Cargo.lock
extract_version_from_lock() {
  local crate=$1
  version=$(grep -Pzo "(?s)\[\[package\]\]\s*name = \"$crate\"\s*version = \"([^\"]+)\"" "$CARGO_LOCK_PATH" | grep -Po 'version = "\K[^\"]+')
  if [[ -z "$version" ]]; then
    log "Retrying to extract version for $crate using an alternative method"
    version=$(grep -A 1 "\[\[package\]\]" "$CARGO_LOCK_PATH" | grep -A 1 "name = \"$crate\"" | grep -Po 'version = "\K[^\"]+')
  fi
  echo "$version"
}

# Function to update Cargo.toml dependencies
update_cargo_toml() {
  local toml_file=$1

  # Read the Cargo.toml file and process it
  while IFS= read -r line; do
    if [[ $line =~ git\ =\ \"https:// ]]; then
      name=$(echo "$line" | cut -d' ' -f1)
      crate=$(echo "$name" | cut -d'=' -f1 | tr -d ' ')
      version=$(extract_version_from_lock "$crate")
      if [ -n "$version" ]; then
        updated_line="${name} = \"${version}\""
        sed -i "s|$line|$updated_line|" "$toml_file"
      else
        log "Warning: Version not found for $crate in Cargo.lock"
      fi
    fi
  done < "$toml_file"
}

# Check if the arguments are provided
if [ $# -ne 2 ]; then
  log "Usage: $0 <zed-repo-path> <output-path>"
  exit 1
fi

ZED_REPO_PATH="$1"
OUTPUT_PATH="$2"
CARGO_LOCK_PATH="${ZED_REPO_PATH}/Cargo.lock"
CARGO_TOML_PATH="${ZED_REPO_PATH}/Cargo.toml"
PATCHES_DIR="${OUTPUT_PATH}/patches"

# Ensure the paths exist
if [ ! -f "$CARGO_LOCK_PATH" ] || [ ! -f "$CARGO_TOML_PATH" ]; then
  log "Error: Cargo.lock or Cargo.toml not found in the specified path."
  exit 1
fi

# Ensure the patches directory exists
mkdir -p "$PATCHES_DIR"

# Temporary files
TEMP_LOCK="${CARGO_LOCK_PATH}.tmp"
TEMP_CHECKSUM="checksum.tmp"

# Ensure jq is installed
if ! command -v jq &> /dev/null; then
  log "jq could not be found, please install it."
  exit 1
fi

# Create a temporary branch for generating diffs
cd "$ZED_REPO_PATH"
git checkout -b temp-diff-branch

# Read the entire Cargo.lock file into a variable
cargo_lock_content=$(cat "$CARGO_LOCK_PATH")

# Process the Cargo.lock content
while IFS= read -r line; do
  # Check if the line contains a git source
  if [[ $line =~ ^source.*git\+ ]]; then
    # Extract the crate name and version from the previous lines
    name=$(echo "$prev_name_line" | cut -d' ' -f3 | tr -d '"')
    version=$(echo "$prev_version_line" | cut -d' ' -f3 | tr -d '"')
    checksum=$(fetch_checksum "$name" "$version")

    # Replace the source line with the registry source and checksum
    cargo_lock_content=$(echo "$cargo_lock_content" | sed "s|$line|source = \"registry+https://github.com/rust-lang/crates.io-index\"\nchecksum = \"$checksum\"|")
  fi

  # Save the current line as the previous line
  prev_line="$line"

  # Check if the previous line is the name or version line
  if [[ $prev_line =~ ^name.* ]]; then
    prev_name_line="$prev_line"
  elif [[ $prev_line =~ ^version.* ]]; then
    prev_version_line="$prev_line"
  fi
done <<< "$cargo_lock_content"

# Save current diff
diff -u "$CARGO_LOCK_PATH" <(echo "$cargo_lock_content") > "$PATCHES_DIR/Cargo.lock.diff"

# Replace the original Cargo.lock with the updated content
echo "$cargo_lock_content" > "$CARGO_LOCK_PATH"

log "Cargo.lock has been updated with registry sources and checksums."
log "Changes have been logged in $PATCHES_DIR/Cargo.lock.diff."

# Update Cargo.toml
update_cargo_toml "$CARGO_TOML_PATH"

# Perform diffs and save to files
cd "$ZED_REPO_PATH"
git diff "$CARGO_TOML_PATH" > "$PATCHES_DIR/Cargo.toml.diff"

log "Diffs have been saved to $PATCHES_DIR/Cargo.lock.diff and $PATCHES_DIR/Cargo.toml.diff."

# Clean up temporary branch
git checkout main
git branch -D temp-diff-branch
