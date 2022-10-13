# Discord

Discord is a free all-in-one messaging, voice and video client that's available on your computer and phone.

This repo hosts the flatpak wrapper for [Discord PTB](https://ptb.discord.com/), available at [Flathub](https://flathub.org/apps/details/com.discordapp.DiscordPTB).

```sh
flatpak install flathub com.discordapp.DiscordPTB
flatpak run com.discordapp.DiscordPTB
```

This flatpak is a fork of [com.discordapp.Discord](https://github.com/flathub/com.discordapp.Discord). It has been modified to wrap Discord-PTB.


## Differences in flatpak version

The flatpak version runs in a sandbox to provide better safety and privacy for users.

However, this sandboxing prevents the following features from working:

- **Game Activity**: This flatpak version of Discord cannot scan running processes to detect running games.  
  There is currently no workaround or solution for this limitation.
- **Unrestricted File Access**: Default sandbox permissions for this package limit Discord to only certain directories, so you can't access your entire Home directory. Currently, this limits which file directories you can attach files from and impacts drag and drop functionality.  
  This limitation will likely be overcomed eventually, when Electron give us a file picker portal which will allow full access to the filesystem while still restricting unauthorized access.  
  To work around this now, you can change sandbox permissions of installed flatpak applications (for example, with [Flatseal](https://flathub.org/apps/details/com.github.tchx84.Flatseal) or with `flatpak override --filesystem=home com.discordapp.DiscordPTB`) to give Discord broader file system access, allowing file attachments from more locations.
- **Rich Presence**: RPC has been depreciated by discord in favor of the Game-SDK!


## Legal

The Discord-PTB app itself is **proprietary** (closed source).

This wrapper is not verified by, affiliated with, or supported by Discord Inc.
