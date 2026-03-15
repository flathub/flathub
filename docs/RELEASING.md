# Releasing OpenKara

## Release Flow

1. **Trigger**: Go to GitHub Actions → **Release** workflow → **Run workflow**
2. **Input**: Enter the version number (e.g. `0.1.0`) — do not include the `v` prefix
3. **Build**: CI builds for all 4 platforms (macOS ARM64, macOS x64, Windows, Linux)
4. **Publish**: GitHub Release is created automatically with DMG, NSIS installer, and AppImage
5. **Homebrew**: The tap repo (`thedavidweng/homebrew-tap`) auto-detects the new release within 6 hours and updates the cask

## Manual Homebrew Update

If you don't want to wait for the scheduled check:

1. Go to `thedavidweng/homebrew-tap` → Actions → **Auto-update OpenKara cask**
2. Click **Run workflow**

## Verification

```bash
brew update
brew install --cask thedavidweng/tap/openkara
```

## Architecture

OpenKara's release workflow is **decoupled from distribution**:

- **OpenKara repo** — only builds and publishes GitHub Releases
- **Homebrew tap repo** — independently polls for new releases and updates the cask
- No cross-repo secrets needed; each repo manages its own automation

## Future Distribution Channels

### Windows

- **winget**: Automate with `wingetcreate` in a GitHub Action to submit manifests to `microsoft/winget-pkgs`. Deferred until Windows builds are validated end-to-end.
- **Scoop**: Create `thedavidweng/scoop-bucket` with the same self-polling pattern as the Homebrew tap. Simpler than winget.

### Linux

- **Flatpak**: Submit a Flathub manifest. Complex due to sandbox permissions (audio device access, filesystem). Deferred.
- **AUR**: Write a PKGBUILD. Community can help maintain. Deferred.
- **Snap**: Write a snapcraft.yaml. Deferred.
