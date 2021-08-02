# MangoHUD 

## Configuration

MangoHud can be configured just like the [instructions](https://github.com/flightlessmango/MangoHud#hud-configuration) mentioned by the project with a few changes:

1. You can either create a configuration file in: `~/.var/app/com.valvesoftware.Steam/config/MangoHud/MangoHud.conf`

1. Or you can give the flatpak access to filesystem on the host: `flatpak override --user --filesystem=xdg-config/MangoHud:ro com.valvesoftware.Steam` if you prefer to keep the config file there.

## Resources

- [MangoHUD Homepage](https://github.com/flightlessmango/MangoHud)