# Building Proton Authenticator from Source (Flatpak)

Proton Authenticator is a [Tauri v2](https://tauri.app/) desktop app that lives inside the
[ProtonMail/WebClients](https://github.com/ProtonMail/WebClients) monorepo. Building it for
Flatpak requires two pre-generated offline source lists — one for Cargo crates and one for
Node/Yarn packages — that must be committed alongside the manifest.

---

## Prerequisites

```sh
# flatpak-node-generator (supports Yarn Berry / v4 lockfiles)
pipx install "git+https://github.com/flatpak/flatpak-builder-tools.git#subdirectory=node"

# flatpak-cargo-generator
curl -LO https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/cargo/flatpak-cargo-generator.py
# or clone the tools repo and use the script directly
```

Make sure you are using a **recent** version of `flatpak-node-generator` — support for
Yarn Berry (v2+) lockfiles was added after the original PR was filed.

---

## Step 1 — Clone the upstream source at the right tag

```sh
git clone --depth 1 --branch proton-authenticator@1.1.4 \
    https://github.com/ProtonMail/WebClients.git webclient-src
cd webclient-src
```

> **Tip:** Confirm the tag exists first:
> ```sh
> git ls-remote --tags https://github.com/ProtonMail/WebClients.git \
>     'refs/tags/proton-authenticator@*'
> ```
> If the tag does not exist yet (e.g. the repo uses a different naming convention),
> check out the commit that corresponds to the release and note its SHA for the
> `commit:` field in `me.proton.Authenticator.yml`.

---

## Step 2 — Generate `cargo-sources.json`

The Cargo lock file is inside the Tauri crate:

```sh
python3 /path/to/flatpak-cargo-generator.py \
    applications/authenticator/src-tauri/Cargo.lock \
    -o ../cargo-sources.json
```

This produces `cargo-sources.json` in your working directory (one level above the checkout).
Copy it into the submission directory alongside the manifest.

---

## Step 3 — Generate `node-sources.json`

The monorepo uses **Yarn 4 (Berry)** with `nodeLinker: node-modules`.
`flatpak-node-generator` handles this lockfile format:

```sh
# Run from the root of the WebClients checkout
flatpak-node-generator yarn yarn.lock -o ../node-sources.json
```

> ⚠️ **Warning:** The WebClients monorepo is large. The generated `node-sources.json`
> will be several hundred MB. GitHub has a 100 MB per-file limit — use the `-s` flag
> to split the output if needed:
>
> ```sh
> flatpak-node-generator yarn yarn.lock -s -o ../node-sources.json
> # Produces node-sources.0.json, node-sources.1.json, …
> ```
>
> Then list each split file as a separate source in `me.proton.Authenticator.yml`.

Copy the resulting file(s) into the submission directory.

---

## Step 4 — Update the manifest

Open `me.proton.Authenticator.yml` and:

1. **Fill in the commit hash** for the git source if you are using a commit SHA instead of a tag:
   ```yaml
   - type: git
     url: https://github.com/ProtonMail/WebClients.git
     commit: <sha>          # replace with the actual hash
   ```

2. **Update split sources** if you used `-s` when generating the node sources:
   ```yaml
   - node-sources.0.json
   - node-sources.1.json
   # …
   ```

---

## Step 5 — Test the build locally

```sh
flatpak-builder \
    --force-clean \
    --install-deps-from=flathub \
    --repo=repo \
    builddir \
    me.proton.Authenticator.yml
```

Run the result:

```sh
flatpak-builder --run builddir me.proton.Authenticator.yml proton-authenticator
```

---

## Files in this submission

| File | Purpose |
|------|---------|
| `me.proton.Authenticator.yml` | Main Flatpak manifest |
| `me.proton.Authenticator.metainfo.xml` | AppStream metadata |
| `flathub.json` | Flathub-specific configuration |
| `cargo-sources.json` | *(to be generated)* Offline Cargo crates |
| `node-sources.json` | *(to be generated)* Offline Yarn packages |

---

## Notes on the build architecture

### Why the full monorepo?

The `@proton/*` packages (e.g. `@proton/shared`, `@proton/components`) are **workspace
packages** defined under `packages/` in the WebClients repo. They are resolved locally
during the build — not downloaded from npm — so the entire monorepo must be checked out.

### Yarn 4 + `nodeLinker: node-modules`

The project's `.yarnrc.yml` sets `nodeLinker: node-modules` (not PnP), which means the
standard `node_modules/` layout is used. This is well-supported by `flatpak-node-generator`.

### Tauri v2 and WebKitGTK

Tauri v2 on Linux links against **webkit2gtk-4.1**, which is provided by
`org.gnome.Platform//47`. The `--env=WEBKIT_DISABLE_DMABUF_RENDERER=1` finish-arg is a
known workaround for WebKit rendering issues in sandbox environments.

### Secret storage

The `keyring` crate uses `sync-secret-service` on Linux, which communicates over D-Bus
with a secrets service (e.g. GNOME Keyring). The `--talk-name=org.freedesktop.secrets`
finish-arg grants the necessary D-Bus access.
```

Now let me look at what we have: