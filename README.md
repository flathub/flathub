Flatpak manifest for GitHub Desktop Plus app
=======================================

This repository contains the files to create a Flatpak version of [GitHub Desktop Plus](https://github.com/pol-rivero/github-desktop-plus), a GitHub Desktop fork with advanced functionality and Bitbucket integration.

Keep in mind that this NOT the official GitHub Desktop app and I am not affiliated with GitHub in any way.

Known Issues
------------

- `Show in your File Manager` does not open the file manager

    This happens because the default manager is not set in your environment. You need to add a default file manager to your `~/.config/mimeapps.list` file. If you are using nautilus, this can be done by adding `inode/directory=org.gnome.Nautilus.desktop` to the end of the `[Default Applications]` section.

- `Open in Terminal` does not open the terminal

    To fix this, simply go to File -> Options -> Integrations, in the *Shell* dropdown select "Configure custom shell..." and then change the dropdown back to your preferred terminal emulator.

- Git Hooks that spawn external programs do not work. This is non-fixable without a massive rewrite inside git to make it possible to spawn git hooks outside the flatpak container.


Installation
------------

To build and install this Flatpak, you have to [install Flatpak, Flatpak builder and the Flathub repo](https://flatpak.org/setup/). Don't forget to initialize this repo submodules. Then run:

```sh
flatpak-builder build io.github.pol_rivero.DesktopPlus.yaml --repo=repo --install --force-clean --install-deps-from=flathub
```

Once installed, launch GitHub Desktop Plus by running:

```sh
flatpak run io.github.pol_rivero.DesktopPlus
```

Updating `github-desktop-plus` repo and dependencies
----------------------------------------

Flatpak builder doesn't allow the build scripts to access the internet, so you have to download all the required dependencies beforehand. These dependencies are listed in the `generated-sources.json` file. That's the reason we have a fixed commit for building `github-desktop-plus` repo, since that can guarantee that `generated-sources.json` dependencies match with the version of `github-desktop-plus` we are building.

To update `github-desktop-plus` repo to its latest commit and update the dependencies, you have to:

1. (If needed) Clone [https://github.com/pol-rivero/github-desktop-plus](https://github.com/pol-rivero/github-desktop-plus) inside this repo.

    ```sh
    git clone https://github.com/pol-rivero/github-desktop-plus.git
    ```

1. Checkout the correct commit:

    ```sh
    cd github-desktop-plus
    git checkout <commit hash or tag>
    ```

1. Change the commit in `io.github.pol_rivero.DesktopPlus.yaml` to the desired one:

    ```yaml
    ...
          - type: git
            url: https://github.com/pol-rivero/github-desktop-plus.git
            tag: <tag name>
            commit: <commit hash>
    ...
    ```

1. If needed, create the venv

    ```sh
    python3 -m venv .venv
    .venv/bin/pip install -r requirements.txt
    ```

1. Run `generate-sources` script to update `generated-sources.json`.

1. Make sure the patches in the `patches` directory still apply.

1. Once you are sure it works, make a PR with the changes.
