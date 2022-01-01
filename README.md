# org.polymc.PolyMC

A flatpak for [PolyMC](https://github.com/PolyMC/PolyMC)

### Info

PolyMC is a custom launcher for Minecraft that focuses on predictability, long term stability and simplicity.

This is a **fork** of the MultiMC Launcher and not endorsed by MultiMC. The PolyMC community felt that the maintainer was not acting in the spirit of Free Software so this fork was made. Read "[Why was this fork made?](https://github.com/PolyMC/PolyMC/wiki/FAQ)" on the wiki for more details.

### Included Java Versions

8, 11, 16, 17. Play any minecraft version!

### FAQ

Q: How do I run MC on a Hybrid-GPU system with a dedicated nvidia GPU using PolyMC?  
A: The flatpak includes a `prime-run` script, which when set as the wrapper command in instance settings, runs MC using the Nvidia GPU.

Q: Does it work with Wayland?  
A: Yes! This includes a patched version of the library minecraft uses GLFW. To enable wayland support without XWayland, enable `use system GLFW` in the settings of the instance in PolyMC.
