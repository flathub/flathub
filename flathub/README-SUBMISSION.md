# Silent Client — Flathub packaging

Community Flatpak of the Silent Client Minecraft launcher, ID
`net.silentclient.launcher`. Built from the official universal Linux AppImage,
downloaded from the Silent Client CDN at install time (extra-data), so the
binary is not stored in this repository.

## Files

    net.silentclient.launcher.yml         manifest
    net.silentclient.launcher.metainfo.xml app metadata
    net.silentclient.launcher.desktop     desktop entry
    net.silentclient.launcher.png         512x512 icon
    net.silentclient.launcher-128.png     128x128 icon
    flathub.json                          x86_64 only

## Build and check

    flatpak-builder --user --install --force-clean build-dir net.silentclient.launcher.yml
    flatpak run net.silentclient.launcher

    flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
        manifest net.silentclient.launcher.yml
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
        repo build-dir

    appstreamcli validate --pedantic net.silentclient.launcher.metainfo.xml

The last command warns about the screenshot URL until the preview image is
online (see below); everything else is clean.

## Before submitting

1. **Screenshot.** `preview.png` has to be uploaded to
   `https://silentclient.net/sc-media/cdn/silentclient-assets/site/videos/`
   (next to the video) or the URL in the metainfo corrected. A 16:9 image of
   the launcher window is fine. Do not commit the PNG into the repository.
2. **Video (optional).** The metainfo points at `bg-hud.mp4` on the CDN, which
   works. AppStream prefers VP9 in WebM; `bg-hud.webm` was converted for that,
   and swapping the `<video>` tag to `codec="vp9" container="webm"` is enough
   once it is uploaded.
3. Fork `flathub/flathub`, branch `new-pr`, copy the files above into the repo
   root, open a PR titled `Add net.silentclient.launcher`. Keep it as a draft
   until the build passes.

## Things to know

- **Updates.** The launcher ships an auto-updater that replaces the AppImage it
  runs from. Inside Flatpak `/app` is read-only, so the wrapper unsets `APPIMAGE`
  and the updater takes its "download the new AppImage yourself" path instead of
  touching the install. Updates come from Flathub: the `x-checker-data` block
  follows `https://api.silentclient.net/...` and bumps url/sha256/size, and
  `<releases>` in the metainfo needs the same version added when it does.
- **Auto-updating `<releases>`.** Flathub requires a release entry per version;
  the checker does not edit the metainfo for you.
- **Architectures.** Only the x86_64 build is published upstream, hence
  `flathub.json`.
- **License.** Proprietary; the metainfo uses a `LicenseRef` pointing at the
  Silent Client terms. Flathub will ask for permission from upstream before
  merging a package of non-free software.
- **App ID.** Cannot be changed after merge.
- **Local previews.** Opening the metainfo file directly in Discover does not
  show screenshots or video — Discover renders AppStream videos poorly and only
  resolves the icon from the installed theme. Install the Flatpak to see the
  real page.
