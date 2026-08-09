manifest := "io.github.idescriptor.iDescriptor.yml"
cargo_generator := "https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/f03a673abe6ce189cea1c2857e2b44af2dd79d1f/cargo/flatpak-cargo-generator.py"
upstream_url := "https://github.com/iDescriptor/iDescriptor.git"

default:
    @just --list

sources commit:
    #!/usr/bin/env bash
    set -euo pipefail

    commit={{quote(commit)}}
    if [[ ! "${commit}" =~ ^[0-9a-f]{40}$ ]]; then
        echo "commit must be a full 40-character Git commit" >&2
        exit 2
    fi

    repo_dir="$(pwd)"
    source_dir="$(mktemp -d /tmp/idescriptor-sources.XXXXXXXX)"
    output="${repo_dir}/cargo-sources.json.tmp"
    trap 'rm -rf "${source_dir}"; rm -f "${output}"' EXIT

    git -C "${source_dir}" init -q
    git -C "${source_dir}" remote add origin "{{upstream_url}}"
    git -C "${source_dir}" fetch --depth=1 origin "${commit}"
    git -C "${source_dir}" checkout -q --detach FETCH_HEAD

    UV_CACHE_DIR=/tmp/idescriptor-flatpak-uv \
        XDG_CACHE_HOME="${source_dir}/.cache" \
        uv run --with aiohttp --with tomlkit --with pyyaml \
        "{{cargo_generator}}" "${source_dir}/Cargo.lock" -o "${output}"

    mv "${output}" "${repo_dir}/cargo-sources.json"

#build:
#    flatpak run --command=flathub-build org.flatpak.Builder --install "{{manifest}}"
build:
    flatpak-builder \
      --user \
      --install \
      --force-clean \
      --install-deps-from=flathub \
      build-dir \
      io.github.idescriptor.iDescriptor.yml


lint-manifest:
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest "{{manifest}}"

clean:
    rm -rf .flatpak-builder build-dir repo

uninstall:
    flatpak uninstall io.github.idescriptor.iDescriptor

run:
    flatpak run io.github.idescriptor.iDescriptor

push:
    git push --set-upstream origin HEAD
