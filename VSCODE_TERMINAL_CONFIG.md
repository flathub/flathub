# VS Code Terminal Configuration

## Why use `host-spawn`?

This project is a **Flatpak**, which means VS Code runs inside an isolated container. Without proper configuration, VS Code's integrated terminal would be confined within the Flatpak sandbox.

## Solution: Using `host-spawn`

The configuration in `.vscode/settings.json` uses `host-spawn` to access the host system's terminal:

```json
{
    "terminal.integrated.defaultProfile.linux": "zsh",
    "terminal.integrated.profiles.linux": {
        "zsh": {
            "path": "host-spawn",
            "icon": "terminal-bash",
            "args": ["zsh"]
        },
        "bash": {
            "path": "host-spawn",
            "args": ["bash"]
        }
    },
    "terminal.external.linuxExec": "ptyxis",
    "workbench.activityBar.location": "top"
}
```

## What each option does:

- **`host-spawn`**: Executes commands on the host system, outside the Flatpak sandbox
- **`zsh`/`bash`**: Default shell to use
- **`ptyxis`**: Native system terminal when opening from the file explorer
- **Activity Bar at top**: Improves editor usability

## If you have terminal issues:

If the integrated terminal is not working correctly, make sure to:
1. Have this configuration in `.vscode/settings.json`
2. `host-spawn` is available on your system (usually included in the Flatpak)
3. Use zsh as an alternative shell (more compatible with Flatpak)

## References:

- [host-spawn GitHub](https://github.com/1player/host-spawn)
- [Flatpak Documentation](https://docs.flatpak.org/)
