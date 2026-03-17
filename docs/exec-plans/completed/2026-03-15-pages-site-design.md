# GitHub Pages Microsite Design

## Goal

Create a public-facing GitHub Pages site for OpenKara without coupling it to the Tauri app build or cluttering the internal documentation structure, while letting public FAQ content come from Markdown.

## Chosen Approach

- Use Jekyll under `docs/site/` so public content pages can be written in Markdown.
- Move engineering docs into `docs/internal/` so the repo distinguishes public-facing pages from contributor-facing references.
- Leave `docs/contracts/` and `docs/plans/` in place because they already express stable contracts and work history clearly.
- Keep the homepage visually custom through dedicated layouts and CSS instead of using a stock docs theme.
- Deploy the Jekyll build output from `docs/site/_site/` with a dedicated GitHub Pages workflow so the desktop app CI and release flows stay untouched.

## Design Direction

- Tone: refined, minimal, local-first, calm rather than flashy.
- Visual style: editorial serif headings, warm paper background, restrained accent color, light gradients, no heavy JavaScript.
- Content scope: a strong product-style homepage with install guidance folded into it, plus a Markdown-driven FAQ page.

## Success Criteria

- Public site files are isolated from app source code.
- Existing internal docs remain easy to find after the move.
- FAQ content can be edited in Markdown and published through the site.
- GitHub Pages can deploy from a single workflow without affecting release automation.
