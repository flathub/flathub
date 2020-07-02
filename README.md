# Zyn-Fusion Flatpak
This allows to build the amazing ZynAddSubFx software synthesizer (https://github.com/zynaddsubfx/zynaddsubfx) with Zyn-Fusion user interface as Lv2 plugin installable as a Flatpak. The Plugin can then be used in Flatpak builds of Ardour, LMMS (via Carla rack, in this case) and probably other Lv2 hosts. Those are available via Flathub. Seems to be working.

I am not sure if I am supposed to use this app ID - I am not affiliated whith freedesktop.org or Linux Audio Projects in any way. However, this is building using https://github.com/flathub/org.freedesktop.LinuxAudio.BaseExtension and in turn depends on org.freedesktop.Platform runtime.
I am not very familiar with practices and conventions when it comes to building Flatpaks - that's the first I have ever built.

# Why
Recently I've got interested in music production, and started playing around with lmms and Ardour. Versions of both application shipped by my distribution of choice (Fedora) are outdated (and ZynAddSubFx with Fusion UI is not available at all). So I've installed those from Flathub. Then I realised that I cannot use any plugins installed via OS packages. Also, while lmms bundles ZynAddSubfx - it's some ancient release with a truly horible UI.
Reading around a bit I found Linux Audio plugins in github, and realised that plugins too can be installed as Flatpaks. There's a decent collection of plugins available this way, however Zyn in missing. I've built Zyn-Fusion from source and decided to try to build a Flatpak.

I firmly believe that technologies like Flatpak are the future of software distribution in Linux as it finally makes Linux somewhat civilised OS. User should be able to install latest and greatest software on the distribution of choice withoug going into a week-long hacking session, user should be able to have multiple versions of software without some horrible hacks, etc. I did spend couple of nights creating this working flatpak manifest, so you don't have to.

Next I intend to write a Flatpak manifest for the Calf Studio Gear Plugins.

# How
* Obtain flatpak-builder - should be available in any modern distribution
* Clone the repo
* Install org.freedesktop.Sdk (branch 19.08) and org.freedesktop.LinuxAudio.BaseExtension :

```
flatpak install org.freedesktop.Sdk//19.08
flatpak install org.freedesktop.LinuxAudio.BaseExtension//19.08
```

* build the Flatpak:

```flatpak-builder zyn-build --repo=zyn-fusion org.freedesktop.LinuxAudio.Lv2Plugins.ZynFusion.json```

Repo and build directory here can be changed to whatever. All of the build is happening in the sandbox, nothing in your system is affected by it. You do not need and MUST not use sudo or any other priviledge escalation mechanism.

* If you'd like to have a bundle (single Flatpak file), run:

```flatpak build-bundle --runtime ./zyn-fusion/ zyn-fusion.flatpak org.freedesktop.LinuxAudio.Lv2Plugins.ZynFusion 19.08```

Here zyn-fusion should be whatever you used as a --repo flag for building. Name of the .flatpak file can be whatever.

* Bundle can then be installed on any Linux computer like:

```flatpak install zyn-fusion.flatpak```

Obviously - file name/path has to be correct.

# Technical Stuff

It ain't pretty. It has to build Python 2.7 because it is needed by Zest's build process and apparently new SDKs no longer have it. Python is not really used by the plugin itsef in any way, so everything Python is removed from the Flatpak in the clean up stage. That's the only workaround I could think of.

Couple dependencies are bundled (libmxml and Zest), probbably could be split into shared modules if any other stuff needs those.

ZynAddSubFx standalone and VST plugins are also being built in the process, removed in the clean up stage.

# Not Ideal

Of course nobody should be building this stuff (or building anything) just to able to use some plugin in a DAW. It would be much better if this Flatpak would be available from Flathub or any other major distributor. However I don't know anything about the submitting process. Hopefully someone can grab this manifest, maybe improve it and build for proper distribution. 
