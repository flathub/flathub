# Building Proton Authenticator from Source (Flatpak)

> **Maintenance tools** — these scripts are not used during the Flatpak build itself;
> they are only needed when updating to a new upstream release:
> - `flatpak-cargo-generator.py` — get it from
>   [flatpak/flatpak-builder-tools](https://github.com/flatpak/flatpak-builder-tools/tree/master/cargo)
> - `tools/strip-private-registry-lockfile.py` — a custom script in this repo that
>   filters private-registry packages out of the WebClients `yarn.lock` before building
>   the frontend dist

Proton Authenticator is a [Tauri v2](https://tauri.app/) desktop app that lives inside the
[ProtonMail/WebClients](https://github.com/ProtonMail/WebClients) monorepo. Building it for
Flatpak requires two pre-generated offline source lists — one for Cargo crates and one for
Node/Yarn packages — that must be committed alongside the manifest.

---

## Prerequisites

```sh
# flatpak-cargo-generator — needed to regenerate cargo-sources.json
curl -LO https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/cargo/flatpak-cargo-generator.py
# Requires: Python 3.9+, no extra dependencies

# tools/strip-private-registry-lockfile.py — already in this repo
# No installation needed; run with python3 directly
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
python3 flatpak-cargo-generator.py \
    applications/authenticator/src-tauri/Cargo.lock \
    -o ../cargo-sources.json
```
</invoke>

This produces `cargo-sources.json` in your working directory (one level above the checkout).
Copy it into the submission directory alongside the manifest.

---

## Step 3 — Regenerate `authenticator-dist.tar.gz` (frontend, when needed)

The frontend is pre-built and supplied as `authenticator-dist.tar.gz` rather than built
inline. This avoids pulling the entire monorepo's Node dependency graph (which includes
Playwright, Chromium, and hundreds of test-only packages) into a `node-sources.json`
that would be several hundred MB — far larger than the 11 MB dist archive.

The dist output is pure JS/CSS/HTML and is **architecture-independent**, so it only needs
to be regenerated when the authenticator version changes. To regenerate it:

```sh
cd webclient-src   # the WebClients checkout from Step 1

# 1. Strip private-registry packages from yarn.lock.
#    tools/strip-private-registry-lockfile.py removes any block whose
#    resolution URL points to a non-public registry (auto-detected from
#    .yarnrc.yml, or pass --private-scope @scopename explicitly):
python3 /path/to/this/repo/tools/strip-private-registry-lockfile.py \
    yarn.lock yarn.lock.filtered
cp yarn.lock yarn.lock.orig
cp yarn.lock.filtered yarn.lock

# 2. Remove private-registry config from .yarnrc.yml:
python3 -c "
import re, pathlib
text = pathlib.Path('.yarnrc.yml').read_text()
for key in ('httpProxy','httpsProxy','npmPublishRegistry'):
  text = re.sub(rf'^{key}:.*\n','',text,flags=re.MULTILINE)
for key in ('plugins','npmScopes'):
  text = re.sub(rf'^{key}:\n([ \t]+.*\n)*','',text,flags=re.MULTILINE)
pathlib.Path('.yarnrc.yml').write_text(text)"

# 3. Add the missing @dnd-kit/sortable dep:
python3 -c "
import json,pathlib
pkg=pathlib.Path('applications/authenticator/package.json')
d=json.loads(pkg.read_text())
d.setdefault('dependencies',{})['@dnd-kit/sortable']='^10.0.0'
pkg.write_text(json.dumps(d,indent=4)+'\n')"

# 4. Install workspace deps and build:
node .yarn/releases/yarn-4.9.4.cjs workspaces focus proton-authenticator
node .yarn/releases/yarn-4.9.4.cjs workspace proton-authenticator build:web

# 5. Pack and record the checksum:
tar -czf ../authenticator-dist.tar.gz applications/authenticator/dist/
sha256sum ../authenticator-dist.tar.gz
```

Update the `sha256:` field in `me.proton.Authenticator.yml` with the new checksum.

---

## Step 4 — Update the manifest

Open `me.proton.Authenticator.yml` and:

1. **Fill in the commit hash** for the git source:
   ```yaml
   - type: git
     url: https://github.com/ProtonMail/WebClients.git
     commit: <sha>          # replace with the actual hash
   ```

2. **Update the `sha256:`** for `authenticator-dist.tar.gz` if you regenerated it.

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

Install and run from a local repo (the `--run` shortcut is unreliable for GNOME 49
apps because glycin's sub-sandboxing requires a full D-Bus session):

```sh
flatpak --user remote-add --no-gpg-verify local-test repo
flatpak --user install local-test me.proton.Authenticator
flatpak run me.proton.Authenticator

# Clean up when done:
flatpak --user uninstall me.proton.Authenticator
flatpak --user remote-delete local-test
```

---

## Files in this submission

| File | Purpose |
|------|---------|
| `me.proton.Authenticator.yml` | Main Flatpak manifest |
| `me.proton.Authenticator.metainfo.xml` | AppStream metadata |
| `me.proton.Authenticator.desktop` | Desktop entry |
| `flathub.json` | Flathub-specific configuration |
| `cargo-sources.json` | Offline Cargo crate sources (generated by `flatpak-cargo-generator.py`) |
| `authenticator-dist.tar.gz` | Pre-built frontend dist (architecture-independent JS/CSS/HTML) |

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