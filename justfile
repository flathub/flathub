# Scripts to help automate the generation of cargo-sources.json, node-sources.json and yarn.lock.
# Simply run: just

manifest_file := 'net.lrclib.lrcget.yaml'
upstream_repo := 'https://github.com/tranxuanthang/lrcget'
cargo_sources := 'cargo-sources.json'
node_sources  := 'node-sources.json'
tools_repo    := 'https://github.com/flatpak/flatpak-builder-tools.git'
tools_dir     := '/tmp/flatpak-builder-tools'

# Generate Cargo and Node source files
all:
    just check-deps
    just generate-cargo-sources
    just generate-node-sources

# Verify that required dependencies are installed
check-deps:
    #!/usr/bin/env bash
    set -euo pipefail

    deps=('git' 'yq' 'poetry' 'curl' 'yarn')
    missing=()

    echo 'Checking dependencies...'

    for dep in "${deps[@]}"; do
        if ! command -v "$dep" >/dev/null 2>&1; then
            missing+=("$dep")
        fi
    done

    if [ ${#missing[@]} -gt 0 ]; then
        echo "Missing dependencies: ${missing[*]}"
        echo 'Please install them and try again.'
        exit 1
    fi

    echo 'All dependencies are available'


# Generate Cargo sources (cargo_sources.json)
generate-cargo-sources:
    #!/usr/bin/env bash
    set -euo pipefail

    # Setup flatpak-builder-tools if necessary
    if [ ! -d '{{tools_dir}}/.git' ]; then
        echo 'Cloning the flatpak-builder-tools git repo...'
        git clone --quiet --depth=1 '{{tools_repo}}' '{{tools_dir}}'
    fi

    commit_hash="$(yq '.modules[].sources[] | select(.url == "{{upstream_repo}}.git").commit' '{{manifest_file}}')"
    if [ -z "$commit_hash" ]; then
        echo 'Error: Could not extract commit hash from {{manifest_file}}'
        exit 1
    fi

    echo "Will generate cargo-sources.json for commit: $commit_hash"

    echo 'Installing dependencies for flatpak-cargo-generator...'
    poetry -C '{{tools_dir}}/cargo' install --quiet --without dev

    temp_cargo_lock="$(mktemp --suffix=_cargo.lock)"
    trap "rm -f '$temp_cargo_lock'" EXIT
    echo "Created temporary file for the Cargo.lock: $temp_cargo_lock"

    cargo_lock_url="{{upstream_repo}}/raw/$commit_hash/src-tauri/Cargo.lock"
    echo "Downloading Cargo.lock: $cargo_lock_url"
    curl -fsSL "$cargo_lock_url" -o "$temp_cargo_lock"

    echo "Running 'flatpak-cargo-generator'..."
    poetry -C '{{tools_dir}}/cargo' run ./flatpak-cargo-generator.py -o '{{justfile_directory()}}/{{cargo_sources}}' "$temp_cargo_lock"
    echo '{{cargo_sources}} generated successfully'

# Generate Node sources (node-sources.json and yarn.lock)
generate-node-sources:
    #!/usr/bin/env bash
    set -euo pipefail

    # Setup flatpak-builder-tools if necessary
    if [ ! -d '{{tools_dir}}/.git' ]; then
        echo 'Cloning the flatpak-builder-tools git repo...'
        git clone --quiet --depth=1 '{{tools_repo}}' '{{tools_dir}}'
    fi

    commit_hash="$(yq '.modules[].sources[] | select(.url == "{{upstream_repo}}.git").commit' '{{manifest_file}}')"
    if [ -z "$commit_hash" ]; then
        echo 'Error: Could not extract commit hash from {{manifest_file}}'
        exit 1
    fi

    echo "Will generate node-sources.json and yarn.lock for commit: $commit_hash"

    echo 'Installing dependencies for flatpak-node-generator...'
    poetry -C '{{tools_dir}}/node' install --quiet --without dev

    package_dir="$(mktemp -d --suffix=_package.json)"
    trap "rm -rf '$package_dir'" EXIT
    echo "Created temporary directory for the package.json: $package_dir"

    package_json_url="{{upstream_repo}}/raw/$commit_hash/package.json"
    echo "Downloading package.json: $package_json_url"
    curl -fsSL "$package_json_url" -o "$package_dir/package.json"

    echo "Running 'yarn install' to generate yarn.lock..."
    yarn install --cwd "$package_dir" --no-progress --silent 2> >(grep -v warning 1>&2)

    echo "Running 'flatpak-node-generator'..."
    poetry -C '{{tools_dir}}/node' run flatpak-node-generator -o '{{justfile_directory()}}/{{node_sources}}' yarn "$package_dir/yarn.lock"
    cp "$package_dir/yarn.lock" '{{justfile_directory()}}'

    echo '{{node_sources}} and yarn.lock generated successfully'
