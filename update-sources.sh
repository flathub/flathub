#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"

cd "${REPO_DIR}"

bunx flatpak-bun-generator raffi-desktop/bun.lock --output raffi-desktop/flatpak/bun-sources.json
go run github.com/dennwc/flatpak-go-mod@latest -json -out raffi-desktop/flatpak -dest-pref raffi-server/ ./raffi-server
mv raffi-desktop/flatpak/go.mod.json raffi-desktop/flatpak/go-sources.json
