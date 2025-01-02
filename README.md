This is a flatpak of [yazi](https://github.com/sxyazi/yazi), a terminal file manager.

## Launching `yazi` and `ya`

To launch `yazi`, run
```shell
flatpak run io.github.sxyazi.yazi
```

You can make a few functions to directly invoke `yazi` and `yz` in the command line. These functions are tested in `zsh` and may need to be adapted to be used in other shells.
```shell
function yazi() {
    flatpak run --command=yazi io.github.sxyazi.yazi $@
}
function ya() {
    # share=network enables networking since the package manager needs it to
    # download themes and plugins
    flatpak --share=network run --command=ya io.github.sxyazi.yazi $@
}
```

`flatpak` also installs a `.desktop` file by default. DEs that support desktop files can launch `yazi` from an application launcher.

## Tweaking config files
By default, all configurations are stored under `$HOME/.var/app/io.github.sxyazi.yazi`. Plugins and themes installed by `ya` are also stored there. If you already have `yazi` configs or a `zoxide` database, you may need to move them. For example, the `zoxide` database is in `$HOME/.local/share/zoxide` by default, but would be in `$HOME/.var/app/io.github.sxyazi.yazi/data/zoxide` for `flatpak`. 

You can also run `zoxide` through `flatpak`. For example, the following command creates a `zoxide` database by importing existing `zsh-z` data. 
```shell
flatpak run --command=zoxide io.github.sxyazi.yazi import --from=z $HOME/.z
```

### Enabling nerd-fonts
For the best out-of-the-box experience, nerd-fonts is disabled by default. To enable it, go to the `theme.toml` file located in `$HOME/.var/app/io.github.sxyazi.yazi/config/yazi` and remove the section that disables nerd-fonts.

## Running commands on the host

Since flatpak sandboxing prevents applications in the host from being directly opened, running commands not in the sandbox, such as `nvim`, would not be possible directly. The workaround requires two things:
1. Enabling the application to talk to `org.freedesktop.Flatpak`. Note that this permits `Yazi` and all its plugins to launch arbitrary commands on the host, which compromises `flatpak`'s security mechanism. This can be done by adding `--talk-name=org.freedesktop.Flatpak` to `flatpak run`.
2. Using `host-spawn` to launch programs.

For example, you can run `yazi` with the following command to make the host's neovim the default editor. 
```shell
flatpak run --talk-name=org.freedesktop.Flatpak --env="EDITOR=host-spawn nvim" io.github.sxyazi.yazi
```

Some plugins that require additional libraries will also not function due to this sandboxing limitation. Upstream needs to make some changes so that plugins use `host-spawn` automatically when inside the flatpak sandbox.

## Fix broken icons

Directories in the home folder should have accompanying icons out of the box. If they are garbled or not present, you may want to install [nerd-fonts](https://www.nerdfonts.com/). See [official documentation](https://yazi-rs.github.io/docs/faq#dont-like-nerd-fonts) for details.

## Other flatpak limitations
Directories such as `/sys`, `/tmp`, `/usr`, and `$HOME/.var/app` are sandboxed. Access to these directories can be granted with, for example, `--filesystem=/tmp`, though some cannot be shared with sandboxed applications (e.g. `/usr`). 

## Updating this flatpak
To create a new version of this application, run
```shell
flatpak run org.flathub.flatpak-external-data-checker io.github.sxyazi.yazi.yml --edit
python update_sources.py
```
You can then invoke `flatpak-builder` to make a test build.
