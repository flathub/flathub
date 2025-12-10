# so.onekey.wallet.desktop

Flatpak packaging assets for the Flathub submission (extra-data / AppImage repack). Before opening a PR to flathub/flathub, verify:

1. `so.onekey.wallet.desktop.yml` extra-data URLs/size/sha256 match the latest published AppImage (all arches in use).
2. `so.onekey.wallet.desktop.appdata.xml` has the correct version/date, changelog, screenshots, and license.
3. Local validation succeeds:
   - `appstream-util validate-relax so.onekey.wallet.desktop.appdata.xml`
   - `flatpak-builder --user --force-clean build-dir so.onekey.wallet.desktop.yml`
   - `flatpak-builder --run build-dir so.onekey.wallet.desktop.yml onekey-wallet`
4. Include permission rationale (device=all, xdg-download/documents/desktop, notifications, portals) and build logs in the PR description.

