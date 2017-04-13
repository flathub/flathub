Flathub
-------

Flathub is the central place for building and hosting Flatpak builds.
Go to http://flathub.org/builds/#/ to see Flathub in action.

Building applications
---------------------

Application manifests should go in their own repository in the [Flathub](http://github.com/flathub) organization,
named after the application ID.

For example, for gnome-recipes, there is a repository named org.gnome.Recipes which has the org.gnome.Recipes.json
file at the toplevel.

Hosted builds should be stable releases, not development snapshots, so please use tarballs or git tags, not just
the tip of a branch.

More detailed requirements can be found in the [Review Guidelines](https://github.com/flatpak/flathub/wiki/Review-Guidelines)

Using the Flathub repository
----------------------------

To install applications that are hosted on Flathub, use the following:
```
flatpak remote-add flathub http://flathub.org/repo/flathub.flatpakrepo
flatpak --user install flathub org.gnome.Recipes
```

