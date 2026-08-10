# Inkdrop

[Inkdrop](https://www.inkdrop.app/) is a Markdown note-taking app for developers.
This repository is its Flathub packaging.

```sh
flatpak install flathub app.inkdrop.Inkdrop
flatpak run app.inkdrop.Inkdrop
```

Inkdrop is proprietary software. The app runs offline against a local database, while
syncing across devices requires an account and a paid subscription after a 30-day free
trial.

## What the app can access

| Access                    | What it is for                                                         |
| ------------------------- | ---------------------------------------------------------------------- |
| Network                   | Syncing, installing plugins, AI providers, and the built-in MCP server |
| Home directory            | The local backup directory, which is a path you type rather than pick  |
| Keyring                   | Storing your account access key, via the Secret Service                |
| Tray icon                 | The "keep Inkdrop running in the background" preference                |
| Suspend and resume events | Reconnecting sync after the machine wakes up                           |
| GPU                       | Hardware-accelerated rendering                                         |

Opening, importing and exporting notes and attachments goes through the system file
chooser, so those files are reachable without granting anything up front.

Anything here can be revoked with [Flatseal](https://flathub.org/apps/com.github.tchx84.Flatseal)
or `flatpak override`. Revoking home directory access leaves the app fully usable —
only the local backup feature stops working.

## Updates

Updates arrive through Flathub, not through the app. Inkdrop's own update check is
disabled inside the sandbox, so run `flatpak update` (or let your software centre do
it) as usual.

## Reporting problems

- Problems with **this Flatpak** — packaging, permissions, the app not starting:
  [open an issue here](https://github.com/flathub/app.inkdrop.Inkdrop/issues).
- Problems with **Inkdrop itself**: [the Inkdrop forum](https://forum.inkdrop.app/).
