# ATLauncher

Flatpak for ATLauncher

## Custom directory

By providing an enviroment variable called `CUSTOM_DIR` with a path value,
you can have ATLauncher run in a custom directory.

You can acomplish this via Flatseal.

Steps:

1. Open Flatseal
2. Select ATLauncher on the left
3. Scroll down to "Filesystem", "Other files"
4. Add a new entry via the icon with a "folder" and "plus" sign
5. Input the path you want to let ATLauncher run in. IE `/home/name/Games/ATLauncher`
6. Scroll down to "Environment", "Variables"
7. Click the "plus" sign to add a new entry
8. Write down an enviroment variable, IE `CUSTOM_DIR=/home/name/Games/ATLauncher`
9. Run ATLauncher
10. Notice that ATLauncher is now working using CUSTOM_DIR
