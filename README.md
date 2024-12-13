# [Madamiru](https://github.com/mtkennerly/madamiru)

## Adding drives

If you use drives mounted outside `/run/media/` for media files,
you need to add the drive to the other files of this app in
[Flatseal](https://flathub.org/apps/details/com.github.tchx84.Flatseal),
or run this command:

```bash
flatpak override com.mtkennerly.madamiru --filesystem=<PATH_TO_MOUNT>
```
