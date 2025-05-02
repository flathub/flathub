# Flatpak Zed (Preview)

## Issues
Please open issues under: https://github.com/flathub/dev.zed.Zed-Preview/issues

## Usage

Zed's current Flatpak integration exits the sandbox on startup and most functionalities work out of the box. Workflows that rely on Flatpak's sandboxing may not work as expected by default.

Please note that Zed's flatpak still runs in an isolated environment and some language toolchains might misbehave when executed from the host OS into the sandbox.  
To cope with it, Zed's flatpak defaults can be changed to: 
  - disable sandbox escape at startup
  - enable SDK extensions to get support for additional languages

### Environment variables

- `ZED_FLATPAK_NO_ESCAPE`: disable flatpak sandbox escape (default: not set)
  ```shell
    $ flatpak override --user dev.zed.Zed-Preview --env=ZED_FLATPAK_NO_ESCAPE=1
  ```

### Execute commands on the host system

When Zed's flatpak is running in the sandbox with no escape, it is not possible to execute commands on the host system.

To execute commands on the host system, run inside the sandbox:

```shell
$ flatpak-spawn --host <COMMAND>
```

or

```shell
$ host-spawn <COMMAND>
```

- Most users seem to report a better experience with `host-spawn`

### Use host shell in the integrated terminal.

Another option to execute commands is to use your host shell in the integrated terminal instead of the sandbox one.

For that, open Zed's settings via <kbd>Ctrl</kbd> + <kbd>,</kbd>

The following examples will figure out and launch the current user's preferred terminal. More configuration settings for spawning commands can be found in [Zed's documentation](https://zed.dev/docs/configuring-zed#terminal-shell).

`flatpak-spawn --host`

```json
{
  "terminal": {
    "shell": {
      "with_arguments": {
        "program": "/usr/bin/flatpak-spawn",
        "args": [
          "--host",
          "--env=TERM=xterm-256color",
          "sh",
          "-c",
          "exec $(getent passwd $USER | cut -d: -f7)"
        ]
      }
    }
  },
}
```

`host-spawn`

```json
{
  "terminal": {
    "shell": {
      "with_arguments": {
        "program": "/app/bin/host-spawn",
        "args": [
          "sh",
          "-c",
          "exec $(getent passwd $USER | cut -d: -f7)"
        ]
      }
    }
  },
}
```

### SDK extensions

This flatpak provides a standard development environment (gcc, python, etc).
To see what's available:

```shell
  $ flatpak run --command=sh dev.zed.Zed-Preview
  $ ls /usr/bin (shared runtime)
  $ ls /app/bin (bundled with this flatpak)
```
To get support for additional languages, you have to install SDK extensions, e.g.

```shell
  $ flatpak install flathub org.freedesktop.Sdk.Extension.dotnet
  $ flatpak install flathub org.freedesktop.Sdk.Extension.golang
```
To enable selected extensions, set `FLATPAK_ENABLE_SDK_EXT` environment variable
to a comma-separated list of extension names (name is ID portion after the last dot):

```shell
  $ FLATPAK_ENABLE_SDK_EXT=dotnet,golang flatpak run dev.zed.Zed-Preview
```
To make this persistent, set the variable via flatpak override:

```shell
  $ flatpak override --user dev.zed.Zed-Preview --env=FLATPAK_ENABLE_SDK_EXT="dotnet,golang"
```

You can use:
```shell
  $ flatpak search <TEXT>
```
to find others.

### Run flatpak Zed from host terminal

If you want to run `zed /path/to/file` from the host terminal just add this
to your shell's rc file:

```shell
  $ alias zed="flatpak run dev.zed.Zed-Preview"
```

then reload sources, now you could try:

```shell
  $ zed /path/to/
  # or
  $ FLATPAK_ENABLE_SDK_EXT=dotnet,golang zed /path/to/
```

## Related Documentation

- https://zed.dev/docs/
