# [Ludusavi](https://github.com/mtkennerly/ludusavi)

## Adding drives

If you use drives mounted outside /run/media/ for games you need to add the drive to other files of this App in [Flatseal](https://flathub.org/apps/details/com.github.tchx84.Flatseal) or run this command:

```bash
flatpak override com.github.mtkennerly.ludusavi --filesystem=<PATH_TO_MOUNT>
```
