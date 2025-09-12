#!/usr/bin/env bash

cd "out/linux.amd64/release/bin"

# libraries (and non-PATH executables)
install -d -m0755 "/app/lib/virtualbox"
install -m0755 *.so -t "/app/lib/virtualbox"
install -m0644 *.r0 VBoxEFI*.fd -t "/app/lib/virtualbox"
## setuid root binaries
install -m0755 VirtualBoxVM VBoxSDL VBoxHeadless VBoxNetAdpCtl VBoxNetDHCP VBoxNetNAT -t "/app/lib/virtualbox"
## other binaries
install -m0755 VirtualBox VBoxManage VBoxSVC VBoxExtPackHelperApp VBoxBalloonCtrl vbox-img vboximg-mount vboxwebsrv webtest -t "/app/lib/virtualbox"

# binaries (in /app/bin)
install -d -m0755 "/app/bin"
sed -i "s@\/usr@/app@g" VBox.sh
sed -i "/PATH\=/d" VBox.sh
install -m0755 VBox.sh "/app/bin/VBox"
for i in VirtualBox VirtualBoxVM VBoxManage VBoxSDL VBoxHeadless VBoxBugReport VBoxBalloonCtrl VBoxAutostart vboxwebsrv; do
    ln -sf VBox "/app/bin/${i}"
    ln -sf VBox "/app/bin/${i,,}"
done
for i in vbox-img vboximg-mount; do
    ln -s ../lib/virtualbox/"${i}" "/app/bin/${i}"
done

# components
install -d -m0755 "/app/lib/virtualbox/components"
install -m0755 components/* -t "/app/lib/virtualbox/components"

# extensions packs
## as virtualbox install itself stuff in this directory, move it to /var and
## trick it with a symlink
## FIXME: trick is disabled for now
#install -d -m0755 "/var/lib/virtualbox/extensions"
#install -d -m0755 "/app/share/virtualbox/extensions"
#ln -s ../../../var/lib/virtualbox/extensions "/app/lib/virtualbox/ExtensionPacks"
install -d -m0755 "/app/lib/virtualbox/ExtensionPacks"

# languages
install -d -m0755 "/app/share/virtualbox/nls"
install -m0755 nls/*.qm -t "/app/share/virtualbox/nls"

# useless scripts
install -m0755 VBoxCreateUSBNode.sh VBoxSysInfo.sh -t "/app/share/virtualbox"

# icons
install -D -m0644 VBox.png "/app/share/pixmaps/VBox.png"

pushd icons
for i in *; do
    install -d "/app/share/icons/hicolor/${i}/mimetypes"
    cp "${i}/"* "/app/share/icons/hicolor/${i}/mimetypes"
done
popd

# desktop
install -D -m0644 virtualbox.desktop "/app/share/applications/virtualbox.desktop"
install -D -m0644 virtualbox.xml "/app/share/mime/packages/virtualbox.xml"

# install configuration
install -d -m0755 "/etc/vbox"
echo 'INSTALL_DIR=/app/lib/virtualbox' > "/etc/vbox/vbox.cfg"

# files for unattended installs
# Is there any better way to do this?
mv 'UnattendedTemplates' "/app/share/virtualbox/"

# back to srcdir
cd "${srcdir}"

#licence
install -D -m0644 VirtualBox-${pkgver}/COPYING "/app/share/licenses/${pkgname}/LICENSE"
install -D -m0644 VirtualBox-${pkgver}/COPYING.CDDL "/app/share/licenses/${pkgname}/LICENSE.CDDL"

# install systemd stuff
install -D -m0644 60-vboxdrv.rules "/app/lib/udev/rules.d/60-vboxdrv.rules"
install -D -m0644 vboxweb.service "/app/lib/systemd/system/vboxweb.service"
install -D -m0644 virtualbox.sysusers "/app/lib/sysusers.d/virtualbox.conf"

# install module reloading shortcut (with a symlink with default helper)
install -D -m0755 vboxreload "/app/bin"
ln -s vboxreload "/app/bin/rcvboxdrv"
