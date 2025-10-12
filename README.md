# Helium Flatpak

A community-maintained Flatpak package for [Helium](https://helium.computer).

## Extension Points

To avoid exposing more of the host file system while still allowing Helium extensions, the following extension points are defined:

- `net.imput.Helium.Policy`
- `net.imput.Helium.Extension`
- `net.imput.Helium.NativeMessagingHost`

### net.imput.Helium.Policy

Configure custom Chromium policies. This extension point is on version '1' and makes any policy under the `policies/managed` and `policies/recommended` subdirectories available to Helium.

### net.imput.Helium.Extension

Install Chromium extensions. This extension point is on version '1' and makes any extension under the `extensions` subdirectory available to Helium.

### net.imput.Helium.NativeMessagingHost

Add [native messaging host](https://developer.chrome.com/docs/apps/nativeMessaging/) support. This extension point is on version '1' and exposes the `native-messaging-hosts` subdirectory to Helium.

### Using Extension Points

Extension points can be provided as regular Flatpaks. The extension point name must follow the syntax `<ExtensionPointName>.<id>`, where `<ExtensionPointName>` is one of the supported extension points above and `<id>` is a generic ID for this specific extension point.

Flatpak also supports "unmanaged extensions", allowing loading extensions installed into `/var/lib/flatpak/extension` and `$XDG_DATA_HOME/flatpak/extension`.

## Important Notes

- The traditional `~/.config/net.imput.helium` directory is **not accessible** to the Flatpak version. Use the sandboxed path `~/.var/app/net.imput.Helium/config/net.imput.helium` instead.
- After making configuration changes, run `flatpak kill net.imput.Helium` to ensure Helium is properly restarted

## Troubleshooting

### Common Issues

1. **PWA (Progressive Web App) installation not working**

    By default, PWA installation is disabled for security reasons. If you need to install PWAs, you can enable the required permissions:

    ```bash
    flatpak override --user --filesystem=xdg-desktop --filesystem=~/.local/share/applications --filesystem=~/.local/share/icons net.imput.Helium
    ```

2. **Download directory issues**

    The Flatpak has access to `~/Downloads` by default. For other directories, you may need to grant additional permissions.

3. **Extensions or policies not loading**

    Ensure extension points are properly installed and restart Helium:

    ```bash
    flatpak kill net.imput.Helium
    flatpak run net.imput.Helium
    ```
