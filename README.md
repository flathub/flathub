# Lemonade Flatpak

A self-contained Flatpak for [Lemonade](https://github.com/lemonade-sdk/lemonade) — a local LLM server (`lemond`) with a desktop app and a system-tray indicator, all in one package. Install it once and you get the server, the tray, and the desktop UI; you don't need to set anything up on the host first.

## What's in the box

The Flatpak ships four pieces that work together:

| Binary | What it does |
| --- | --- |
| `lemond` | The local LLM server. Listens on `127.0.0.1:13305` by default and serves chats, model management, etc. |
| `lemonade-tray` | The system-tray indicator. Lets you see and control the server without keeping the app open. |
| `lemonade-app` | The desktop UI (Tauri). What opens when you click the "Lemonade" icon. |
| `lemonade-supervisor` | The small launcher that wires it all together. You never call this directly — it runs on your behalf when you start the app. |

## Install

```bash
flatpak-builder --force-clean --user --install --install-deps-from=flathub \
  build-dir ai.lemonade_server.Lemonade.yaml
```

Then launch from your application menu (search for "Lemonade") or:

```bash
flatpak run ai.lemonade_server.Lemonade
```

## How it works when you launch

When you click the "Lemonade" icon, the supervisor runs through this sequence in under a second:

1. **Is a Lemonade server already running on this machine?** The supervisor checks `http://127.0.0.1:13305/api/v1/health` and, as a fallback, listens briefly for the UDP broadcast that `lemond` advertises (in case it's on a non-default port). If a server is found, the Flatpak just connects to it and never starts a competing one.
2. **No server found → start one.** The bundled `lemond` is started in the background using whichever data directory makes sense (see "Where your data lives" below).
3. **Start the tray icon** (unless you've explicitly turned it off — see "Configuration").
4. **Open the desktop window.**

When you **close the app window**, the tray and the server keep running — that's the whole point of having a tray. You can re-open the app from the tray menu without any startup cost.

When you **quit from the tray menu**, the supervisor cleanly shuts down everything *it* started. If the server was already running on the system (not started by the Flatpak), it's left alone.

> **Ownership rule:** the Flatpak never shuts down a `lemond` it didn't start. So if you have the system service running, quitting the Flatpak's tray won't touch it.

## Where your data lives

The Flatpak treats the two scenarios — *external server* and *bundled server* — as completely independent:

- **Bundled server (no host `lemond` running).** `lemond` keeps its config and downloaded models in the Flatpak's per-app cache directory (`~/.var/app/ai.lemonade_server.Lemonade/cache/lemonade/`). Nothing on the host outside that path is touched. To pick another location (e.g. a separate disk), set `LEMONADE_DATA_DIR=/path` before launch.
- **External server** (a host `lemond` is already running). The Flatpak is purely a UI client — it never reads or writes the external server's data directory. Whatever location *that* server uses is its concern.

This is intentionally a simple split: the Flatpak audience is "I just want Lemonade", not "I'm running multiple installations and need the Flatpak to inherit one of them". If you do have a host install you want the Flatpak to share with, the simplest path is to run that server (`systemctl start lemond` or `lemond` directly) before launching the Flatpak — external-detection then connects to it and the Flatpak's bundled `lemond` stays out of the way.

### Hugging Face model cache (the one exception)

`lemond` downloads models via Hugging Face, which keeps a global cache at `~/.cache/huggingface/hub/`. The Flatpak's bundled `lemond` shares this cache with the host (via `--filesystem=xdg-cache/huggingface:rw`) so models downloaded by other AI tools — `huggingface_hub`, `transformers`, the `llama.cpp` CLI — are reused instead of re-downloaded. This is the only host data dir the Flatpak deliberately reaches into.

You can override the location by setting `HF_HOME` before launching the Flatpak.

## Configuration (environment variables)

All of these are optional — leave them unset for the default behavior.

Flatpak passes the host environment through into the sandbox, so `lemond`'s own
environment variables (documented in the [upstream configuration reference](https://lemonade-server.ai/docs/guide/configuration/))
are forwarded and honored as-is — e.g. `LEMONADE_API_KEY`, `LEMONADE_ADMIN_API_KEY`,
`LEMONADE_LOG_LEVEL`, and `HF_HUB_CACHE`. On top of that, the supervisor itself
reads the standard `LEMONADE_PORT` / `LEMONADE_HOST` (the very names `lemond`
uses) plus a few Flatpak-only orchestration knobs:

| Variable | Purpose |
| --- | --- |
| `LEMONADE_PORT` | Port for the bundled `lemond` and the port to expect when probing for an existing one (default `13305`). The same upstream variable `lemond` reads natively. The supervisor also catches non-default ports automatically via the UDP beacon (unless `LEMONADE_FLATPAK_NO_BEACON` is set), so you rarely need to set this. |
| `LEMONADE_HOST` | Bind address for the bundled `lemond` and where to look for an existing one (default `127.0.0.1`, loopback-only). Set to `0.0.0.0` to expose the bundled server on the network. The same upstream variable `lemond` reads natively. |
| `LEMONADE_DATA_DIR` | Force `lemond` to use this exact directory for config + models + cache. Bypasses all auto-detection. |
| `LEMONADE_FLATPAK_FORCE_BUNDLED=1` | Skip the "is a host server running?" check and always start the bundled `lemond`. Useful if the host server is in a bad state and you want a fresh one. Will fail if the host server has the port. |
| `LEMONADE_FLATPAK_NO_BEACON=1` | Skip the UDP beacon auto-discovery. The Flatpak still connects to a `lemond` at `LEMONADE_HOST:LEMONADE_PORT` if one is running, otherwise starts the bundled server — but never auto-adopts a server that only advertises itself via the beacon (e.g. one on a non-default port or elsewhere on the network). |
| `HF_HOME` | Where Hugging Face stores its model cache. Defaults are documented above. |
| `LEMONADE_LOG_DIR` | Override the directory where the supervisor and `lemond` write their transient logs. Defaults to `$XDG_RUNTIME_DIR/lemonade` (ephemeral, cleared on logout). |

To pass an environment variable to the Flatpak:

```bash
flatpak run --env=LEMONADE_FLATPAK_FORCE_BUNDLED=1 ai.lemonade_server.Lemonade
```

## Modes of operation (quick reference)

| Situation | What the Flatpak does |
| --- | --- |
| Fresh install, no host Lemonade | Starts its own server, tray, and app. Stores everything under `~/.var/app/ai.lemonade_server.Lemonade/cache/lemonade/` (Flatpak per-app cache; not visible to other tools on the host). |
| You already run `lemond` from the RPM via systemd | Connects to the system server (external detection); does **not** start a competing one; never touches the systemd service's lifecycle. |
| You've been running `lemond` from your shell | If it's running, the Flatpak connects to it. If it's not, the Flatpak's bundled `lemond` starts with its own per-app cache — your host `~/.cache/lemonade/` data is left untouched. |
| You quit the app window | Tray and server keep running in the background. |
| You quit from the tray menu | Bundled server is gracefully shut down. System servers are left alone. |
| You want a clean isolated environment | Set `LEMONADE_FLATPAK_FORCE_BUNDLED=1` and `LEMONADE_DATA_DIR=/some/empty/path`. |

## Diagnostics

If something goes wrong, the supervisor logs every launch to your session's runtime directory (ephemeral — cleared on logout):

```bash
cat /run/user/$(id -u)/lemonade/supervisor.log
```

`lemond` itself writes its own log to the same place (`lemonade-server.log`). Both files vanish at logout, so they never accumulate on disk.

Every launch starts with a `PRELUDE` line containing the key facts:

```
2026-05-20T17:43:20Z PRELUDE owner=flatpak host=127.0.0.1:13305 data_source=xdg data_root=/home/.../lemonade pid=384589 lemond_pid=384599 tray_pid=384626
```

You can also stream live output:

```bash
flatpak run ai.lemonade_server.Lemonade 2>&1 | tee /tmp/lemonade.log
```

The supervisor mirrors all of its log lines to stderr too, so they show up in the terminal. To override the log directory (e.g. for persistent collection), set `LEMONADE_LOG_DIR=/some/path` before launch.

### Common questions

**The tray icon doesn't appear.** Your desktop environment might be missing a StatusNotifier host. GNOME requires the AppIndicator extension; KDE/Xfce/Cinnamon work out of the box. When the supervisor detects that the tray failed to start, it falls back to "app close = cleanup" mode — the app and server still run; closing the app window is what tears them down.

**Models I downloaded outside the Flatpak aren't showing up.** The Flatpak's bundled `lemond` uses its own per-app cache, so models in your host `~/.cache/lemonade/` aren't visible. If you want shared access, start the host `lemond` before launching the Flatpak — external-detection will connect to it instead. Hugging Face *models* (under `~/.cache/huggingface/`) are always shared and don't need any special setup.

**`lemond` keeps running after I close everything.** Make sure you quit from the **tray menu**, not just the app window. Closing the app window is intentionally a no-op for the server when a tray is active (that's why we have a tray).

## Contributing

Developer and maintainer documentation — repo layout, the `make` workflow, running the tests, regenerating offline sources, upstream bumps, and the architecture gotchas — lives in [CONTRIBUTOR.md](CONTRIBUTOR.md).

Quick start:

```bash
make            # list all targets
make install    # build + install for the current user
make test       # run the supervisor test suite (containerized bats)
make status     # dump permissions / metadata / resolved vars for debugging
```
