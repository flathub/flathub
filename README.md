This is where the magic happens.

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

All applications in flathub should ship with appstream data.
