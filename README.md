# com.dygma.bazecor (Flathub Bundle)

Flathub packaging repository for **Bazecor** by Dygma.

## Build

Build this Flatpak bundle locally:

```bash
./build_and_install.sh
```

## Install From Flathub

Install Bazecor from Flathub:

```bash
flatpak install flathub com.dygma.bazecor
```

Run Bazecor:

```bash
flatpak run com.dygma.bazecor
```

Update Bazecor:

```bash
flatpak update com.dygma.bazecor
```

Remove Bazecor:

```bash
flatpak uninstall com.dygma.bazecor
```

## Installing udev rules

Bazecor needs a host udev rule so it can communicate correctly with Dygma keyboards.

Install the rule with the helper script:

```bash
curl -fsSL https://raw.githubusercontent.com/flathub/com.dygma.bazecor/main/install-dygma-udev-rules.sh | sh
```

The script installs `/etc/udev/rules.d/60-dygma.rules`, reloads rules, and triggers udev.

After running it, you may need to unplug and reconnect your keyboard.

### Manual installation (alternative)

If you prefer not to pipe an online from curl, download the rules file and install manually like so:

```bash
# First download the rules file, and inspect it
curl -fsSL -o ~/Downloads/60-dygma.rules https://raw.githubusercontent.com/flathub/com.dygma.bazecor/main/etc/udev/rules.d/60-dygma.rules

# Then install it (requires root privilege)
sudo cp ~/Downloads/60-dygma.rules /etc/udev/rules.d/

# And finally, reload the udev rules with udevadm trigger. You may require to unplug and re-connect your keyboard
sudo udevadm control --reload-rules
sudo udevadm trigger
```
