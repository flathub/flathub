Vulkan Capture for OBS
======================

Tools to capture Vulkan/OpenGL applications with OBS. For instructions on how to use, see [this page](https://github.com/nowrep/obs-vkcapture#usage).

For this to work you also need the corresponding OBS VkCapture plugin for OBS Studio. This plugin is available as [`com.obsproject.Studio.Plugin.OBSVkCapture` on Flathub](https://github.com/flathub/com.obsproject.Studio.Plugin.OBSVkCapture).

Permissions
-----------

The capture tools communicate with OBS via an abstract socket. Therefore it is necessary for both OBS Studio and the captured application to have networking permissions. Luckily OBS Studio already has this, as well as Steam.

To check if the application you're trying to capture has permission to use networking, use the following command:
```
flatpak info --show-permissions <APP ID>
```
where `<APP ID>` is the ID of the application that you want to capture.
If your output contains a line looking like `shared=network,...`, then
networking is already enabled for that application.

If the application you're trying to capture does not currently have networking permissions, you can add them with the following command:
```
flatpak override --share=network <APP ID>
```