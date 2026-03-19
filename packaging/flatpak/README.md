# Flatpak Packaging

This directory stores the upstream source of truth for the Flathub-ready
OpenKara packaging metadata.

Important boundaries:

- OpenKara keeps building GitHub Release binaries in this repository.
- Flathub must build the final Flatpak itself from source; this repository only
  prepares the manifest, metadata, screenshots, and dependency inputs.
- `scripts/render-flatpak-manifest.mjs` generates the release-specific
  submission bundle once a tag and public source archive exist.

Bootstrap requirements for PR automation:

- A fork of the target Flathub app repo
- Repo variable `FLATHUB_FORK_REPO`, for example
  `thedavidweng/io.github.thedavidweng.OpenKara`
- Optional repo variable `FLATHUB_TARGET_REPO` to override the upstream target
  (defaults to `flathub/io.github.thedavidweng.OpenKara`)
- Repo secret `FLATHUB_PR_TOKEN`
