# com.valvesoftware.Steam.Utility.vkBasalt

This is the [vkBasalt](https://github.com/DadSchoorse/vkBasalt/) flatpak for Steam's flatpak.

## Enable vkBasalt per game

Edit the launch option of a game and add:

```ini
ENABLE_VKBASALT=1 %command%
```

## Permanentally enable vkBasalt globally

```bash
flatpak override --env=ENABLE_VKBASALT=1 com.valvesoftware.Steam # add `--user` if it is installed as a user
```

## Configuration

By default, games will utilize the vkBasalt configuration from https://github.com/DadSchoorse/vkBasalt/blob/master/config/vkBasalt.conf.

You can create a custom `vkBasalt.conf` file at `~/.var/app/com.valvesoftware.Steam/config/vkBasalt`, which games will utilize this configuration file instead:

```bash
mkdir ~/.var/app/com.valvesoftware.Steam/config/vkBasalt
curl https://raw.githubusercontent.com/DadSchoorse/vkBasalt/master/config/vkBasalt.conf -o ~/.var/app/com.valvesoftware.Steam/config/vkBasalt/vkBasalt.conf
$EDITOR ~/.var/app/com.valvesoftware.Steam/config/vkBasalt/vkBasalt.conf
```
