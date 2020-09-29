#!/usr/bin/env sh

VERSION=2020.1
PREFIX=/app/opt/Xilinx

set -e

# Make sure that the Destination and CreateDesktopShortcuts have the correct value
sed -i "s|^Destination=.*|Destination=$PREFIX|" "$FLATPAK_BUILDER_BUILDDIR/xilinx_unified_installer.conf"
sed -i "s|^CreateDesktopShortcuts=.*|CreateDesktopShortcuts=1|" "$FLATPAK_BUILDER_BUILDDIR/xilinx_unified_installer.conf"

# Run the installer
"$FLATPAK_BUILDER_BUILDDIR/installer/xsetup" --batch Install --agree XilinxEULA,3rdPartyEULA,WebTalkTerms --config "$FLATPAK_BUILDER_BUILDDIR/xilinx_unified_installer.conf"

# Link the binaries (can't use symlinks for Vivado and Vitis, as they require $0 to be set to the real binary path)
mkdir -p /app/bin
echo -e '#!'"/bin/sh\n\nexport LD_LIBRARY_PATH=/app/lib\nexec $PREFIX/Vivado/$VERSION/bin/vivado $*" > /app/bin/com.xilinx.Vivado
echo -e '#!'"/bin/sh\n\nexport LD_LIBRARY_PATH=/app/lib\nexec $PREFIX/Vitis/$VERSION/bin/vitis $*" > /app/bin/com.xilinx.Vivado.Vitis
echo -e '#!'"/bin/sh\n\nexport LD_LIBRARY_PATH=/app/lib\nexec $PREFIX/Vitis/$VERSION/bin/vitis_hls $*" > /app/bin/com.xilinx.Vivado.HLS
chmod 755 /app/bin/com.xilinx.Vivado
chmod 755 /app/bin/com.xilinx.Vivado.Vitis
chmod 755 /app/bin/com.xilinx.Vivado.HLS
ln -s "$PREFIX/DocNav/docnav" /app/bin/com.xilinx.Vivado.DocNav

# Install the icons
install -Dm644 "$PREFIX/Vivado/$VERSION/doc/images/vivado_logo.png" /app/share/icons/hicolor/64x64/apps/com.xilinx.Vivado.png
install -Dm644 "$PREFIX/DocNav/resources/doc_nav_application_48.png" /app/share/icons/hicolor/48x48/apps/com.xilinx.Vivado.DocNav.png
icotool -x -w 256 -o "$FLATPAK_BUILDER_BUILDDIR/ide_icon.png" "$PREFIX/Vitis/$VERSION/doc/images/ide_icon.ico"
install -Dm644 "$FLATPAK_BUILDER_BUILDDIR/ide_icon.png" /app/share/icons/hicolor/256x256/apps/com.xilinx.Vivado.Vitis.png
install -Dm644 "$FLATPAK_BUILDER_BUILDDIR/ide_icon.png" /app/share/icons/hicolor/256x256/apps/com.xilinx.Vivado.HLS.png

# Install the desktop files
install -Dm644 "$HOME/Desktop/Vivado $VERSION.desktop" /app/share/applications/com.xilinx.Vivado.desktop
install -Dm644 "$HOME/Desktop/Documentation Navigator.desktop" /app/share/applications/com.xilinx.Vivado.DocNav.desktop
install -Dm644 "$HOME/Desktop/Xilinx Vitis $VERSION.desktop" /app/share/applications/com.xilinx.Vivado.Vitis.desktop
install -Dm644 "$HOME/Desktop/Vitis HLS $VERSION.desktop" /app/share/applications/com.xilinx.Vivado.HLS.desktop

# Adjust "Exec" and "Icon" of the installed desktop files
desktop-file-edit --set-key=Exec --set-value=com.xilinx.Vivado --set-icon=com.xilinx.Vivado /app/share/applications/com.xilinx.Vivado.desktop
desktop-file-edit --set-key=Exec --set-value=com.xilinx.Vivado.DocNav --set-icon=com.xilinx.Vivado.DocNav /app/share/applications/com.xilinx.Vivado.DocNav.desktop
desktop-file-edit --set-key=Exec --set-value=com.xilinx.Vivado.Vitis --set-icon=com.xilinx.Vivado.Vitis /app/share/applications/com.xilinx.Vivado.Vitis.desktop
desktop-file-edit --set-key=Exec --set-value=com.xilinx.Vivado.HLS --set-icon=com.xilinx.Vivado.HLS /app/share/applications/com.xilinx.Vivado.HLS.desktop
