# Gibo
[gibo][gibo] (short for .gitignore boilerplates) is a shell script to help you easily
access .gitignore boilerplates from [Github.com][gitignore].

## Build and install
Assuming *flatpak* and *flatpak-builder* are installed, follow the next steps:

```bash
$ git clone https://github.com/flathub/com.github.simonwhitaker.Gibo.git
$ cd com.github.simonwhitaker.Gibo
$ flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo
$ flatpak-builder .build --force-clean --install-deps-from=flathub --install --user com.github.simonwhitaker.Gibo.yaml
```

To uninstall:
```bash
$ flatpak uninstall --delete-data --user com.github.simonwhitaker.Gibo
```

To clean up everything:
```bash
$ rm -rf .flatpak-builder/ .build/
$ flatpak uninstall --unused --user
$ flatpak remote-delete --user flathub
```

## Roadmap
See the [issue tracker](https://github.com/flathub/com.github.simonwhitaker.Gibo/issues/).

[gibo]: https://github.com/simonwhitaker/gibo
[gitignore]: https://github.com/github/gitignore
