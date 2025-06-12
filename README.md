# PatchPal Flatpak

## Installation

```sh
$ flatpak install --user com.redhat.patchpal.gui*.flatpak
$ install -Dm755 patchpal.sh ~/.local/bin/patchpal
```

The patchpal script is used to process the sandboxed application's parameters
and give it access to your host git repository.

Note that you will need to make sure that your git configuration lives in
`~/.config/git`, not in `~/.gitconfig`, as only `~/.config` is accessible
by the sandboxed tool.

## Limitations

The editor button is currently non-functional, as the text editors would need to be included in the Flatpak for this functionality to work. See:
https://gitlab.cee.redhat.com/patchpal/gui/-/issues/1
