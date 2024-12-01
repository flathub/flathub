# GitFourchette Flatpak

This is the source for [GitFourchette](https://github.com/jorio/gitfourchette)'s Flatpak distribution.

If you stumbled here looking to install the Flatpak for GitFourchette, simply run:

```sh
flatpak install org.gitfourchette.gitfourchette
```

## Flatpak maintenance notes

### Build the Flatpak locally

```sh
flatpak-builder -v --install-deps-from=flathub --user --install --force-clean build org.gitfourchette.gitfourchette.yml
```

### How to update the dependencies

1. Install the KDE Flatpak SDK corresponding to the newest runtime supported by PyQt.BaseApp.
    - Run `flatpak install org.kde.Sdk` and pick [the latest version supported by PyQt.BaseApp](https://github.com/flathub/com.riverbankcomputing.PyQt.BaseApp#branch-comparison).
    - This is not necessarily the latest available release of the KDE runtime.

2. In [org.gitfourchette.gitfourchette.yml](./org.gitfourchette.gitfourchette.yml), replace `runtime-version` and `base-version` with the version of the runtime you just installed.

3. Note the Python version that is included in the runtime:
    - `flatpak run --command=python3 org.kde.Sdk --version`

4. Install `pygit2` on your machine (e.g. via `pip`) so we can get the latest Python dependencies.

5. Run `pipdeptree -p pygit2` and note the versions of the Python dependencies.
    - We're just interested in `pygit2`, `cffi`, and `pycparser`.
    - You can obtain `pipdeptree` from `pip`.

6. Regenerate [python3-packages.yml](./python3-packages.yml):
    - `req2flatpak --yaml --requirements pygit2==1.16.0 cffi==1.17.1 pycparser==2.22 -t 311-aarch64 311-x86_64 > python3-packages.yml`
    - Replace Python version number `311` with the Python version from the runtime (step 3).
    - Replace the package versions with what you noted in step 5.
    - You can obtain `req2flatpak` from `pip`.

7. You can now rebuild the Flatpak.
