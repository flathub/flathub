# Note To Maintainers

Regarding `io.sourceforge.pysolfc.PySolFC.json`:

1. `tkinter.json` is simply copied verbatim from
   <https://github.com/iwalton3/tkinter-standalone> with `x-checker-data`
   sections added (I've opened
   [a bug](https://github.com/iwalton3/tkinter-standalone/issues/4) about them
   being absent) because Tkinter is sort of in limbo in the Freedesktop runtime,
   with Python being too common a dependency to omit, but Tkinter being too rare
   a dependency for its size to be included by default, and both being part of
   the same source package.
2. `python3-modules.json` was produced by running the
   [flatpak-pip-generator](https://github.com/flatpak/flatpak-builder-tools/blob/master/pip/flatpak-pip-generator)
   script as
   `python3 flatpak-pip-generator --checker-data attrs configobj pillow pycotap 'pygame>=2' ttkthemes pysol-cards`
3. `solvers_extra_deps.json` was produced by running the
   [flatpak-cpan-generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/cpan)
   script as
   `./flatpak-cpan-generator.pl -d solvers_extra_deps -o solvers_extra_deps.json Moo Path::Tiny Template`.

   Note that, unlike `flatpak-pip-generator`, this produces a bare sources list,
   not a complete module section, and the includes for them differ accordingly.
   As this script does not support generating `x-checker-data` entries for me
   and I was thoroughly disillusioned with getting the solvers to build by this
   point, it's up to you to decide whether you want to add them manually.

4. There's no version field because Flathub assumes the newest version listed in
   the `.appdata.xml` file is the version you're publishing. I don't know if it
   automatically filters out versions marked as development versions to only be
   displayed by tooling when you're asking for the development build channel,
   but I wouldn't be surprised, given how it automatically splits out
   localization data and debug symbols when it recognizes them and "magically do
   the right thing by default" seems to be a running theme with Flatpak tooling.

`x-checker-data` serves two purposes:

1. If you've set up Flathub as a package source with ID `flathub` (what the
   quick start instructions guide you through), then you can
   `flatpak install flathub org.flathub.flatpak-external-data-checker` to
   install `flatpak-external-data-checker` locally and then
   `flatpak run org.flathub.flatpak-external-data-checker io.sourceforge.pysolfc.PySolFC.json`
   whenever you want to automatically check for new releases of your
   dependencies.

   (Flatpak treats command-line packages sort of like how Rust treats
   `cargo install`. They don't show up in the web catalogue and are meant more
   as a means for distributing developer tools. I wrote
   [a script](https://gist.github.com/ssokolow/db565fd8a82d6002baada946adb81f68)
   which retrofits regular command names onto them.)

2. Flathub's bot on GitHub will use it to automatically detect when your
   dependencies get updated and submit PRs to bump your release. Since Flathub
   will also run a buildbot run on any PRs, you can incorporate whatever
   automated testing you want into your Flatpak build process and then have the
   pass/fail show up right in the PR to streamline evaluating version bumps.
