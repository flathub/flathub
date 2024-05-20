# org.unmojang.FjordLauncher

A flatpak for [FjordLauncher](https://github.com/unmojang/FjordLauncher)
Please see upstream README for more information.

## FAQ

### How to use MangoHud

1\. Install MangoHud for Flatpak apps with the following command:

```
flatpak install org.freedesktop.Platform.VulkanLayer.MangoHud//23.08
```

2\. Open Fjord, click on the **Settings** button

3\. Select **Minecraft** from the left panel

4\. Select the **Tweaks** tab

5\. Tick the **Enable MangoHud** checkbox

That's it!

### How do I run MC on a hybrid GPU system with a dedicated NVIDIA GPU using Fjord Launcher?

The flatpak includes a `prime-run` script, which when set as the wrapper command in instance settings, runs MC using the Nvidia GPU.  
Soon this should be unneccesary, as it has been fixed in the master branch of the launcher already.
