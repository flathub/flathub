#!/bin/sh

export WINEPREFIX=${HOME}/.npp
NPP_ROOT="${WINEPREFIX}/drive_c/Program Files/Notepad++"

function install_color_theme() {
  REG_FILE=$(mktemp)
cat > "$REG_FILE" <<EOL
Windows Registry Editor Version 5.00
[HKEY_CURRENT_USER\Control Panel\Colors]
"Scrollbar"="212 208 200"
"Background"="33 87 141"
"ActiveTitle"="230 230 230"
"GradientActiveTitle"="230 230 230"
"InactiveTitle"="255 255 255"
"GradientInactiveTitle"="255 255 255"
"Menu"="255 255 255"
"Window"="255 255 255"
"WindowFrame"="0 0 0"
"MenuText"="0 0 0"
"WindowText"="0 0 0"
"TitleText"="0 0 0"
"ActiveBorder"="212 208 200"
"InactiveBorder"="212 208 200"
"AppWorkSpace"="128 128 128"
"Hilight"="230 230 230"
"HilightText"="0 0 0"
"ButtonFace"="240 240 240"
"ButtonShadow"="162 162 162"
"GrayText"="172 168 153"
"ButtonText"="0 0 0"
"InactiveTitleText"="180 180 180"
"ButtonHilight"="255 255 255"
"ButtonDkShadow"="162 162 162"
"ButtonLight"="241 239 226"
"InfoText"="0 0 0"
"InfoWindow"="255 255 225"
"ButtonAlternateFace"="181 181 181"
"HotTrackingColor"="0 0 128"
"MenuHilight"="220 220 220"
"MenuBar"="239 238 243"
EOL
  wine64 regedit /s "$REG_FILE"
  rm "$REG_FILE"
}

function set_dpi() {
  echo "Setting dpi to $1"
  wine64 reg add "HKLM\System\CurrentControlSet\Hardware Profiles\Current\Software\Fonts" /v LogPixels /t REG_DWORD /d $1 /f
}

# https://github.com/Winetricks/winetricks/blob/master/src/winetricks#L21708
function set_font_smoothing() {
  case "$1" in
    disable)   FontSmoothing=0; FontSmoothingOrientation=1; FontSmoothingType=0;;
    gray|grey) FontSmoothing=2; FontSmoothingOrientation=1; FontSmoothingType=1;;
    bgr)       FontSmoothing=2; FontSmoothingOrientation=0; FontSmoothingType=2;;
    rgb)       FontSmoothing=2; FontSmoothingOrientation=1; FontSmoothingType=2;;
    *)         return;;
  esac
  echo "Setting font smoothing to $1"
  REG_FILE=$(mktemp)
  cat > "$REG_FILE" <<EOL
REGEDIT4
[HKEY_CURRENT_USER\\Control Panel\\Desktop]
"FontSmoothing"="${FontSmoothing}"
"FontSmoothingGamma"=dword:00000578
"FontSmoothingOrientation"=dword:0000000${FontSmoothingOrientation}
"FontSmoothingType"=dword:0000000${FontSmoothingType}
EOL
  wine64 regedit /s "$REG_FILE"
  rm "$REG_FILE"
}

function select_ui_font() {
  find /run/host/fonts /run/host/local-fonts -iname '*sans*' -exec fc-scan -f "%{family}\t:%{style}:%{lang}\n" {} \; |\
    grep -E '\s+:Regular:[a-z\|\-]*en[a-z\|\-]*$' | grep -Ev 'Mono|CJK' | awk 'BEGIN {FS="\t"}; {print $1}' | sort -u | {
      LIST_ITEMS=""
      unset FONTS
      readarray FONTS
    for CURRENT_FONT in "${FONTS[@]}"; do
      CURRENT_FONT=$(echo "$CURRENT_FONT" | tr -d '\n')
      LIST_ITEMS="${LIST_ITEMS} FALSE '${CURRENT_FONT}'"
    done
    sh -c "zenity --width=600 --height=400 --list --text='UI Font' --radiolist --column='' --column='Font Family' TRUE Default ${LIST_ITEMS}"
    }
}

function set_ui_font() {
  echo "Setting UI font to ${1}"
  wine64 REG ADD "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes" /v "MS Shell Dlg" /t REG_SZ /d "$1" /f
  wine64 REG ADD "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes" /v "MS Shell Dlg 2" /t REG_SZ /d "$1" /f
}

function initialize_wine() {
  # initialize prefix
  wineboot

  # prevent wine from making new file associations
  wine64 reg add "HKLM\Software\Microsoft\Windows\CurrentVersion\RunServices" /v winemenubuilder /t REG_SZ /d "C:\windows\system32\winemenubuilder.exe -r" /f

  install_color_theme

  SELECTED_DPI=$(zenity --width=600 --height=400 --list --text="Screen Resolution" --radiolist --column="" --column="DPI" TRUE 96 FALSE 120 FALSE 144 FALSE 168 FALSE 192 FALSE 216 FALSE 240 FALSE 288 FALSE 336 FALSE 384 FALSE 432 FALSE 480)
  if [ "$SELECTED_DPI" ]; then
    set_dpi $SELECTED_DPI
  fi

  SELECTED_FONT_SMOOTHING=$(zenity --width=600 --height=400 --list --text="Font Smoothing" --radiolist --column="" --column="Sub-pixel Rendering" TRUE default FALSE disable FALSE gray FALSE bgr FALSE rgb)
  if [ "$SELECTED_FONT_SMOOTHING" ]; then
    set_font_smoothing $SELECTED_FONT_SMOOTHING
  fi

  SELECTED_UI_FONT=$(select_ui_font)
  if [ ! -z "$SELECTED_UI_FONT" ] && [ "$SELECTED_UI_FONT" != "Default" ]; then
    set_ui_font "$SELECTED_UI_FONT"
  fi
}

# initialize wine and install npp if prefix directory doesn't exist or is empty
if [ ! -d "$WINEPREFIX/drive_c" ]; then
  initialize_wine
  mkdir -p "$NPP_ROOT"
  tar xf /app/data/npp.tar.xz -C "$NPP_ROOT"
fi

wine64 "${NPP_ROOT}/notepad++.exe" "$@"
