# Contributing to Lemonade Flatpak

Developer and maintainer guide for the [Lemonade](https://github.com/lemonade-sdk/lemonade) Flatpak. End-user documentation (install, configuration, diagnostics) lives in the [README](README.md); this file is about building, testing, and maintaining the package.

## What this is

This repo is the Flatpak packaging for Lemonade. It self-contains four binaries plus a small orchestration script:

| Binary | Role |
| --- | --- |
| `lemond` | The local LLM server. |
| `lemonade` | Command-line client. |
| `lemonade-tray` | System-tray indicator. |
| `lemonade-app` | Tauri desktop UI. |
| `lemonade-supervisor` | Shell script that wires the three together. **The only Lemonade-specific code in this repo** — everything else is upstream, built from source. |

When the user clicks the Lemonade icon, the supervisor decides whether to start a bundled `lemond` or connect to one already running on the host, starts the tray, and launches the desktop app. Subsequent `flatpak run` invocations become "sidecars" that share the supervisor's lifecycle via a shared POSIX `flock`. See the [README](README.md#how-it-works-when-you-launch) for the full launch sequence.

## Prerequisites

- `flatpak` and `flatpak-builder`, with the `flathub` remote configured for your user:
  ```bash
  flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo
  ```
  The GNOME 50 Platform/SDK and the `rust-stable` / `node24` SDK extensions are pulled automatically by `--install-deps-from=flathub` (wrapped by `make build`/`make install`).
- Git submodules — this repo vendors the flatpak-builder shared-modules collection:
  ```bash
  git submodule update --init
  ```
- **Tests:** `podman` or `docker` (the suite runs in the official `bats` container — no host `bats` needed).
- **Linting:** `shellcheck`.
- **Regenerating offline sources:** `podman` or `docker` — `make sources` runs the [flatpak-builder-tools](https://github.com/flatpak/flatpak-builder-tools) generators in a container; no host tools needed.

## Repo layout

| Path | Role |
| --- | --- |
| `ai.lemonade_server.Lemonade.yaml` | Flatpak manifest. Defines every module that goes into the build. |
| `ai.lemonade_server.Lemonade.appdata.xml` | AppStream metadata: release entries + permission justifications for Flathub. |
| `lemonade-supervisor.sh` | The launcher / lifecycle manager. Tested via `bats`. |
| `Makefile` | Developer workflow entry point (`make help`). |
| `modules/` | Discrete C++ dependency modules absent from the GNOME 50 SDK (`nlohmann_json`, `CLI11`, `cpp-httplib`, `libwebsockets`). |
| `shared-modules/` | Git submodule of the flatpak-builder shared-modules collection. We consume only the GTK3 `libayatana-appindicator` stack (for the tray). |
| `tests/supervisor/*.bats` | Supervisor test suite (data resolution, host detection, lifecycle, ownership split). |
| `0001-fix-server-config-port-discovery-early-return.patch` | Carried patch — renderer-side port-discovery early-return fix. |
| `0002-fix-app-store-settings-under-user-config-dir.patch` | Carried patch — moves Tauri settings out of `~/.cache` into the platform config dir. Mirrors [`lemonade-sdk/lemonade#1974`](https://github.com/lemonade-sdk/lemonade/pull/1974); drop on the next upstream bump that includes it. |
| `generated-node-sources.json`, `generated-cargo-sources.json` | Offline npm and cargo source manifests for the Tauri app build. |
| `.github/workflows/` | CI (tests + build verification) and the CalVer release workflow. |

## Development workflow (the Makefile)

`make` (or `make help`) lists every target. The common ones:

```bash
make build       # build into build-dir (no install)
make install     # build + install for the current user
make uninstall   # remove the installed flatpak
make test        # run the supervisor bats suite in a container
make lint        # shellcheck lemonade-supervisor.sh
make clean       # remove build-dir, .flatpak-builder, repo
make status      # dump permissions / metadata / resolved sandbox vars (debug)
```

### Running the app via make

The `run/*` targets launch individual binaries inside the sandbox. Arguments after `--` are forwarded to the launched command (the `--` stops `make`'s own option parsing):

```bash
make run                       # default command (the supervisor: lemond + tray + app)
make run/lemond -- --help      # run lemond directly, passing --help to it
make run/tray                  # run the system-tray indicator directly
make run/desktop               # run the Tauri desktop UI directly
```

### Useful overrides

| Variable | Default | Purpose |
| --- | --- | --- |
| `CONTAINER_ENGINE` | `podman` if present, else `docker` | Engine used by `make test`. |
| `BATS_IMAGE` | `docker.io/bats/bats:latest` | Container image for the test suite. |
| `FLATPAK`, `FLATPAK_BUILDER`, `SHELLCHECK` | the bare command names | Override to use a wrapper or alternate binary. |

```bash
make test CONTAINER_ENGINE=docker
```

## Tests

```bash
make test
```

This runs `tests/supervisor/*.bats` (12 tests covering data-root resolution, host detection, lifecycle, and the ownership-split scenarios) inside the official [`bats` container](https://bats-core.readthedocs.io/en/stable/docker-usage.html). The engine is auto-detected (podman preferred), the repo is bind-mounted with the SELinux `:z` flag, and the minimal Alpine image is topped up with `curl` and `python3` — the supervisor's health checks need `curl`, and the test mocks use a `python3` stub server.

To run against a host `bats` instead (needs `bats`, `curl`, and `python3` on `PATH`):

```bash
bats tests/supervisor/
```

CI runs the same suite plus a full flatpak build on every push and PR to `main`.

## Architecture gotchas

These have bitten us. Read them before touching the manifest, the supervisor, or the patches.

- **Both carried patches are referenced in TWO modules** (`lemonade-cpp` and `lemonade-app`), because both consume the same `&lemonade-source` checkout. Adding a third patch means adding it in both places.
- **`buildsystem: cmake-ninja` uses `no-make-install: true`.** Upstream's CMake `install(CODE)` blocks unconditionally write to `/usr/bin` and `/usr/lib/systemd/system`, both read-only in the sandbox. We skip `cmake --install` and copy binaries explicitly in `post-install`.
- **`/app/bin/resources` is a symlink** to `../share/lemonade-server/resources`. `lemond`'s resource search hardcodes `/usr*/share/lemonade-server` and `<exe_dir>/resources`; `/app` isn't in that list, so the symlink bridges it.
- **The `tauri/custom-protocol` cargo feature is required** when building Tauri with raw `cargo build`. Without it the webview loads `devUrl` (`http://localhost:9123`) and shows a blank page.
- **flatpak `finish-args` parsing is last-write-wins on the same bus-name wildcard.** Never specify both `--own-name=X.*` and `--talk-name=X.*` — the trailing one wins and the other drops silently. `--own-name` already implies talk for the same name.
- **Cross-sandbox cleanup uses a shared `flock`, not signals.** Each bubblewrap instance has its own PID namespace, so signals don't cross. The supervisor holds an exclusive lock; sidecars block on a shared lock; releasing the exclusive wakes every sidecar.
- **The supervisor's traps must be registered BEFORE `acquire_lock`** — sidecars run their entire app lifecycle inside `acquire_lock`'s call to `second_instance_sidecar`. A signal arriving before the trap is set orphans the app.

## Maintenance tasks

### Regenerating `generated-*.json` after an upstream bump

Both files are the offline npm/cargo source manifests for the Tauri app and must be
regenerated whenever upstream's `package-lock.json` or `Cargo.lock` changes. They are
produced in a container — no host Python, no generators, and no lemonade checkout
needed (only podman or docker):

```bash
make sources                       # regenerate both at the manifest-pinned ref
make sources LEMONADE_REF=v10.6.0  # or at an explicit upstream ref
```

`make sources` derives the upstream ref from the `commit:` under the `&lemonade-source`
anchor, fetches the two lock files from GitHub at that ref, and runs
`flatpak-node-generator` / `flatpak-cargo-generator` (from
[flatpak-builder-tools](https://github.com/flatpak/flatpak-builder-tools)) against them,
inside the official [`uv`](https://docs.astral.sh/uv/) image.

uv resolves each generator's dependencies on the fly — the cargo generator from its
PEP 723 inline metadata, the node generator from its git subdirectory — so there is no
venv to bootstrap. A named volume persists uv's download cache across runs; `make
sources/clean` removes it. The flatpak-builder-tools version is pinned via `FBT_REF` in
the `Makefile`; bumping it just changes which ref the next `make sources` resolves.

> Under rootful docker the regenerated files are owned by root (as with `make test`);
> rootless podman writes them as your user.

### Updating to a new upstream Lemonade release

1. Update `tag:` and `commit:` in the `&lemonade-source` anchor (`lemonade-cpp` module in the manifest).
2. Run `make sources` to regenerate both source files (it auto-derives the upstream ref from the `&lemonade-source` `commit:` you just bumped).
3. Verify both carried patches (`0001-…` and `0002-…`) still apply cleanly; refresh line numbers if not. Remember that both patches are referenced in **two** modules (`lemonade-cpp` and `lemonade-app`).
4. Re-check `MIN_*_VERSION` values in upstream's root `CMakeLists.txt` against the pins in `modules/*.json`; bump module commits if upstream raised a minimum.
5. Add a new `<release>` entry in `ai.lemonade_server.Lemonade.appdata.xml`.

### Adding a sandbox permission

If a new feature requires additional sandbox access (file paths, D-Bus names, devices), update `finish-args:` in the manifest **and** add a justification line to `ai.lemonade_server.Lemonade.appdata.xml`'s `<description>` permission list. Flathub reviewers read both.

## Releases

- **CI** (`.github/workflows/ci.yml`) runs on every push and PR to `main`: the supervisor bats suite plus a full flatpak build to verify the manifest still builds.
- **Releases** (`.github/workflows/release.yml`) are cut manually via `workflow_dispatch`. The workflow builds a `Lemonade.flatpak` bundle, derives a CalVer tag (`YYYY.MM.N`, incrementing `N` within the month), pushes the tag, and creates a GitHub release with the bundle attached. End users install it with `flatpak install --user Lemonade.flatpak`.

## License

Licensed under Apache-2.0, matching upstream Lemonade (declared in `ai.lemonade_server.Lemonade.appdata.xml`).
