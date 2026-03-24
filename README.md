# Root Flatpak

> **Note:** This wrapper is not verified by, affiliated with, or supported by Root Communications Inc. It is maintained by the community.

This is the Flatpak package for [Root](https://www.rootapp.com/).

## Building

To build this Flatpak package locally, run:

```bash
flatpak-builder build com.rootapp.Root.yml --force-clean --ccache --user --install
```

## File Access

By default, this Flatpak package is configured with specific XDG directories (`xdg-download`, `xdg-documents`, `xdg-pictures`, `xdg-videos`) to provide secure file access for upload/download operations while maintaining user privacy.

This approach grants the application access to standard user directories without exposing your entire host filesystem.

### If you experience file picker issues

If you encounter problems with drag-and-drop, copy-paste, or the native file picker not working correctly, you can grant full host filesystem access by running:

```bash
flatpak override --filesystem=host com.rootapp.Root
```

**Note:** This will grant the application access to your entire host filesystem, which may pose security risks. Use this option only if necessary for proper functionality.

### Custom directory access

You can also grant access to specific custom directories if needed:

```bash
flatpak override --filesystem=/path/to/your/directory com.rootapp.Root
```

## Known Issues (Upstream)

The following issues are known upstream bugs in the Linux/Avalonia version of the Root App and are **not** caused by this Flatpak wrapper:

- **Webcam Screen Sharing:** Sharing the screen to the camera is not working.
- **Wayland Support:** Native Wayland is not supported yet and is forced to run via XWayland. This is due to [AvaloniaUI Issue #1243](https://github.com/AvaloniaUI/Avalonia/issues/1243).

## Issues

If you find an issue that is specific to the Flatpak packaging itself (and not the Root App bugs mentioned above), please open an issue in this repository.
