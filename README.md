Flathub
-------

Flathub is the central place for building and hosting flatpak builds.
Go to http://flathub.org/builds/#/ to see Flathub in action.

Building applications
---------------------

Application manifests should go in their own branches, named after the application ID.

For example, for gnome-recipes, there is a branch named org.gnome.Recipes which has
the org.gnome.Recipes.json file at the toplevel.

The manifests should build releases, not development snapshots, so please use
tarballs or git tags, not just the tip of a branch.

In case you build from a git tag, please specify both the tag name and the commit id, like so:
```
   "branch": "1.0.4",
   "commit": "cdfb19b90587bc0c44404fae30c139f9ec1cca5c"
```
This makes the build repeatable, since a tag can otherwise change its value over time.
You can also use only a commit, but specifying a tag name if there is one is good for readability.

Flathub always builds in the flatpak branchname "stable", and it always passes --default-name=stable,
so you don't need to specify a branch key. But if you do (not recommended), it must be "stable".

Flathub requires that you build against an sdk that is itself hosted on Flathub. The same goes for
sdk extensions that may be required to build your application. The easiest way to see what runtimes
are currently available is to install the flathub remote and use
```
flatpak remote-ls --runtime flathub
```
All applications in flathub should ship with appstream data.

Using the flathub repository
----------------------------

To install applications that are hosted on Flathub, use the following:
```
flatpak remote-add flathub http://flathub.org/repo/flathub.flatpakrepo
flatpak --user install flathub org.gnome.Recipes
```

