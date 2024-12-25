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
    flatpak run --command=ya io.github.sxyazi.yazi $@
}
```

`flatpak` also installs a `.desktop` file by default. DEs that support desktop files can launch `yazi` from an application launcher.

## Running commands on the host

Since flatpak sandboxing prevents applications in the host from being directly opened, running commands not in the sandbox, such as `nvim`, would not be possible directly. The workaround is to use `host-spawn`. For example, `host-spawn nvim`. You can run the `yazi` flatpak with the following command to make the host's neovim the default editor.
```shell
flatpak run --env="EDITOR=host-spawn nvim" io.github.sxyazi.yazi
```

Some plugins that require additional libraries will not function due to this sandboxing limitation. Upstream needs to make some changes so that plugins use `host-spawn` automatically when inside the flatpak sandbox.

## Fix broken icons

Directories in the home folder should have accompanying icons out of the box. If they are garbled or not present, you may want to install [nerd-fonts](https://www.nerdfonts.com/). See [official documentation](https://yazi-rs.github.io/docs/faq#dont-like-nerd-fonts) for details.

## Updating this flatpak
```shell
flatpak run org.flathub.flatpak-external-data-checker io.github.sxyazi.yazi.yml --edit
python update_sources.py
```
You can then invoke `flatpak-builder` to make a test build.
