Flatpak manifest for Desktop Plus app
=======================================

This repository contains the files to create a Flatpak version of [Desktop Plus](https://github.com/desktop-plus/desktop-plus), a GitHub Desktop fork with advanced functionality and Bitbucket integration.

Keep in mind that this NOT the official GitHub Desktop app and I am not affiliated with GitHub in any way.

> [!NOTE]
> This application was previously published on Flathub as `io.github.pol_rivero.github-desktop-plus`. It has been renamed to `org.desktop_plus.desktop-plus`.

Known Issues
------------

- `Show in your File Manager` does not open the file manager

    This happens because the default manager is not set in your environment. You need to add a default file manager to your `~/.config/mimeapps.list` file. If you are using nautilus, this can be done by adding `inode/directory=org.gnome.Nautilus.desktop` to the end of the `[Default Applications]` section.

- `Open in Terminal` does not open the terminal

    To fix this, simply go to File -> Options -> Integrations, in the *Shell* dropdown select "Configure custom shell..." and then change the dropdown back to your preferred terminal emulator.

- Git Hooks that spawn external programs do not work. This is non-fixable without a massive rewrite inside git to make it possible to spawn git hooks outside the flatpak container.


Installation
------------

To build and install this Flatpak, you have to [install Flatpak, Flatpak builder and the Flathub repo](https://flatpak.org/setup/).

```sh
flatpak-builder build org.desktop_plus.desktop-plus.yaml --repo=repo --install --force-clean --install-deps-from=flathub
```

Once installed, launch Desktop Plus by running:

```sh
flatpak run org.desktop_plus.desktop-plus
```

Updating `desktop-plus` repo and dependencies
----------------------------------------

Flatpak builder doesn't allow the build scripts to access the internet, so you have to download all the required dependencies beforehand. These dependencies are listed in the `generated-sources.json` file. That's the reason we have a fixed commit for building `desktop-plus` repo, since that can guarantee that `generated-sources.json` dependencies match with the version of `desktop-plus` we are building.

To update `desktop-plus` repo to its latest commit and update the dependencies, you have to:

1. (If needed) Clone [https://github.com/desktop-plus/desktop-plus](https://github.com/desktop-plus/desktop-plus) inside this repo.

    ```sh
    git clone https://github.com/desktop-plus/desktop-plus.git
    ```

1. Checkout the correct commit:

    ```sh
    cd desktop-plus
    git checkout <commit hash or tag>
    ```

1. Change the commit in `org.desktop_plus.desktop-plus.yaml` to the desired one:

    ```yaml
    ...
          - type: git
            url: https://github.com/desktop-plus/desktop-plus.git
            tag: <tag name>
            commit: <commit hash>
    ...
    ```

1. If needed, create the venv

    ```sh
    python3 -m venv .venv
    .venv/bin/pip install -r requirements.txt
    ```

1. Run `./generate-sources` to update `generated-sources.json`.

1. Once you are sure it works, make a PR with the changes.
