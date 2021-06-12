## How do I build this package?

The easiest way will be to have `flatpak-builder` installed and running:

```sh
rm -rf .flatpak-builder/ && flatpak-builder build-dir/ io.gdevs.GDLauncher.yml --force-clean --install --user
```

## How do I play versions of Minecraft that require Java non-LTS (1.17+)?

GDLauncher does not currently provide a mechanism to do this automatically, but 
it is quite trivial to do it in this flatpak build.

Simply right click on the instance you wish to use Java 16 for > Click "Manage" >
 Enable "Custom Java Path" > And map the path to `/app/jre/bin/java`
 
Then you should be good to go!
