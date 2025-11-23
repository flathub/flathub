# ungoogled-cef
This is a custom build of CEF (Chromium Embedded Framework) but with the ungoogled-chromium patchset applied to it, thus removing all the Google telemetry. Also see build.sh for a list of buildflag overrides; most features unnecessary for simple use-cases are turned off. The current version is 139.0.7258.139.

Looking to use ungoogled-cef outside of flatpak? The build steps here are reproducible and in theory bootstrappable. Alternatively, I have pre-built debug and release distributions available [here](https://adamcake.com/cef).

## Usage
```yaml
base: com.adamcake.ungoogled_cef.BaseApp
base-version: '25.08'
```

When you add this baseapp to your manifest, a binary distribution of CEF will appear in `/app/cef/dist`, which will have the same file structure as the official "minimal" builds of CEF. ("minimal" means you can't build in Debug mode.)

Usually when installing with a CEF binary distribution, you'll copy files out of that directory and into a different file structure. If you're doing that, you don't want `/app/cef` to still be present for all users, otherwise they'll needlessly have two copies of libcef.so on their PC. So don't forget to add this to your manifest:
```yaml
cleanup-commands:
  - rm -rf /app/cef
```
(NB: adding `/cef` to `cleanup` doesn't work, but this does.)

## Maintenance
Updating this to a newer CEF version is a monumental effort usually best approached from-scratch, and for that reason I don't do it very often.

If you want to try it for yourself, my recommendation would be to set up a local build of CEF using the [official instructions](https://bitbucket.org/chromiumembedded/cef/wiki/MasterBuildQuickStart), then try your best to come up with a working build, seeing which patches are necessary to get it to work. The existing patches in this repo will be a good place to start, but you probably won't need all of them, and the ones you do need will probably not apply to the new chromium version and will need to be re-generated. You will need to create new patches to fix new problems. You will most likely need to resolve conflicts between the CEF patchset and the ungoogled-chromium patchset, which is why this build patches the ungoogled-chromium repo before using it.

Once you've followed the official instructions far enough to have a build directory, you can get a list of all possible GN_DEFINES flags by running: `gn args --list out/Release_GN_x64` (or whatever any folder in `out/` is called). This will list every available buildflag from every subproject. However, not all of them are expected to be overridden directly and might cause the build to fail. If in doubt you should check the .gn file where the flag is defined and try to see if it inherits its value from somewhere else. The build flags used by this build are listed in build.sh, and were chosen mainly to turn off unwanted features such as debug checks/features, tests, and features that aren't useful for normal desktop app usage - that means things like media remoting and service discovery. Changing build flags can also fix or circumvent build errors in some cases.

Unlike the official build instructions, this build can't use depot_tools due to the fact that it would try to download things during the build step, which flatpak doesn't allow. (It also has a lot of Google binaries in it which we don't want to use.) Therefore, the way we clone and setup the sources for compilation is completely different. The chromium repo has a huge tree of submodules which are cloned conditionally; we have `generate-submodule-sources.py` to automatically generate a list of sources in flatpak format, `chromium-sybmodules.yaml`. This file is then simply included in the main .yml file so that flatpak will clone all the needed git repos into the correct places during setup. There's also a list of npm packages which need to be installed in `third_party/node/node_modules`, so by the same concept we have `generate-node-sources.py` which creates `third-party-node-modules.yaml`. There are a couple of in-tree patches which would have been applied by depot_tools, so these are applied at the start of build.sh.

As well as the automatically-generated sources, don't forget to update this repo's submodules, and the non-automatically-generated build sources. If there's a version specific to the Chromium tag you're trying to build (as there is with the ungoogled-chromium repo, for example), you'll need to use that, otherwise you should use either the latest commit or latest stable version.

Good luck. Don't expect this to be a smooth process.

## Thanks to:
- `nmlynch94`: hard work and persistence
