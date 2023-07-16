# SDK Extension for Gamescope

The successor to the [Steam extension](https://github.com/flathub/com.valvesoftware.Steam.Utility.gamescope) containing the same utility.

This will allow Gamescope to function on non-x86 architectures, something useful for Flatpaks such as [Prism Launcher](https://flathub.org/apps/org.prismlauncher.PrismLauncher).

This is based on the [extension made by tinywrkb](https://github.com/tinywrkb/org.freedesktop.Platform.VulkanLayer.GameScope). As such, here are some relevant bits from the previous README:

> The Freedesktop runtime defines an extension mount point that bind-mounts any extension with the `org.freedesktop.Platform.VulkanLayer` prefix onto a subdirectory of `/usr/lib/extensions/vulkan`.
This gamescope extension takes advantage of the above, and adds gamescope and it supporting files to the runtime under `/usr/lib/extensions/vulkan/gamescope`.

> Yes, this is a hack, as this specific extension mount point was likely not intended to be used with this kind of tool, and submission of this extension to Flathub might not be approved due to this fact, but that's good enough for my needs.

> The extension definition doesn't include the add-ld-path key, and there's no extension key to append a directory to PATH, so we will need to take care of both of these in a different way.

### Notes

Here, we include a script that automatically adds the appropriate paths, so all you need to do is add `/usr/lib/extensions/vulkan/gamescope/bin` to your Flatpak's PATH variable (`--env=PATH=$PATH:/usr/lib/extensions/vulkan/gamescope/bin`).

The script is named `gamescope`, so it should be a drop-in replacement. 

It should also be noted that this extension introduces a couple of shared libraries that will possibly conflict with those provided by an application, if it does packages the same libraries.
This includes, but is not limited to, `libevdev`, `libfontenc`, `libinput`, `libliftoff`, `libmtdev`, `libseat`, `libwlroots`, `libxcvt`, `libXfont2`, and `libXRes`.

If you bundle any of these and rely on gamescope, look at including this just as you would [shared-modules](https://github.com/flathub/shared-modules/); the layout is made such that you can (mostly) pick and choose what you want. You *will* need to build gamescope (and its specified dependencies) manually, though. 

Or remove what you're bundling, or do what you already do, and risk it; it's all up to you.
