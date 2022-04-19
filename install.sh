#!/bin/bash

set -e
shopt -s extglob
unset {COMMON,WEBKITGTK,WXWIDGETS}_{ENABLED,GI_TYPELIBS,INSTALLED,LIBS,LOCALES} WEBKITGTK_APIVER WXWIDGETS_VER{,_STABLE}
SDK_PATH=/usr/lib/sdk/wxwidgets

COMMON_LIBS=(
  libnotify
  libsecret
)
WXWIDGETS_LIBS=(
  libGLU
  libwx_
)
WXWEBVIEW_LIBS=(
  libwx_gtk3u_webview
)
WEBKITGTK_LIBS=(
  libenchant
  libevdev
  libgudev
  libjavascriptcoregtk
  libmanette
  libwebkit2gtk
  libwoff2common
  libwoff2dec
  libwoff2enc
  libwpe
  libWPEBackend-fdo
)
COMMON_GI_TYPELIBS=(
  Notify
  Secret
)
WEBKITGTK_GI_TYPELIBS=(
  GUdev
  JavaScriptCore
  Manette
  WebKit
  WebKit2WebExtension
)
COMMON_LOCALES=(
  libsecret
)
WXWIDGETS_LOCALES=(
  wxstd
)
WEBKITGTK_LOCALES=(
  WebKit2GTK
)

install_locale() {
  locfile=$1
  cd ${SDK_PATH}
  for f in share/locale/*/LC_MESSAGES/${locfile}; do
    install -vDm644 ${f} ${FLATPAK_DEST}/${f}
  done
}

is_in_array() {
  # 1: array, 2: filename
  local -a 'array=("${'"$1"'[@]}")'
  for e in "${array[@]}"; do
    [[ $2 =~ $e ]] && return 0
  done
  return 1
}

install_lib() {
  local f="$1"
  local fn="$(basename $f)"
  local libn="$(basename ${f%%+(.[0-9])})"
  if [ -z "$fn" ] || [ -z "$libn" ]; then
    echo "Something is wrong! Could not parse filename: $f"
    exit 1
  fi

  install -vDm644 $f ${FLATPAK_DEST}/${f}

  # create symlinks
  for syml in $(find lib/ -maxdepth 1 -type l -name ${libn}'*'); do
    if [ "$syml" = '*' ]; then
      echo 'Something is wrong! Symlink filename is *'
      exit 1
    fi
    echo $syml
    symlt=$(basename $(readlink $syml))
    ln -vs ${symlt} ${FLATPAK_DEST}/${syml}
  done
}


cd ${SDK_PATH}

if WXWIDGETS_VER=$(${SDK_PATH}/bin/wx-config --version 2>/dev/null); then
  echo -e "\nwxWidgets version: $WXWIDGETS_VER\n"
  WXWIDGETS_VER_STABLE="${WXWIDGETS_VER%.*}"
  if [ -n "$WXWIDGETS_VER_STABLE" ]; then
    echo -e "\nwxWidgets stable version: $WXWIDGETS_VER_STABLE\n"
  else
    echo -e "\nFailed detecting wxWidgets major version!\n"
    exit 1
  fi
else
  echo -e "\nFailed detecting wxWidgets version!\n"
  exit 1
fi

WEBKITGTK_APIVER="$(find lib/ -maxdepth 1 -name 'libwebkit2gtk-*.so' -exec basename '{}' \; | sed 's/libwebkit2gtk-\(.*\)\.so/\1/' )"
if [ -n "$WEBKITGTK_APIVER" ]; then
  echo -e "\nWebKitGTK API version: $WEBKITGTK_APIVER\n"
else
  echo "Failed detecting WebKitGTK API version!"
  exit 1
fi

# handle cases where both install-webkitgtk.sh and install-wxwidgets.sh were used instead of install.sh
if [ -e "${FLATPAK_DEST}/lib/libwebkit2gtk-${WEBKITGTK_APIVER}.so" ]; then
  WEBKITGTK_INSTALLED=1
  COMMON_INSTALLED=1
fi

if [ ! -e "lib/libwx_baseu-${WXWIDGETS_VER_STABLE}.so" ]; then
  echo "WxWidgets detection test is outdated!"
  exit 1
fi

if [ -e "${FLATPAK_DEST}/lib/libwx_baseu-${WXWIDGETS_VER_STABLE}.so" ]; then
  WXWIDGETS_INSTALLED=1
  COMMON_INSTALLED=1
fi

EXEC=$(basename $0)
case $EXEC in
  install-webkitgtk.sh)
    WEBKITGTK_ENABLED=1
    ;;
  install-wxwidgets.sh)
    WXWIDGETS_ENABLED=1
    ;;
  *)
    WEBKITGTK_ENABLED=1
    WXWIDGETS_ENABLED=1
    ;;
esac

# ${SDK_PATH}/lib
echo -e "\nInstalling shared libraries...\n"
for f in $(find lib/ -maxdepth 1 -type f); do
  fn="$(basename $f)"
  if [ -z "$fn" ]; then
    echo "Something is wrong! Could not parse filename: $f"
    exit 1
  fi

  if is_in_array COMMON_LIBS ${fn}; then
    if [ -z "$COMMON_INSTALLED" ]; then
      install_lib $f
    fi
  elif is_in_array WXWIDGETS_LIBS ${fn}; then
    # testing also if wxwidgets already installed, and then only add wxwebview
    if [ -n "$WXWIDGETS_ENABLED" ] || [ -n "$WXWIDGETS_INSTALLED" ]; then
      if [[ ${fn} =~ libwx_gtk3u_webview ]]; then
        if [ -n "$WEBKITGTK_ENABLED" ] || [ -n "$WEBKITGTK_INSTALLED" ]; then
          install_lib $f
        fi
      # install everything but wxwebview if wxwidgets is enabled
      elif [ -n "$WXWIDGETS_ENABLED" ]; then
        install_lib $f
      fi
    fi
  elif is_in_array WEBKITGTK_LIBS ${fn}; then
    if [ -n "$WEBKITGTK_ENABLED" ]; then
      install_lib $f
    fi
  else
    echo "Unknown library ${fn}"
    exit 1
  fi
done


# ${SDK_PATH}/lib/girepository*/
echo -e "\nInstalling GObject Introspection's typelibs...\n"

for f in $(find lib/girepository*/ -type f); do
  fn="$(basename $f)"
  if [ -z "$fn" ]; then
    echo "Something is wrong! Could not parse filename: $f"
    exit 1
  fi

  if is_in_array COMMON_GI_TYPELIBS ${fn}; then
    if [ -z "$COMMON_INSTALLED" ]; then
      install -vDm644 $f ${FLATPAK_DEST}/${f}
    fi
  elif is_in_array WEBKITGTK_GI_TYPELIBS ${fn}; then
    if [ -n "$WEBKITGTK_ENABLED" ]; then
      install -vDm644 $f ${FLATPAK_DEST}/${f}
    fi
  else
    echo "Unknown typelib ${fn}"
    exit 1
  fi
done


# types: runtime: lib/webkit, lib/wx/ver share/locale
if [ -n "${WEBKITGTK_ENABLED}" ];then

  # ${SDK_PATH}/lib/webkit2gtk*/
  echo -e "\nInstalling WebKitGTK injected bundle client...\n"
  for f in $(find lib/webkit2gtk*/injected-bundle -type f); do
    install -vDm644 $f ${FLATPAK_DEST}/${f}
  done

  # ${SDK_PATH}/libexec/webkit2gtk*/
  echo -e "\nInstalling WebKitGTK executables...\n"
  for f in $(find libexec/webkit2gtk*/ -type f); do
    install -vDm755 $f ${FLATPAK_DEST}/${f}
  done

  # ${SDK_PATH}/lib/enchant*/
  echo -e "\nInstalling Enchant spell-checkers...\n"
  for f in $(find lib/enchant*/ -type f); do
    install -vDm644 $f ${FLATPAK_DEST}/${f}
  done

  # ${SDK_PATH}/share/enchant/
  echo -e "\nInstalling Enchant ordering file\n"
  for f in $(find share/enchant*/ -type f); do
    install -vDm644 $f ${FLATPAK_DEST}/${f}
  done
fi


# ${SDK_PATH}/lib/wx/wxver/web-extensions/webkit2_extu-*.so
if { [ -n "${WEBKITGTK_ENABLED}" ] && [ -n "${WXWIDGETS_ENABLED}" ]; } ||
  { [ -n "${WEBKITGTK_ENABLED}" ] && [ -n "${WXWIDGETS_INSTALLED}" ]; } ||
  { [ -n "${WEBKITGTK_INSTALLED}" ] && [ -n "${WXWIDGETS_ENABLED}" ]; }; then

    echo -e "\n\Installing wxWebView web extension...\n"
    for f in $(find lib/wx/${WXWIDGETS_VER}/web-extensions/ -type f); do
      install -vDm644 $f ${FLATPAK_DEST}/${f}
    done
fi


# ${SDK_PATH}/share/locale
echo -e "\nInstalling locales...\n"

for f in $(find share/locale/ -type f -name '*.mo'); do
  fn="$(basename $f)"
  if [ -z "$fn" ]; then
    echo "Something is wrong! Could not parse filename: $f"
    exit 1
  fi

  if is_in_array COMMON_LOCALES ${fn}; then
    if [ -z "$COMMON_INSTALLED" ]; then
      install -vDm644 $f ${FLATPAK_DEST}/${f}
    fi
  elif is_in_array WXWIDGETS_LOCALES ${fn}; then
    if [ -n "$WXWIDGETS_ENABLED" ]; then
      install -vDm644 $f ${FLATPAK_DEST}/${f}
    fi
  elif is_in_array WEBKITGTK_LOCALES ${fn}; then
    if [ -n "$WEBKITGTK_ENABLED" ]; then
      install -vDm644 $f ${FLATPAK_DEST}/${f}
    fi
  else
    echo "Unknown locale ${fn}"
    exit 1
  fi
done
