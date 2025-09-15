## Flathub shared modules

This repository contains Flatpak build recipes of commonly shared
modules and is intended to be used as a git submodule. Each submodule
may include additional instructions to be used properly. Please check
the folder containing that module to see if anything extra needs to be
done.

### Adding

To use shared modules for packaging an application, add the submodule:

```sh
git submodule add https://github.com/flathub/shared-modules.git
```

### Usage

Then modules from this repository can be specified in an application
manifest.

```json
"modules": [
  "shared-modules/SDL/SDL-1.2.15.json",
]
```
And for a YAML manifest:

```yaml
modules:
  - shared-modules/SDL/SDL-1.2.15.json
```

### Updating

To update the submodule:

```sh
git submodule update --remote --merge
```

To automate updates, [dependabot](https://docs.github.com/en/code-security/getting-started/dependabot-quickstart-guide)
can be used but please limit the update frequency to not more than once
or twice a week.

### Removing

To remove the submodule:

```sh
git submodule deinit -f -- shared-modules
rm -rf .git/modules/shared-modules
git rm -f shared-modules
rm .gitmodules
```

### External data checker

We provide an automatic updating mechanism for submodules located here.
In order to utilize it [set up x-checker-data](https://github.com/flathub/flatpak-external-data-checker)
for your sources and they will be checked for updates in a weekly basis.

### Inclusion criteria

- The module must be widely used on [Flathub](https://github.com/flathub)
  by actively maintained applications.

- The module must not be provided by any current branch of the runtimes.

- The module manifest must be in JSON format.

### Inclusion process

Please open a [pull request](https://github.com/flathub/shared-modules/pulls)
with the module manifest and add yourself as a [codeowner](https://github.com/flathub/shared-modules/blob/master/CODEOWNERS)
of that module.

The manifest must be buildable with the latest Freedesktop SDK runtime.
