# TeamSpeak 3

If you are looking for TeamSpeak 5, please check out [com.teamspeak.TeamSpeak](https://github.com/flathub/com.teamspeak.TeamSpeak).


## Installation
Add flathub:
```
flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
```

Install:
```
flatpak install flathub com.teamspeak.TeamSpeak3
```

## Migration
If you migrate from the original TeamSpeak (5) repository, just run the following to migrate your data:
```
cp -r .var/app/com.teamspeak.TeamSpeak/ .var/app/com.teamspeak.TeamSpeak3
```

