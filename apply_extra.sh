#!/bin/bash
set -e
shopt -s failglob
FLATPAK_ID="${FLATPAK_ID:-cn.wps.wps_365}"
CARCH="$(uname -m)"

mkdir -p export/share/

bsdtar --to-stdout -xf wps-office.deb data.tar.xz | bsdtar -xf -

# Move shared files to export directory
mv usr/share/{icons,applications,mime} export/share/

# Rename icons, desktop files, and mime files to use Flatpak ID
YEAR_SUFFIX=2023
rename --no-overwrite "wps-office-" "${FLATPAK_ID}." export/share/{icons/hicolor/*/*,applications}/wps-office-*.*
rename --no-overwrite "wps-office${YEAR_SUFFIX}-" "${FLATPAK_ID}." export/share/icons/hicolor/*/*/wps-office${YEAR_SUFFIX}-*.*
rename --no-overwrite "custom-wps-office" "${FLATPAK_ID}" export/share/mime/packages/custom-wps-office.xml
rename --no-overwrite "xiezuo" "${FLATPAK_ID}.xiezuo" export/share/{icons/hicolor/*/*,applications}/xiezuo.*

# Edit .desktop files to adjust Exec and Icon entries
for a in wps wpp et pdf prometheus xiezuo; do
    desktop_file="export/share/applications/${FLATPAK_ID}.$a.desktop"
    appbin="$a %F"
    appicon="${FLATPAK_ID}.${a}main"
    case "$a" in
        pdf)
            appbin="wpspdf %F"
        ;;
        prometheus)
            appbin="wps %F"
            appicon="${FLATPAK_ID}.k${a}"
            # Use this as the main .desktop file for the Flatpak
            new_desktop_file="$(dirname $desktop_file)/${FLATPAK_ID}.desktop"
            mv $desktop_file $new_desktop_file
            desktop_file=$new_desktop_file
        ;;
        xiezuo)
            appbin="xiezuo %U"
            appicon="${FLATPAK_ID}.xiezuo"
        ;;
    esac
    desktop-file-edit \
        --set-key="Exec" --set-value="$appbin" \
        --set-key="Icon" --set-value="$appicon" \
        --add-category="Office" \
        "$desktop_file"
done

# Edit mime XML files to adjust generic-icon names
sed -i "s/generic-icon name=\"wps-office-/generic-icon name=\"${FLATPAK_ID}./g" "export/share/mime/packages/${FLATPAK_ID}.xml"

# Fix hardcoded /opt paths in binaries
sed -i 's|/opt|/app/extra/opt|' usr/bin/*

# use system libraries
rm opt/kingsoft/wps-office/office6/lib{jpeg,stdc++}.so*
if [[ "$CARCH" = "aarch64" ]]; then
    rm opt/kingsoft/wps-office/office6/addons/cef/libm.so*
    rm opt/kingsoft/wps-office/office6/libfreetype.so*
fi

# fix input method
sed -i '2i [[ "$XMODIFIERS" == "@im=fcitx" ]] && export QT_IM_MODULE=fcitx' \
    usr/bin/{wps,wpp,et,wpspdf}

rm -r wps-office.deb
