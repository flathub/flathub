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

- A fork of the Flathub submission repo while OpenKara is not yet published:
  `thedavidweng/io.github.thedavidweng.OpenKara` is a fork of
  `flathub/flathub`
- Repo variable `FLATHUB_FORK_REPO`, for example
  `thedavidweng/io.github.thedavidweng.OpenKara`
- Optional repo variable `FLATHUB_TARGET_REPO` to override the upstream target
  (defaults to `flathub/flathub`)
- Optional repo variable `FLATHUB_BASE_BRANCH` to override the upstream branch
  (defaults to `new-pr`)
- Repo secret `FLATHUB_PR_TOKEN`

Initial submission boundary:

- OpenKara is not published on Flathub yet, so the release workflow prepares
  and pushes a submission branch, then prints a GitHub web compare URL with the
  Flathub submission PR title and notes already filled in.
- The actual Flathub submission PR must be opened manually against
  `flathub/flathub:new-pr` with title `Add io.github.thedavidweng.OpenKara`.
- Review the prefilled official Flathub submission notes before opening the PR,
  and keep GitHub Copilot automatic review disabled before creating it.

After Flathub creates `flathub/io.github.thedavidweng.OpenKara`, point
`FLATHUB_TARGET_REPO` at that app repository and `FLATHUB_BASE_BRANCH` at
`master`; update PRs can then be created automatically by the release workflow.
