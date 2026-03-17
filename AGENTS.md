# OpenKara Agent Notes

These are project-specific rules for any coding agent working in this repository.

## Mandatory Verification Before Completion

- Before claiming work is complete, before creating a commit, and before pushing, run `pnpm format` from the repository root.
- If `pnpm format` fails, fix the reported files first. Do not leave formatting failures for CI to discover.
- Treat formatting verification as required even for documentation-only or config-only changes.

## Common Formatting Hotspots

- `pnpm-lock.yaml`
- `docs/site/**/*.html`
- `docs/site/**/*.css`
- `src-tauri/tauri.conf.json`
- `.github/workflows/*.yml`
- Any Markdown, JSON, HTML, CSS, or YAML file touched in the change

These files are easy to forget because they often change during docs, config, or dependency work without any TypeScript/Rust compilation signal.

## Verification By Change Type

- Frontend/UI changes: run `pnpm format`, then `pnpm lint`, then `pnpm build`
- Rust/backend changes under `src-tauri/`: run `pnpm format`, then `cargo test` from `src-tauri`
- Workflow/config/docs-only changes: run `pnpm format` at minimum, even if no code changed

## Rationale Comments For Non-Obvious Decisions

- Preserve comments that explain why a piece of code exists, not just what it does.
- When touching product tradeoffs, portability rules, or performance/storage constraints, add or update a short rationale comment near the relevant code.
- This is required for decisions that a future agent could otherwise "simplify" in the wrong direction.
- Examples in this repo include: storing cached stems as OGG instead of WAV to keep library size manageable, storing library paths as relative forward-slash paths for portability, and keeping model bootstrap logic variant-aware instead of treating any existing file as ready.
- Do not add narration comments to ordinary code paths; focus on the decisions most likely to be misunderstood and accidentally reverted.

## GitHub Actions And Linux Build Notes

- Keep `actions/checkout` and `actions/setup-node` on current supported majors; this repository has already hit GitHub Actions runtime deprecation warnings.
- When editing `.github/workflows/ci.yml` or `.github/workflows/release.yml`, preserve Linux native packages required by Tauri and audio builds.
- In particular, do not remove `libasound2-dev` from Linux workflow setup unless the audio stack changes. OpenKara depends on `alsa-sys` via `cpal`, and Linux CI fails without the ALSA development package.

## CI Triage Hint

- If all `Verify` matrix jobs fail quickly on Linux, macOS, and Windows, first check whether they all died at the same step.
- If that shared step is `Verify formatting`, assume a repo-wide Prettier issue before debugging platform-specific code.

## Completion Checklist

- Run `pnpm format`
- If code changed, run the relevant local verification commands for that area
- Only then report completion, commit, or push

## Why This Exists

Recent CI failures in this repository were caused by agents forgetting to run Prettier checks before finishing work. The local formatting check is fast and must be used as the final gate.
