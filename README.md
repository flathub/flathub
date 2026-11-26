# Adalet E-İmza

Flatpak package of Adalet E-İmza, the Turkish Ministry of Justice's helper that lets
browser-based UYAP systems sign with the certificate on a smart card or USB token.

## Using it

```bash
flatpak install flathub tr.gov.adalet.Eimza
flatpak run tr.gov.adalet.Eimza
```

- The host needs a running `pcscd` and the driver for your card reader.
- The app serves the browser on `127.0.0.1:5975`. Remove the ministry's deb or
  AppImage first, otherwise both try to use that port.
- It does not start at login. Launch it before signing.

## Building

```bash
git clone --recurse-submodules https://github.com/flathub/tr.gov.adalet.Eimza.git
cd tr.gov.adalet.Eimza
flatpak run --command=flathub-build org.flatpak.Builder tr.gov.adalet.Eimza.yaml
flatpak install --user -y ./repo tr.gov.adalet.Eimza
```

## Updating

The app runs on Flathub's OpenJDK instead of the launcher and Java bundled by the
ministry, so `platform/adalet-eimza-tray.sh` repeats the launcher's JVM options.
Before merging a version bump, check that they have not changed:

```bash
tools/launcher-options.sh | diff tools/launcher-options.txt -
```
