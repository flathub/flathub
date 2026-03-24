# WinGet Packaging

This directory stores the repo-local source of truth for OpenKara's WinGet
submission metadata and automation.

Why templates instead of checked-in release manifests:

- WinGet installer URLs and SHA256 values must point to a real GitHub Release.
- OpenKara's next public release is `0.4.0`, so the final manifests can only be
  materialized once the release assets exist.
- `scripts/render-winget-manifests.mjs` fetches the release metadata from GitHub
  and emits the versioned `manifests/<first-letter>/<publisher>/<package>/<version>/`
  tree used for PRs against `microsoft/winget-pkgs`.

Bootstrap requirements for PR automation:

- A fork of `microsoft/winget-pkgs`
- A token with push access to that fork
- Repo variable `WINGET_FORK_REPO`, for example `thedavidweng/winget-pkgs`
- Repo secret `WINGET_PR_TOKEN`

The release workflow always generates WinGet manifest artifacts. If the fork and
token are configured, it also opens or updates the external PR automatically.
