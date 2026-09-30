# Flathub

Flathub is the central place for building and hosting Flatpak builds.

Using the Flathub repository
----------------------------

To install applications that are hosted on Flathub, use the following:
```
flatpak remote-add flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.gnome.Recipes
```

To install applications from the beta branch, use the following:
```
flatpak remote-add flathub-beta https://flathub.org/beta-repo/flathub-beta.flatpakrepo
flatpak install flathub-beta org.godotengine.Godot
```

For more information and more applications see https://flathub.org

Contributing to Flathub
-----------------------

For information on creating packages or reporting issues please see the [contributing page](/CONTRIBUTING.md).

***Note:*** *this repository is not for reporting issues related to the flathub.org website itself or contributing to its development. For that, go to https://github.com/flathub-infra/website*

---

## AI Agent & Developer Submission Guide

For autonomous AI coding agents and package maintainers submitting Flatpak applications to Flathub, please consult [AGENTS.md](AGENTS.md) for strict submission requirements, manifest linter rules, AppStream checklist formatting, and automated bot instructions.
