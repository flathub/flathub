# Flatpak / Flathub files

Copy the contents of this directory into a top-level `com.swiftbrowser.Swift/` directory in a fork of `github.com/flathub/flathub` and open a pull request.

| File | Purpose |
| --- | --- |
| `com.swiftbrowser.Swift.yml` | flatpak-builder manifest with per-arch release archives |
| `com.swiftbrowser.Swift.desktop` | desktop launcher |
| `com.swiftbrowser.Swift.metainfo.xml` | AppStream metadata for Flathub and software centers |
| `swift-browser-wrapper` | executable installed to `/app/bin/swift-browser` |
| `icon-128.png`, `icon-256.png` | hicolor icons |

Before submitting, replace the `SHA256_X64` and `SHA256_ARM64` placeholders in the manifest with real checksums from the GitHub release. See `../scripts/update-flatpak-sha.sh`.

Flathub reviewers normally expect at least one screenshot referenced from the PR. Generate screenshots from a running Swift window and add them to the same directory, then reference them from the metainfo with `<screenshots>` if desired.
