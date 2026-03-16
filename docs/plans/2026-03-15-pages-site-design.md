# GitHub Pages Microsite Design

## Goal

Create a public-facing GitHub Pages site for OpenKara without coupling it to the Tauri app build or cluttering the internal documentation structure.

## Chosen Approach

- Keep the public site as plain static HTML and CSS under `docs/site/`.
- Move engineering docs into `docs/internal/` so the repo distinguishes public-facing pages from contributor-facing references.
- Leave `docs/contracts/` and `docs/plans/` in place because they already express stable contracts and work history clearly.
- Deploy `docs/site/` with a dedicated GitHub Pages workflow so the desktop app CI and release flows stay untouched.

## Design Direction

- Tone: refined, minimal, local-first, calm rather than flashy.
- Visual style: editorial serif headings, warm paper background, restrained accent color, light gradients, no heavy JavaScript.
- Content scope: landing page, install guide, and FAQ, each linking back to deeper GitHub-hosted docs.

## Success Criteria

- Public site files are isolated from app source code.
- Existing internal docs remain easy to find after the move.
- GitHub Pages can deploy from a single workflow without affecting release automation.
