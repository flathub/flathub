This is where the magic happens.

Application manifests should go in their own branches, named after the application ID.

For example, for gnome-recipes, there is a branch named org.gnome.Recipes which has
the org.gnome.Recipes.json file at the toplevel.

The manifests should build releases, not development snapshots, so please use
tarballs or git tags, not just the tip of a branch.
