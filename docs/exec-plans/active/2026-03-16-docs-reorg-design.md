# Documentation Reorganization Design

## Goal

Reorganize OpenKara's documentation to match the requested top-level structure, remove stale documentation drift, and clean low-risk dead files and code without moving application source directories.

## Chosen Approach

- Keep application code under the existing `src/` and `src-tauri/` layout.
- Normalize only the documentation tree so `docs/` has one clear information architecture.
- Add a root `ARCHITECTURE.md` entry point, but keep the detailed architecture source of truth under `docs/design-docs/`.
- Prefer move-and-rename over duplication so the repository ends with one canonical home per document set.
- Keep `docs/site/` intact because it is part of the public GitHub Pages surface.

## Target Structure

```text
AGENTS.md
ARCHITECTURE.md
docs/
├── design-docs/
│   ├── index.md
│   ├── core-beliefs.md
│   └── ...
├── exec-plans/
│   ├── active/
│   ├── completed/
│   └── tech-debt-tracker.md
├── generated/
│   └── db-schema.md
├── product-specs/
│   ├── index.md
│   ├── new-user-onboarding.md
│   └── ...
├── references/
│   └── contracts/
└── site/
```

## Consistency Fixes

- Update the architecture docs so the lyrics flow matches the Rust code: `LRCLIB -> embedded tags -> sidecar .lrc`, with manual import and edit flows handled separately.
- Update caching docs to reflect the current OGG/Vorbis stem cache instead of the older WAV description.
- Refresh the project-structure doc so it reflects the current frontend, backend, and documentation directories.
- Repair inbound links from `README.md`, `README_CN.md`, `docs/site/faq.md`, `docs/site/index.html`, and moved reference docs.

## Cleanup Scope

- Delete tracked `.DS_Store` files from the repository.
- Remove clearly unused code only when repository-wide search confirms no callers. The current low-risk candidate is `src/lib/cover-art.ts`.
- Normalize naming so `design-docs`, `exec-plans`, `references`, and `product-specs` are the only canonical labels.

## Success Criteria

- The requested documentation categories exist under `docs/`.
- The root `ARCHITECTURE.md` entry point exists.
- Architecture and project-structure docs match the actual implementation.
- Old doc links are repaired.
- Low-risk dead files and dead code are removed.
