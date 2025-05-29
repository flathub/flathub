# Cursor Flatpak<!-- omit in toc -->

🚨 Warning: This is an unofficial Flatpak build of Cursor, generated from the official Anysphere-built .deb packages [here](https://github.com/flathub/co.anysphere.cursor/blob/master/co.anysphere.cursor.yaml#L103).

## Table of Contents<!-- omit in toc -->

- [Usage](#usage)
  - [Execute commands in the host system.](#execute-commands-in-the-host-system)
  - [Use host shell in the integrated terminal.](#use-host-shell-in-the-integrated-terminal)
  - [Support for language extension.](#support-for-language-extension)
- [Support](#support)

## Usage

Most functionality works out of the box, though please note that flatpak runs in an isolated environment and some work is necessary to enable those features.

### Execute commands in the host system.

To execute commands on the host system, run inside the sandbox:

`$ flatpak-spawn --host <COMMAND>`

or

`$ host-spawn <COMMAND>`

- Most users seem to report a better experience with `host-spawn`

### Use host shell in the integrated terminal.

Another option to execute commands is to use your host shell in the integrated terminal instead of the sandbox one.

For that go to `File -> Preferences -> Settings` and find `Features > Terminal > Integrated > Profiles`, then click on `Edit in settings.json` (The important thing here is to open settings.json)

And make sure that you have the following lines there:

`flatpak-spawn --host`

```
{
  "terminal.integrated.defaultProfile.linux": "bash",
  "terminal.integrated.profiles.linux": {
    "bash": {
      "path": "/usr/bin/flatpak-spawn",
      "args": ["--host", "--env=TERM=xterm-256color", "bash"],
      "icon": "terminal-bash",
      "overrideName": true
    }
  }
}
```

`host-spawn`

```
{
  "terminal.integrated.defaultProfile.linux": "bash",
  "terminal.integrated.profiles.linux": {
    "bash": {
      "path": "/app/bin/host-spawn",
      "args": ["bash"],
      "icon": "terminal-bash",
      "overrideName": true
    }
  }
}
```

- You can change **bash** to any terminal you are using: zsh, fish, sh.
- `overrideName` allows for the 'name' (or whatever you set it to) of the shell you're using to appear (e.g. normally zsh, fish, sh).

### Support for language extension.

Some Visual Studio extension depends on packages that might exist on your host, but they are not accessible thought Flatpak. Like support to programming languages: gcc, python, etc..

**See available SDK:**

```
$ flatpak run --command=sh co.anysphere.cursor
$ ls /usr/bin (shared runtime)
$ ls /app/bin (bundled with this flatpak)
```

**Getting support for additional languages, you have to install SDK extensions, e.g.**

```
$ flatpak install flathub org.freedesktop.Sdk.Extension.dotnet
$ flatpak install flathub org.freedesktop.Sdk.Extension.golang
$ FLATPAK_ENABLE_SDK_EXT=dotnet,golang flatpak run co.anysphere.cursor
```

**Finding other SDK**

`flatpak search <TEXT>`

## Support

Please open issues under: https://github.com/flathub/co.anysphere.cursor/issues
