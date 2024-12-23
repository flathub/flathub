## Fix broken icons

Directories in the home folder should have accompanying icons out of the box. If they are garbled or not present, you may want to install [nerd-fonts](https://www.nerdfonts.com/). See [official documentation](https://yazi-rs.github.io/docs/faq#dont-like-nerd-fonts) for details. 

## Launching

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

`flatpak` also installs a desktop file by default. DEs that support desktop files can launch `yazi` from an application launcher.

## What does not work

[Shell wrappers](https://yazi-rs.github.io/docs/quick-start#shell-wrapper)
