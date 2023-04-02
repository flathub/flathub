# Roccat-tools

## Add udev rules for your devices.

1. List supported devices rules :

```sh
flatpak run --command="ls" net.sourceforge.roccat.roccat-tools /app/lib/udev
```

2. Copy the rules correponding to your devices(s) to your host system. For instance :

```sh
flatpak run --command="cat" net.sourceforge.roccat.roccat-tools /app/lib/udev/90-roccat-[YOUR_DEVICE].rules | sudo tee /etc/udev/rules.d/90-roccat-[YOUR_DEVICE].rules
```

3. Reload udev rules

```sh
sudo udevadm control --reload-rules && udevadm trigger
```

## Start your device application

You can now start a roccat configuration application from your desktop environment.

Be sure to start the configuration app for your configured device.