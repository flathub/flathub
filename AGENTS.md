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
- `docs/site/**/*.html`
- `docs/site/**/*.css`
- `src-tauri/tauri.conf.json`
- `.github/workflows/*.yml`
- Any Markdown, JSON, HTML, CSS, or YAML file touched in the change

GitHub Actions and Linux build constraints:

- Keep `actions/checkout` and `actions/setup-node` on supported major versions.
- Preserve Linux native packages required by Tauri and audio builds when editing CI.
- If all Verify jobs fail quickly on every OS, check whether they all failed at the same step before debugging platform-specific causes.
- If the shared failure step is formatting, assume a repo-wide Prettier issue first.

Windows `cargo test` constraint (DO NOT try to fix by staging DLLs):

- `cargo test` is intentionally skipped on Windows CI. See the detailed comment
  block in `.github/workflows/ci.yml` above the "Run Rust tests" step.
- Root cause: pyke's prebuilt `onnxruntime.lib` (ort-sys) bakes in DirectML,
  creating hard load-time imports for `DirectML.dll`, `D3D12.dll`, `DXGI.dll`,
  and `DXCore.dll`. Headless Windows Server runners lack the GPU/DirectX 12
  stack these DLLs require → `STATUS_ENTRYPOINT_NOT_FOUND` (0xc0000139).
- The following approaches were **already tried exhaustively and all failed**:
  staging DirectML from NuGet, staging onnxruntime.dll from the official ORT
  release, using the D3D12 Agility SDK, pinning to windows-2022, and
  manipulating DLL search paths via PATH.
- The only real fixes are: (a) switch ort to `load-dynamic` linking so the
  DirectML/D3D12 imports are not in the binary's import table, or (b) wait
  for pyke to ship a Windows build without DirectML as the default provider.
- Windows CI still validates compilation and linking via `pnpm tauri build`.

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

## Why This Exists

This repository has repeatedly seen CI failures caused by agents skipping local
formatting and verification. The local checks are fast compared with CI
turnaround and are the required final gate.
