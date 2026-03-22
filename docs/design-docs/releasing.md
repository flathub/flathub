# Releasing OpenKara

## Release Flow

1. **Trigger**: Go to GitHub Actions → **Release** workflow → **Run workflow**
2. **Input**: Enter the version number (e.g. `0.3.0`) — do not include the `v` prefix
3. **Build**: CI builds for all 4 platforms (macOS ARM64, macOS x64, Windows, Linux)
4. **Publish**: GitHub Release is created automatically with DMG, NSIS installer, and AppImage
5. **Homebrew**: The tap repo (`thedavidweng/homebrew-tap`) polls for new releases once per day and updates the cask when it detects a new version. Expect up to about 24 hours before the scheduled sync picks it up.

## Manual Homebrew Update

If you don't want to wait for the scheduled check:

1. Go to `thedavidweng/homebrew-tap` → Actions → **Sync Releases**
2. Click **Run workflow**

## Verification

```bash
brew update
brew install --cask thedavidweng/tap/openkara
```

## Architecture

OpenKara's release workflow is **decoupled from distribution**:

- **OpenKara repo** — only builds and publishes GitHub Releases
- **Homebrew tap repo** — independently polls daily for new releases and updates the cask
- No cross-repo secrets needed; each repo manages its own automation

## Automated Distribution Manifests

GitHub Releases remain the only place where OpenKara binaries are built.

Additional distribution channels are derived from those releases:

- **WinGet** — release automation renders versioned manifests and, when
  `WINGET_FORK_REPO` and `WINGET_PR_TOKEN` are configured, opens or updates a PR
  against `microsoft/winget-pkgs`. If the token can push the fork branch but
  cannot create the upstream PR, the workflow now prints a compare URL and
  keeps the release run green.
- **Flathub** — release automation renders the source-build Flatpak bundle and,
  when `FLATHUB_FORK_REPO` and `FLATHUB_PR_TOKEN` are configured, opens or
  updates a PR against the target Flathub app repository.

Repo-local source of truth:

- `packaging/winget/`
- `packaging/flatpak/`

Repo-local validation:

- `.github/workflows/packaging.yml` validates that the manifest generators still
  produce syntactically correct WinGet and Flatpak metadata from the latest
  public release.

## Future Distribution Channels

### Windows

- **winget**: Automated via release workflow once the external fork/token
  bootstrap is configured.
- **Scoop**: Create `thedavidweng/scoop-bucket` with the same self-polling pattern as the Homebrew tap. Simpler than winget.

### Linux

- **Flatpak**: Source-build Flathub-ready manifest is maintained in-repo and can
  be submitted automatically after bootstrap. Final Flatpak binaries are still
  built by Flathub, not by GitHub Actions.
- **AUR**: Write a PKGBUILD. Community can help maintain. Deferred.
- **Snap**: Write a snapcraft.yaml. Deferred.
