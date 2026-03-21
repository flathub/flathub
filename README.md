# Root Flatpak

> **Note:** This wrapper is not verified by, affiliated with, or supported by Root Communications Inc. It is maintained by the community.

This is the Flatpak package for [Root](https://www.rootapp.com/).

## Building

To build this Flatpak package locally, run:

```bash
flatpak-builder build com.rootapp.Root.yml --force-clean --ccache --user --install
```

## Known Issues (Upstream)

The following issues are known upstream bugs in the Linux/Avalonia version of the Root App and are **not** caused by this Flatpak wrapper:

*   **File Uploads:** Uploading files via the app's UI picker currently fails.
*   **Webcam Screen Sharing:** Sharing the screen to the camera is not working.
*   **Wayland Support:** Native Wayland is not supported yet and is forced to run via XWayland. This is due to [AvaloniaUI Issue #1243](https://github.com/AvaloniaUI/Avalonia/issues/1243).

## Issues

If you find an issue that is specific to the Flatpak packaging itself (and not the Root App bugs mentioned above), please open an issue in this repository.
