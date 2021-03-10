# Flatpak package for Xilinx Vivado Design Suite

Vivado Design Suite is a software suite designed by Xilinx for the design,
synthesis and analysis of HDL for its line of FPGAs and SoCs.

Vivado Design Suite includes many tools, like Vivado, Vitis, Vitis HLS and many
others.

This package provides a flatpak wrapper over an official installation. As the
Xilinx tools are not redistributable, this package does not contain any
Xilinx-licensed material, which is instead downloaded on-the-fly through the
official Xilinx installer.

## Install

Install the flatpak package, then run any Xilinx launcher and follow the
instructions.

## Uninstallation

Just remove the flatpak package to remove this wrapper.
Note that this *does not* remove the Xilinx installation; to remove it, run
the launcher *Uninstall Xilinx Design Tools or Devices* (or run
`flatpak uninstall com.github.corna.Vivado --delete-data` to wipe everything in
`~/.var/app/com.github.corna.Vivado/`).

## FAQ

### Why can't I see some of my files in Vivado?

All the Xilinx tools handled by this wrapper run in a sandbox with limited
filesystem access, only your documents and desktop folders are visibile.
Either work in those directories or override the default permission (with
`flatpak override com.github.corna.Vivado --filesystem=<path_here>`).

### I get an error "Can't open project [...]. Please verify that the project still exist.", why?

Same as before, make sure that the project is in one of the allowed folders.

### I have created a project outside Desktop or Documents: where are they?
You can find the sandboxed filesystem in `~/var/app/com.github.corna.Vivado/`.

### I can not access the hardware devices

Have you installed the "cable drivers" (which are just udev rules) at the end
of the installation? To install them, run:
`sudo ~/.var/app/com.github.corna.Vivado/data/xilinx-install/Vivado/<version>/data/xicom/cable_drivers/lin64/install_script/install_drivers/install_drivers && sudo udevadm control --reload`,
then re-connect any hardware device.

### Can you include the Xilinx installer in this package?

No, the installer is not redistributable.

### Can I deny internet access to the Xilinx tools?

Yes, just remove the corresponding permission with:
`flatpak override com.github.corna.Vivado  --unshare=network`.
Note that the license manager uses your MAC addresses, so you may experience
issues with non free products.

### How can I install additional components?

Run the launcher *Add Xilinx Design Tools or Devices*.

### Can I install another version of the Xilinx tools?

Yes, run `flatpak run --command=xilinx_install com.github.corna.Vivado`

### How can I remove a particular version?

Run the launcher *Uninstall Xilinx Design Tools or Devices*.

