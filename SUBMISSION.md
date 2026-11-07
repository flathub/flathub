# Flathub submission — com.poktweb.poktcode

## App ID / domain

- Flatpak ID: `com.poktweb.poktcode`
- Homepage (temporary): https://github.com/poktweb/poktcode-releases
- `poktweb.com` is not live yet; a Vercel custom domain is planned.
- Please grant an exception for `appid-url-not-reachable` until the domain is online.
- When the domain is ready we will add Flathub verification under `/.well-known/org.flathub.VerifiedApps.txt`.

## Private source / public binaries

Application source remains private (`poktweb/PoktCode`).
Public artifacts and screenshots are published at https://github.com/poktweb/poktcode-releases.

## Permission exceptions requested

IDE workloads need:

| finish-arg | Why |
|---|---|
| `--filesystem=host` | Open arbitrary project folders on the host |
| `--talk-name=org.freedesktop.Flatpak` | `flatpak-spawn --host` for local MCP/tooling (node/pnpm on host) |
| `--socket=ssh-auth` | Git over SSH from the IDE |
| `--device=all` | GPU / NVIDIA stability (Electron) |
| `--system-talk-name=org.freedesktop.login1` | Chromium startup without D-Bus hang |
| portal talk-names | Clipboard / document portal for chat image paste |

## Generative AI disclosure

PoktCode is an AI-assisted IDE. Parts of the application code and packaging docs were produced with AI assistance. The Flatpak **manifest** (`com.poktweb.poktcode.yml`) was authored/reviewed as packaging config derived from the existing local Electron BaseApp layout (not an opaque AI dump).

## Build locally

```bash
flatpak-builder --user --force-clean build-dir packaging/flathub/com.poktweb.poktcode.yml
```
