## Shell extension

In order to get the shell extension working outside the app, you need to add the org.freedesktop.Flatpak permission. 
```
flatpak override --user me.proton.Lumo --system-talk-name=org.freedesktop.Flatpak  
```
You also need to tell Lumo to execute commands like this. This command can be added in the memory for Lumo to remember in all future conversations. 
```
flatpak-spawn --host <COMMAND>
```
 ⚠️ This will enable Lumo to do everything on your system. Be careful !

# For maintainers

## Source generation
To transform their package locks into flatpak sources, [flatpak-builder-tools](https://github.com/flatpak/flatpak-builder-tools) is used.

```sh
# Generate lumo-desktop sources for use with cargo
<path-to flatpak-builder-tools>/cargo/flatpak-cargo-generator.py -o cargo-sources.json <path-to lumo-desktop>/src-tauri/Cargo.lock

# Generate lumo-desktop sources for use with npm
flatpak-node-generator --no-requests-cache -r -o node-sources.json npm <path-to lumo-desktop>/package-lock.json
```
