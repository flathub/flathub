# OpenKara Agent Notes

This file is the single agent instruction entrypoint for this repository. It is
for coding agents only. Keep it self-contained: do not assume the next agent
will cross-reference README, workflows, or contracts before acting.

## Project Runbook

Use these commands for the normal local workflow:

- Install dependencies: `pnpm install`
- Prewarm local development/test models: `./scripts/setup.sh`
- Start the desktop app in development: `pnpm tauri dev`
- Build a production bundle: `pnpm tauri build`

Model path boundary:

- `src-tauri/models/` is only a local development and deterministic-test cache.
- End-user installs use the app data directory for runtime model downloads.
- Do not treat `src-tauri/models/` as a required runtime dependency for shipped builds.

## Build / Test / Check Commands

Choose verification by the highest-risk area touched.

| Change type                              | Required commands                                                                                               |
| ---------------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| Docs/config only                         | `pnpm format`                                                                                                   |
| Frontend/UI                              | `pnpm format` → `pnpm lint` → `pnpm build` → `pnpm test`                                                        |
| Rust/backend under `src-tauri/`          | `pnpm format` → `cd src-tauri && cargo test -q`                                                                 |
| Release-sensitive or cross-stack changes | `pnpm format` → `pnpm lint` → `pnpm build` → `pnpm test` → `cd src-tauri && cargo test -q` → `pnpm tauri build` |

Always escalate to full verification for any of the following:

- Version changes
- `src-tauri/tauri.conf.json` edits
- Packaging or release workflow changes
- Cross-frontend/backend refactors
- Model bootstrap changes
- Playback, separation, lyrics-sync, or other core media-path changes

## Engineering Rules

- Preserve comments that explain why a piece of code exists, not just what it does.
- When touching product tradeoffs, portability rules, or storage/performance constraints, add or update a short rationale comment near the code.
- Do not add narration comments to ordinary code paths.
- Keep code, contracts, and docs aligned. If behavior changes, update the relevant `docs/contracts/*.md` or internal docs in the same change.
- Treat repo-tracked docs and configs as first-class code: validate them locally instead of deferring discovery to CI.

Examples of rationale comments worth preserving in this repo:

- Cached stems are stored as OGG instead of WAV to keep library size manageable.
- Library paths are stored as relative forward-slash paths for portability.
- Model bootstrap is variant-aware; an arbitrary existing file is not enough to mark the model as ready.

## PR / Delivery Requirements

Before preparing a commit, push, or handoff:

- State the scope of the change clearly: what behavior or subsystem changed.
- Report the exact verification commands you ran.
- If you ran only a subset of tests, say exactly what you did not run.
- Do not skip local verification and leave first discovery of breakage to CI.
- Do not revert, reformat, or reorganize unrelated dirty changes you did not make.
- Keep documentation/config/contract updates in the same change when implementation meaning moved.

Delivery reports must include:

- What changed
- What commands were run
- What commands were not run
- Any known residual risk, limitation, or missing manual smoke coverage

## Constraints / Never Do

- Never treat `pnpm format` as optional.
- Never claim work is complete if the required verification for that change type was not run in this session.
- Never remove Linux CI package `libasound2-dev` unless the audio stack itself changes.
- Never repurpose `src-tauri/models/` as the production runtime model location.
- Never change public IPC commands, payloads, or events without updating the corresponding contract docs.
- Never remove rationale comments just because the surrounding code was refactored.
- Never leave formatting failures for CI to catch later.

Common formatting hotspots that are easy to miss:

- `pnpm-lock.yaml`
- `website/**/*.html`
- `website/**/*.css`
- `src-tauri/tauri.conf.json`
- `.github/workflows/*.yml`
- Any Markdown, JSON, HTML, CSS, or YAML file touched in the change

GitHub Actions and Linux build constraints:

- Keep `actions/checkout` and `actions/setup-node` on supported major versions.
- Preserve Linux native packages required by Tauri and audio builds when editing CI.
- If all Verify jobs fail quickly on every OS, check whether they all failed at the same step before debugging platform-specific causes.
- If the shared failure step is formatting, assume a repo-wide Prettier issue first.

Windows `cargo test` constraint:

- `cargo test` is intentionally skipped on Windows CI. See the detailed comment
  block in `.github/workflows/ci.yml` above the "Run Rust tests" step.
- We now use `ort` with `load-dynamic` plus the official ONNX Runtime CPU
  download, but the hosted Windows runner still fails to start the Rust test
  harness with `STATUS_ENTRYPOINT_NOT_FOUND` (0xc0000139).
- The current official CPU `onnxruntime.dll` still imports `dxgi.dll`, so the
  Windows failure remains a runtime-loader issue, not a compile or link issue.
- Windows CI still validates the desktop stack via `pnpm tauri build`, which
  exercises compile, link, and Tauri packaging on the hosted runner.

## Completion Gate

This gate applies before claiming completion, creating a commit, or pushing.

1. Run `pnpm format` from the repo root.
2. Run the required verification for the highest-risk area touched.
3. If multiple subsystems changed, use the stricter command set.
4. Re-check that any touched docs/config/contracts still match the code.
5. Only then report completion or prepare the change for submission.

Completion is not valid unless the final report includes:

- The implemented change summary
- Fresh verification evidence from this session
- Any intentionally skipped checks
- Any known residual risks or missing manual validation

## Cursor Cloud specific instructions

### System dependencies (pre-installed in snapshot)

The VM snapshot includes Tauri/audio Linux packages (`libwebkit2gtk-4.1-dev`,
`libasound2-dev`, `libayatana-appindicator3-dev`, `librsvg2-dev`, `libssl-dev`,
`libxdo-dev`, `patchelf`, etc.) and Rust stable via `rustup`. These do not need
to be reinstalled.

### Running the app in Cloud

- `pnpm tauri dev` launches a Vite dev server on `http://localhost:1420` and
  compiles + runs the Rust backend in debug mode. The first dev compile takes
  ~5 minutes; subsequent runs are fast thanks to incremental compilation.
- `pnpm tauri build` produces `.AppImage` and `.deb` bundles under
  `src-tauri/target/release/bundle/`.
- The app opens a WebView window on first launch (requires a display). On first
  run it prompts for language selection, library creation, and separation mode.

### Model & ONNX Runtime setup

`./scripts/setup.sh` downloads the Demucs ONNX model (~80 MB) and the ONNX
Runtime shared library into `src-tauri/models/` and
`src-tauri/generated/onnxruntime/` respectively. Both are cached locally and the
script is idempotent (skips download when files are already verified).

### Testing notes

- Frontend: `pnpm test` runs 242 Vitest tests (jsdom environment).
- Backend: `cd src-tauri && cargo test -q` runs 175+ Rust tests. These tests
  exercise the separator, library, lyrics, audio, metadata, and command modules.
- The esbuild warning about ignored build scripts during `pnpm install` is
  harmless — esbuild's binary installs correctly via its platform-specific
  optional dependency.

### Gotchas

- Rust 1.85+ is required. The `time` crate dependency needs edition2024 support,
  which is not available in Rust <1.85. Use `rustup update stable` if the
  pre-installed version is too old.
- `pnpm tauri dev` needs a display (X11/Wayland) to open the WebView window.

## Why This Exists

This repository has repeatedly seen CI failures caused by agents skipping local
formatting and verification. The local checks are fast compared with CI
turnaround and are the required final gate.
