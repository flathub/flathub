#!/bin/bash
installdir=`flatpak-spawn --host flatpak info --show-location io.github.Archeb.opentrace`
if ! flatpak-spawn --host getcap "$installdir/files/extra/_nexttrace" | grep cap_net_admin > /dev/null
then
#     echo "Setting up caps.."
    flatpak-spawn --host pkexec bash -c "setcap cap_net_raw,cap_net_admin+eip $installdir/files/extra/_nexttrace"
fi
flatpak-spawn --host "$installdir/files/extra/_nexttrace" "$@"
