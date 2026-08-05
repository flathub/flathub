# Packaging notes

Maintainer-facing. For bumping to a new upstream release see [UPDATE.md](UPDATE.md);
for what the package grants at runtime see [README.md](README.md).

## How it is put together

The build repacks the published `.deb` as `extra-data` — nothing is compiled here.

`apply_extra.sh` runs at *install* time, when only the Platform runtime is mounted:
no SDK, so no binutils and no `ar`. It unpacks with `bsdtar`, which reads both the
`ar` wrapper and the inner tarball, and is the same approach `com.google.Chrome` uses.
`./opt/Inkdrop/…` is three path components deep, so stripping three lands the payload
at `/app/extra/`.

`inkdrop.sh` launches `/app/extra/inkdrop` through `zypak-wrapper` and states the
display backend explicitly — Electron reaches for Wayland on its own whenever a
Wayland socket exists, and aborts if it finds one it cannot use.

## Blockers before opening the submission PR

- [ ] **Re-point `extra-data` at a stable release.** Both sources currently pin
      `6.0.0-rc.2`, and `flatpak-builder-lint` rejects a pre-release as the latest
      version on the stable remote. Steps 1 and 2 of [UPDATE.md](UPDATE.md).
- [ ] **Keep pre-releases out of the checker feed.** `x-checker-data` reads
      `latest-linux.yml` / `latest-linux-arm64.yml`, which today serve `6.0.0-rc.2`.
      As long as `generateUpdatesFilesForAllChannels: true` publishes release
      candidates to the `latest` channel, the update bot will pull them into
      Flathub stable. Release candidates need to land in `beta-*.yml` only.
- [ ] **Request both linter exceptions** — `finish-args-home-filesystem-access` and
      `finish-args-login1-system-talk-name`. See below.

## Permission rationale

| Argument | Why |
| --- | --- |
| `--socket=wayland` + `--socket=fallback-x11` + `--share=ipc` | Display server. `--share=ipc` is required alongside X11. See the X11 caveat under app-side changes. |
| `--device=dri` | GPU acceleration. `--device=all` would also grant input and USB. |
| `--share=network` | Sync, plugin installs, AI providers, local MCP/HTTP server. |
| `--talk-name=org.freedesktop.secrets` | `@napi-rs/keyring` stores the access key via libsecret. KWallet 6 serves the same interface, so `org.kde.kwalletd6` is not needed. |
| `--talk-name=org.kde.StatusNotifierWatcher` | Tray icon for the "keep running in the background" preference. |
| `--system-talk-name=org.freedesktop.login1` | `powerMonitor` `resume` / `unlock-screen`, which Electron implements over logind. Without it `handleResumeSystem` never fires and sync does not reconnect after suspend. |
| `--filesystem=home` | The local backup directory is a free-text path validated with `fs.isDirectorySync()` and written to continuously by `file-replicator.ts`. A portal-picked path resolves to `/run/user/$UID/doc/…`, so the file chooser portal cannot serve a hand-typed one. |

Deliberately omitted: `--socket=pulseaudio` (no audio anywhere in the app),
`--talk-name=org.freedesktop.Notifications` (`NotificationManager` renders in-app
toasts, not libnotify), `--allow=devel` (zypak already provides what the Chromium
sandbox needs), and `--socket=system-bus` (unfiltered bus access, never needed
alongside a `--system-talk-name`).

Every other file interaction — import, export, attachments, restore-from-backup —
goes through `dialog.showOpenDialog` / `showSaveDialog`, which Electron routes to the
file chooser portal inside a sandbox and which needs no static permission.

### Linter exceptions needed

Two of the arguments above are **errors** in `flatpak-builder-lint`, not warnings, so the
CI check on the submission PR fails until Flathub grants an exception for each:

| Check | Held by | Notes |
| --- | --- | --- |
| `finish-args-home-filesystem-access` | 505 apps | Routine, but the one reviewers push back on hardest. |
| `finish-args-login1-system-talk-name` | 48 apps | Includes Slack, Signal, Bitwarden, KeePassXC, VS Code. |

Counts are from the linter's bundled `staticfiles/exceptions.json`. Worth knowing before
writing the PR: only three published apps hold **both** (`com.anydesk.Anydesk`,
`io.github.zaps166.QMPlay2`, `org.opencpn.OpenCPN`), so asking for the pair invites more
scrutiny than either alone. The `login1` justification is the stronger of the two and
degrades cleanly; `--filesystem=home` is the one to have an answer ready for.

Once granted they are stored keyed by branch:

```json
{
  "app.inkdrop.Inkdrop": {
    "stable": {
      "finish-args-home-filesystem-access": "backup directory is a hand-typed path, so the file chooser portal cannot serve it",
      "finish-args-login1-system-talk-name": "powerMonitor resume events, so sync reconnects after suspend"
    }
  }
}
```

`--user-exceptions` does **not** take that shape. It wants the checks flat against the
app ID, with no branch key, and it is inert unless `--exceptions` is also passed — on its
own it silently changes nothing and the lint still fails:

```sh
cat > /tmp/inkdrop-exceptions.json <<'EOF'
{
  "app.inkdrop.Inkdrop": {
    "finish-args-home-filesystem-access": "hand-typed backup path",
    "finish-args-login1-system-talk-name": "powerMonitor resume events"
  }
}
EOF
flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
  --exceptions --user-exceptions /tmp/inkdrop-exceptions.json \
  manifest app.inkdrop.Inkdrop.yml
```

Without `login1` the only loss is the resume-from-suspend nudge —
reconnect-on-network-change is driven by the renderer's `online` event and PouchDB live
replication retries on its own — so dropping it again is a clean fallback if review
pushes back.

## App-side changes

Landed upstream in `inkdropapp/desktop`:

- **Report the Flathub app ID.** Desktop environments match a window to its `.desktop`
  entry by app_id (Wayland) or `WM_CLASS` (X11). `app.setName('inkdrop')` makes the app
  report `inkdrop`, while Flathub requires `app.inkdrop.Inkdrop.desktop`. One guarded
  line in `src/main.js` covers Wayland, and `desktopName` in the emitted
  `build/package.json` fixes electron-builder's `StartupWMClass` for the deb and
  AppImage:

  ```js
  if (process.env.FLATPAK_ID) app.setDesktopName(`${process.env.FLATPAK_ID}.desktop`)
  ```

  The Electron BaseApp's `/app/bin/patch-desktop-filename` does the same by rewriting
  `app.asar`, but it is a build-time patch and `extra-data` only unpacks at install
  time — so it would have to run inside `apply_extra.sh`, and it is deprecated in
  favour of `patch-electron-desktop-filename` in 26.08.

- **Skip the in-app updater under Flatpak.** The repacked `.deb` still carries
  `resources/package-type`, so electron-updater will offer updates it cannot install
  inside the sandbox. The hourly background check is gated on `process.env.FLATPAK_ID`
  in `AutoUpdater.startCheckingForAutoUpdates()`, which also covers the
  `core.autoUpdate` preference (it re-enters through the same method).

Still open:

- **The Check for Updates menu command** reaches `checkForUpdatesAndNotify()` directly
  via `application:check-for-update` (`src/main/app.ts:411`), bypassing the gate. Under
  Flatpak it will find a newer version on the S3 feed, offer it, and fail on install. A
  silent no-op would be worse than the bug, so this wants a dialog saying updates are
  delivered by Flathub — or the menu item hidden.

- **The X11 requirement.** `inkdrop-keyboard-layout` calls `XOpenDisplay()` at module
  load and throws without a display. Because it is imported by
  `inkdrop-keymap/src/helpers.js` in the renderer, a sandbox without the X11 socket
  starts, creates a window, and paints nothing — while the main process logs a clean
  startup. Note `--socket=fallback-x11` withholds the X display whenever a Wayland
  socket exists, so the current pairing needs verifying on a real Wayland session
  before submission. Constructing the manager lazily — or falling back to the bundled
  US keymap when the display is unavailable — is what would make a Wayland-only
  flatpak possible, and it also affects any Wayland session without XWayland,
  independent of Flathub.
