# pgAdmin Flatpak

## Building

```
flatpak run org.flatpak.Builder build-dir --user --ccache --force-clean --install org.pgadmin.pgadmin4.yml
```

Then you can run it via the command line:

```
flatpak run org.pgadmin.pgadmin4
```

or just search for the installed app on your system

You can regenerate pip dependencies with the script `regen-pip.sh`, but you will need to have `krb5-config` installed on your system due to a wheel needing it.
