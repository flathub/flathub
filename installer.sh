#!bin/sh

VERSION=2020.1
PREFIX=/app/opt/Xilinx

set -e

# Extract and run the installer
echo "Extracting installer archive..."
mkdir "$FLATPAK_BUILDER_BUILDDIR/installer"
tar -xf "$FLATPAK_BUILDER_BUILDDIR/xilinx_unified_installer.tar.gz" -C "$FLATPAK_BUILDER_BUILDDIR/installer"
sed -i "s/^Destination=.*/Destination=$PREFIX/" "$FLATPAK_BUILDER_BUILDDIR/xilinx_unified_installer.conf"
"$FLATPAK_BUILDER_BUILDDIR/installer/"*"/xsetup" --batch Install --agree XilinxEULA,3rdPartyEULA,WebTalkTerms --config "$FLATPAK_BUILDER_BUILDDIR/xilinx_unified_installer.conf"

# Link the binaries
mkdir -p /app/bin
ln -s "$PREFIX/Vivado/$VERSION/bin/vivado" /app/bin/com.xilinx.Vivado
ln -s "$PREFIX/Vivado/$VERSION/bin/vivado_hls" /app/bin/com.xilinx.Vivado.HLS
ln -s "$PREFIX/DocNav/docnav" /app/bin/com.xilinx.Vivado.Docnav

# Install the icons
convert '$PREFIX/Vivado/$VERSION/doc/images/vivado_logo.png[0]' -thumbnail 256x256 -alpha on -background none -flatten "$FLATPAK_BUILDER_BUILDDIR/vivado.png"
install -Dm644 "$FLATPAK_BUILDER_BUILDDIR/vivado.png" /app/share/icons/hicolor/256x256/apps/com.xilinx.Vivado.png
install -Dm644 "$PREFIX/Vivado/$VERSION/common/icons/CS1056_Vivado_HSL_Icon_64x64.png" /app/share/icons/hicolor/64x64/apps/com.xilinx.Vivado.HLS.png
install -Dm644 "$PREFIX/DocNav/resources/doc_nav_application_48.png" /app/share/icons/hicolor/48x48/apps/com.xilinx.Vivado.Docnav.png

# Install the desktop files
install -Dm644 "$HOME/Desktop/Vivado $VERSION.desktop" /app/share/applications/com.xilinx.Vivado.desktop
install -Dm644 "$HOME/Desktop/Vivado HLS $VERSION.desktop" /app/share/applications/com.xilinx.Vivado.HLS.desktop
install -Dm644 "$HOME/Desktop/Documentation Navigator.desktop" /app/share/applications/com.xilinx.Vivado.Docnav.desktop

# Adjust "Exec" and "Icon" of the installed desktop files
desktop-file-edit --set-key=Exec --set-value=com.xilinx.Vivado --set-icon=com.xilinx.Vivado /app/share/applications/com.xilinx.Vivado.desktop
desktop-file-edit --set-key=Exec --set-value=com.xilinx.Vivado.HLS --set-icon=com.xilinx.Vivado.HLS /app/share/applications/com.xilinx.Vivado.HLS.desktop
desktop-file-edit --set-key=Exec --set-value=com.xilinx.Vivado.Docnav --set-icon=com.xilinx.Vivado.Docnav /app/share/applications/com.xilinx.Vivado.Docnav.desktop

# Vivado requires libtinfo5
mkdir -p /app/lib
ln -s /usr/lib/x86_64-linux-gnu/libtinfo.so.6.1 /app/lib/libtinfo.so.5

