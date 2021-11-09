# Threema Desktop Electron App

This follows the upstream documentation in the README [1].
Note that this repository contains the actual web app as a submodule which we
need to declare as a separate dependency.

Further, we use `linux:deb` in various build scripts, but outside of
the actual `package` script, the `:deb` prefix doesn't do anything debian
specific.

Since the app uses Electron we use the template and follow the documentation
here [2] to pre-fetch all dependencies.
As the actual application and its `package-lock.json` is distributed over
multiple files (and repos), we create the `generated-sources.json` file by
cloning the repo with all submodules,
and recursively (`-r`) collecting the info of all relevant dependency files
using the flatpak-builder-tools [3]:

    git clone --recurse-submodules https://github.com/threema-ch/threema-web-electron.git
    flatpak-node-generator npm -r --xdg-layout --electron-node-headers \
            package-lock.json

We also need to add the electron headers, otherwise the postinstall of electron
will try to fetch additional dependencies in the build step as well.

[1]: https://github.com/threema-ch/threema-web-electron/
[2]: https://docs.flatpak.org/en/latest/electron.html
[3]: https://github.com/flatpak/flatpak-builder-tools/
