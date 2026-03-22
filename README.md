# Root Flatpak

> **Note:** This wrapper is not verified by, affiliated with, or supported by Root Communications Inc. It is maintained by the community.

This is the Flatpak package for [Root](https://www.rootapp.com/).

## Building

To build this Flatpak package locally, run:

```bash
flatpak-builder build com.rootapp.Root.yml --force-clean --ccache --user --install
```

## File Access

By default, this Flatpak package is configured with `--filesystem=host` to ensure that drag-and-drop, copy-paste, and the native file picker work correctly. This grants the application access to your host filesystem.

If you prefer to restrict the application's access for security reasons, you can override this permission. For example, to restrict access strictly to your Downloads folder, you can run:

```bash
flatpak override --nofilesystem=host --filesystem=xdg-download com.rootapp.Root
```
*(Note: Restricting filesystem access will break the in-app file picker)*

## Known Issues (Upstream)

The following issues are known upstream bugs in the Linux/Avalonia version of the Root App and are **not** caused by this Flatpak wrapper:

*   **Webcam Screen Sharing:** Sharing the screen to the camera is not working.
*   **Wayland Support:** Native Wayland is not supported yet and is forced to run via XWayland. This is due to [AvaloniaUI Issue #1243](https://github.com/AvaloniaUI/Avalonia/issues/1243).

## Issues

If you find an issue that is specific to the Flatpak packaging itself (and not the Root App bugs mentioned above), please open an issue in this repository.
