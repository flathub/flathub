# Homebrew Cask Packaging

This directory holds the scaffolding for shipping OpenKara through Homebrew.

Why cask instead of formula:

- OpenKara ships as a macOS desktop application bundle.
- Homebrew Formula is a poor fit for GUI app installation.
- Homebrew Cask is the supported path for distributing signed `.dmg` assets.

## Files

- `openkara.rb.template`: template for the tap repository cask file
- `auto-update-openkara.yml`: GitHub Actions workflow for the tap repo — auto-detects new OpenKara releases and updates the cask

## Automated Release Flow

The release is fully automated and decoupled:

1. **OpenKara repo**: Manually trigger the Release workflow → builds all platforms → publishes GitHub Release
2. **Tap repo** (`thedavidweng/homebrew-tap`): Runs `auto-update-openkara.yml` every 6 hours (or on manual trigger) → detects new release → downloads DMGs → computes SHA-256 → updates `Casks/openkara.rb` → commits and pushes

## Tap Repo Setup

To set up the tap repo for auto-updates:

1. Copy `auto-update-openkara.yml` to `thedavidweng/homebrew-tap/.github/workflows/`
2. Ensure GitHub Actions is enabled on the tap repo
3. The workflow uses the default `GITHUB_TOKEN` — no additional secrets needed
