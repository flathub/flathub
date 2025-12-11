# so.onekey.wallet

Official Flatpak packaging assets for OneKey Wallet (extra-data / upstream AppImage repack). Before opening a PR to flathub/flathub, verify:

1. `so.onekey.wallet.yml` extra-data URLs/size/sha256 match the latest published AppImage (x86_64 / aarch64).
2. `so.onekey.wallet.metainfo.xml` has the correct version/date, changelog, screenshots, and license; `appstream-util validate-relax so.onekey.wallet.metainfo.xml`.
3. Local validation succeeds:
   - `flatpak-builder --user --force-clean build-dir so.onekey.wallet.yml`
   - `flatpak-builder --run build-dir so.onekey.wallet.yml onekey-wallet`
4. Include permission rationale in the PR description: `--device=all`, `--socket=pcsc`, `--socket=pulseaudio`, `--share=network/ipc`, `--socket=x11/wayland`, notifications, and the optional Wayland flags toggled via `USE_WAYLAND=1`.

Notes:
- Enable Wayland: `flatpak override --user --env=USE_WAYLAND=1 so.onekey.wallet`.
- Hardware wallets: host PCSC daemon (`pcscd`) must be running; sandbox access is granted via `--socket=pcsc` and `--device=all`. We rely on the host pcscd (no bundled pcsc-lite) and the upstream AppImage via extra-data.
- Rationale for PCSC and devices: the app uses pcsc-lite to reach hardware wallets through host `pcscd`; `--socket=pcsc` opens that channel. `--device=all` stays to cover smartcard/USB/HID paths used by pcscd. This matches other hardware-wallet apps on Flathub. Bridge/HTTP flows still require network/IP sockets, already covered by `--share=network/ipc`.
