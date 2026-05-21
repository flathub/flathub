# AGENTS.md

Quick context for AI agents working in this repo.

## What this is

Flatpak packaging for [Lemonade](https://github.com/lemonade-sdk/lemonade). It bundles four upstream binaries — `lemond` (LLM server), `lemonade` (CLI), `lemonade-tray`, and `lemonade-app` (Tauri desktop UI) — plus `lemonade-supervisor`, a shell launcher that detects or starts `lemond`, then runs the tray and desktop app. Subsequent `flatpak run` invocations become "sidecars" sharing the supervisor's lifecycle via a POSIX `flock`. The supervisor is the only Lemonade-specific code here; everything else is built from upstream source.

**Read [CONTRIBUTOR.md](CONTRIBUTOR.md) first.** It is the canonical reference for the repo layout, the `make` workflow (build/install/run/test/status), the maintenance tasks (offline sources, upstream bumps, sandbox permissions), the release process, and — most importantly — the **architecture gotchas** that have bitten us. Don't touch the manifest, the supervisor, or the carried patches without reading the gotchas.

Treat the code as the source of truth: any doc (this one included) may have drifted.

## Bumping the bundled Lemonade

To repoint the build at a different upstream Lemonade — a tagged release or an arbitrary commit on `main` — work in this order:

1. **Edit the `&lemonade-source` anchor** (in the `lemonade-cpp` module of `ai.lemonade_server.Lemonade.yaml`):
   - **Tagged release:** set `tag:` and `commit:` (the commit the tag points to).
   - **Commit from `main`:** drop the `tag:` line and set `commit:` to the hash. A git source with only `commit:` is valid and fully reproducible; the tag is optional.

   The anchor is reused (`*lemonade-source`) by the `lemonade-app` and `metadata` modules, so this one edit repoints all of them.
2. **`make sources`** — regenerates `generated-node-sources.json` and `generated-cargo-sources.json` from upstream's `package-lock.json` / `Cargo.lock`. It auto-derives the ref from the `commit:` you just set (`LEMONADE_REF` in the `Makefile`), so it works for tags and bare commits alike. Override explicitly with `make sources LEMONADE_REF=<ref>` if needed.
3. **Re-check the carried patches** (`0001-…`, `0002-…`) — confirm each is still *needed*. A patch becomes unnecessary once upstream merges the equivalent fix (e.g. `0002` mirrors [lemonade#1974](https://github.com/lemonade-sdk/lemonade/pull/1974) — drop it on the bump that includes it); check upstream's history at the new ref. When dropping one, delete the file **and** remove its `type: patch` entry from **both** modules (`lemonade-cpp` and `lemonade-app` each list every patch).
4. **`make build`** (or `make install`). flatpak-builder applies the patches at the start of each module build and fails fast if a hunk no longer applies — so the build is also the "do they still apply cleanly?" check. If a patch is rejected, refresh its line numbers.

> **Keep this section in sync with the patches.** The patch names (`0001-…`, `0002-…`) and the `lemonade#1974` example above are concrete and will drift. Whenever you add, drop, or renumber a carried patch, update — or remove — the corresponding references in this file so it keeps matching what's actually in the repo.

That covers the manifest/source side. The rest of the upstream-bump checklist — module `MIN_*_VERSION` re-checks and a new appdata `<release>` entry — is in [CONTRIBUTOR.md](CONTRIBUTOR.md#updating-to-a-new-upstream-lemonade-release).

> **Release notes:** keep each appdata `<release>` description to a single one-line `<p>` summary — no nested `<ul>` bullet lists or internal phasing (e.g. "phase 2"). The entry is shown to end users in software centers.

## Touching the supervisor

`lemonade-supervisor.sh` is the only Lemonade-specific code in the repo and is covered by the bats suite. Any change to it must keep the suite green before you commit:

```bash
make test    # bats suite in a container — MUST pass
make lint    # shellcheck
```
