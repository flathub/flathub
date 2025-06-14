# RBDoom3BFG on Flatpak

This project contains files to build [RBDoom3BFG](https://github.com/RobertBeckebans/RBDOOM-3-BFG) as a Flatpak app.

## Copy game files
Copy base/ folder either from Steam or GOG to `~/.var/app/io.github.RobertBeckebans.RBDoom3BFG/data/rbdoom3bfg/base/`.

## How to build the app

### 1 - Prepare the environment
Ensure you have the following commands installed on your system:
- `git`
- `patch`
- `flatpak`
- `flatpak-builder`

Ensure you have the `flathub` repo enabled:
```shell
$ flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

Clone this project on your computer:
```shell
$ git clone https://github.com/RobertBeckebans/RBDOOM-3-BFG_Flatpak.git
```

### 2 - Build and install the app
From the project directory run the command:
```shell
$ FLATPAK_BUILDER_N_JOBS=$(nproc) flatpak-builder --user --verbose --install --install-deps-from=flathub --force-clean build io.github.RobertBeckebans.RBDoom3BFG.yaml
```

See [flatpak documentation](https://docs.flatpak.org/) for more info.

The first build can take a while (around 15 minutes or more), it depends on your hardware. It compiles and installs the app, making it available for your user in the system.

*NOTE:* if you want to install the app system wide, remove the `--user` option and the use `sudo` command.

### 3 - Run the app
You can run the RBDoom3BFG launching it from your favorite desktop, or manually by using the `flatpak` command:
```shell
$ flatpak run io.github.RobertBeckebans.RBDoom3BFG
```

### 4 - Debug the app if necessary
You can run the build using gdb:
```shell
$ flatpak run --command=gdb --devel io.github.RobertBeckebans.RBDoom3BFG -- /app/bin/RBDoom3BFG
```
