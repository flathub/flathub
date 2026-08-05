# Updating to a new Inkdrop release

Once the app is published, most of this happens on its own — Flathub runs
flatpak-external-data-checker against `x-checker-data`, which downloads the new
artifact, computes its sha256 and opens the update PR. Follow this runbook when that
has not happened yet: the initial submission, a version to pin by hand, or a build to
test locally.

## 1. Point the manifest at the new version

`extra-data` accepts sha256 only, and upstream publishes sha512 — so the digest has to
be computed from the artifact. `update-manifest.py` does that here rather than in the
release pipeline, since Flathub is the only consumer that needs it:

```sh
./update-manifest.py              # follow the stable feed
./update-manifest.py 6.0.1        # pin a version
./update-manifest.py --dry-run    # print the values, write nothing
```

It resolves the version from `latest-linux.yml` / `latest-linux-arm64.yml` (erroring if
the two feeds disagree), streams each `.deb` once to get its sha256 and byte size, and
rewrites only the six `url` / `sha256` / `size` lines — comments, key order and the
nested `x-checker-data` are left alone. Python 3 stdlib only, no dependencies.

It refuses a pre-release unless given `--allow-prerelease`, because that is the one
mistake `flatpak-builder-lint` rejects outright.

## 2. Move the metainfo `<release>` to match

`update-manifest.py` warns when this is stale but does not edit the file. Add an entry
at the top of `<releases>` in `app.inkdrop.Inkdrop.metainfo.xml`:

```xml
<release version="6.0.1" date="2026-08-20">
  <description>
    <p>One or two sentences on what changed.</p>
  </description>
</release>
```

The date cannot be in the future, and releases must be ordered newest first.

## 3. Build, lint and run

```sh
flatpak install -y flathub org.flatpak.Builder
flatpak run org.flatpak.Builder --force-clean --repo=repo builddir app.inkdrop.Inkdrop.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest app.inkdrop.Inkdrop.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
flatpak remote-add --user --if-not-exists --no-gpg-verify inkdrop-local repo
flatpak install --user -y --reinstall inkdrop-local app.inkdrop.Inkdrop
flatpak run app.inkdrop.Inkdrop
```

Export to a repo and install from it — **not** `--install --user`. `org.flatpak.Builder`
is itself a flatpak, so it already holds a user namespace, and installing an
`extra-data` app means running `apply_extra` under a second, nested one. bwrap cannot
create it from in there and the install dies with:

```
bwrap: No permissions to create a new namespace, likely because the kernel does not
allow non-privileged user namespaces.
```

which is misleading — the host setting it names is almost certainly already correct, and
plain `bwrap` works fine outside the sandbox. Going through a repo puts the install back
in the hands of the host's flatpak, where the namespace is a fresh one. The `remote-add`
is a one-off; after that a rebuild is just the `--repo` line plus `install --reinstall`,
which picks up the new commit.

Two side effects worth knowing. The lint step above needs `repo/` to exist, so it only
ever worked in this order. And ostree refuses to write into a repo when the filesystem
is under `min-free-space-percent`, 3% by default — on a large and nearly full disk that
reserve is gigabytes, and the export fails with `min-free-space-percent '3%' would be
exceeded, at least 4.1 kB requested` while df still shows free space. Free some, or
set `ostree --repo=repo config set core.min-free-space-percent 0` on this throwaway repo.

Until Flathub grants the exceptions, the manifest lint fails with
`finish-args-home-filesystem-access` and `finish-args-login1-system-talk-name` — see
[BUILD.md](BUILD.md#linter-exceptions-needed) for the local `--exceptions
--user-exceptions` workaround and the justification for each.

Confirm the window actually **renders**, not just that the process starts. A window that
opens and paints nothing is the failure mode this package has hit before, and the main
process logs a clean startup while it happens.

## Using the official checker instead

flatpak-external-data-checker replaces step 1 entirely and runs locally from its
container image:

```sh
./run-in-container.sh --update app.inkdrop.Inkdrop.yml   # from the checker's repo
```

It is the same tool Flathub runs, so `update-manifest.py` can be dropped if you would
rather not maintain it. It needs podman or docker; `update-manifest.py` needs neither.
