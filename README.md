# Classi on Flathub

Flathub packaging for [Classi](https://github.com/openpatch/classi), a
local-first teacher desk for groups, grades, notes and checklists.

## Files

| File | Role |
| --- | --- |
| `org.openpatch.classi.yml` | **The manifest Flathub builds.** Generated — do not edit by hand. |
| `flatpak-flutter.yml` | The input manifest. Edit this, then regenerate. |
| `generated/` | Pinned sources, the offline Flutter SDK module, and patches. Generated. |

## Why there are two manifests

Flathub builds [with no network
access](https://docs.flathub.org/docs/for-app-authors/requirements#no-network-access-during-build)
and [from
source](https://docs.flathub.org/docs/for-app-authors/requirements#building-from-source).
Flutter fails both by default: the `flutter` tool downloads the Dart SDK on
first use, and `pub get` downloads packages.

[flatpak-flutter](https://github.com/TheAppgineer/flatpak-flutter) resolves
this. It reads `flatpak-flutter.yml` plus the app's `pubspec.lock` and emits
`org.openpatch.classi.yml`, in which the Flutter SDK and every pub dependency
is a pinned, checksummed source.

## Regenerating

```sh
git clone https://github.com/TheAppgineer/flatpak-flutter.git
pip install -r flatpak-flutter/requirements.txt

# From this directory:
../flatpak-flutter/flatpak-flutter.py flatpak-flutter.yml
```

Regenerate whenever the app's dependencies, the Flutter version, or anything in
`flatpak-flutter.yml` changes. Commit both the input and the generated output.

## Building and verifying locally

```sh
flatpak install flathub org.freedesktop.Platform//25.08 \
  org.freedesktop.Sdk//25.08 org.freedesktop.Sdk.Extension.llvm20//25.08

# --sandbox reproduces Flathub's no-network build.
flatpak-builder --force-clean --sandbox --user --repo=repo build org.openpatch.classi.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest org.openpatch.classi.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

`--disable-rofiles-fuse` may be needed on filesystems where the fuse overlay
misbehaves.

The repo lint reports `appstream-screenshots-not-mirrored-in-ostree` and
`appstream-external-screenshot-url` on local builds. Both are
[expected](https://discourse.flathub.org/t/fixing-appstream-screenshots-not-mirrored-in-ostree-error/8116):
Flathub's CI mirrors screenshots itself.

## The two app-specific quirks

**Flutter needs clang; the base SDK only has gcc.** Flutter's Linux build
invokes CMake with `CC=clang`/`CXX=clang++` hard-coded
(`flutter_tools/lib/src/linux/build_linux.dart`) and the environment cannot
override it. Hence the `org.freedesktop.Sdk.Extension.llvm20` SDK extension.

**SQLite is normally downloaded, not built.** Classi stores its libraries in
SQLite encrypted with [SQLite3 Multiple
Ciphers](https://github.com/utelle/SQLite3MultipleCiphers). The app's
`pubspec.yaml` pins `hooks.user_defines.sqlite3.source: sqlite3mc`, which makes
the `sqlite3` package download a prebuilt `libsqlite3mc.so` at build time —
impossible offline, and not allowed here. The manifest therefore declares the
amalgamation as a pinned `archive` source and rewrites `pubspec.yaml` to
`source: source`, so the build hook compiles it instead.

That rewrite is asserted before it is applied (`grep -q` on the exact line), so
if the app ever changes that block the build fails loudly rather than silently
producing an unencrypted or differently-encrypted library.

The compiled library must produce **the same encrypted on-disk format** as the
prebuilt one, or a teacher moving between the AppImage and the Flatpak would
find their library unopenable — which looks exactly like a forgotten
passphrase. The app repository has `tool/check_cipher_compat.sh`, which writes a
database with each build and reads it back with the other, both directions. Run
it in the app repo whenever the `sqlite3` dependency changes.

Versions must stay aligned across three places:

| Where | What |
| --- | --- |
| the `sqlite3` package's `CHANGELOG.md` | the SQLite3 Multiple Ciphers version it ships prebuilt |
| the app's `tool/fetch_sqlite3mc.sh` | version + sha256 |
| `flatpak-flutter.yml` here | archive URL + sha256 |

## Permissions

| Permission | Reason |
| --- | --- |
| `--share=ipc`, `--socket=wayland`, `--socket=fallback-x11`, `--device=dri` | Drawing the window. |
| `--share=network` | WebDAV backup and restore, to a server the teacher configures. There is no Classi account and no Classi server. |
| `--talk-name=org.freedesktop.secrets` | `flutter_secure_storage` keeps the WebDAV password and the biometric-unlock passphrase in the Secret Service. |
| `--filesystem=xdg-documents` | Libraries default to `~/Documents/Classi` (path_provider's application documents directory). Without it the app cannot create or reopen its own default library, and users coming from the AppImage would not find their existing one. Libraries kept elsewhere are reached through the file chooser portal, so nothing broader is needed. |

## Self-updates are compiled out

Classi ships an in-app updater for its AppImage/dmg/exe builds. Flathub delivers
updates, and a self-updater would at best do nothing inside the sandbox, so the
build passes `--dart-define=CLASSI_DISABLE_SELF_UPDATE=true`.

## Verified

Against commit
[`ef637ce`](https://github.com/openpatch/classi/commit/ef637cea8c67ebf6f52d8a4a84f982dbead2b88f):

- `flatpak-builder --sandbox` build succeeds with no network access.
- `flatpak-builder-lint manifest` passes.
- `flatpak-builder-lint repo` reports only the two expected screenshot errors.
- `appstreamcli validate` passes on the metainfo.
- The bundled `libsqlite3.so` reports `SQLite3 Multiple Ciphers 2.5.0` /
  SQLite `3.53.4`, compiled in the sandbox, and exposes the `sqlcipher` cipher
  the app uses.
- The app installs from the built repo, launches, and stays running.
- Cipher compatibility holds in both directions between the prebuilt and
  source-built libraries.

## Before opening the PR

1. **Replace the screenshots.** The metainfo currently points at Classi's
   Android phone screenshots (1080×2424) so that it validates. Classi is listed
   as a desktop app; capture desktop-sized screenshots, commit them to the app
   repo on a tag, and point the metainfo at those. Reviewers do notice portrait
   phone shots on a desktop listing.
2. **Bump the pinned commit** to whatever release is being submitted, and add a
   matching `<release>` entry to the app's metainfo. The pinned commit currently
   ships app version 1.17.0 while the metainfo's newest `<release>` is 1.16.0.
3. **aarch64.** No `flathub.json` is included, so Flathub will build both
   `x86_64` and `aarch64`. Only `x86_64` has been built locally. If the arm64
   build fails, add:
   ```json
   { "only-arches": ["x86_64"] }
   ```
4. Open the PR against
   [flathub/flathub](https://github.com/flathub/flathub) from the
   `org.openpatch.classi` branch, following the [submission
   guide](https://docs.flathub.org/docs/for-app-authors/submission).
