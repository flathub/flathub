
git clone https://github.com/zen-browser/desktop.git browser

icon_id="com.zen.browser"
icon_repo_path="browser/configs/branding"

function icon {
  local size="$1x$1"
  local brand=$2

  echo "Icon: $size"

  # Initialise dirs
  mkdir -p "icons/$brand/hicolor/$size/apps/"
  mkdir -p "icons/$brand/appdir/$size/"
  # Download files to the correct dir
  cp "$icon_repo_path/$brand/logo$1.png" "icons/$brand/hicolor/$size/apps/$icon_id.png"
  cp "$icon_repo_path/$brand/logo$1.png" "icons/$brand/appdir/$size/$icon_id.png"
}

function branding_icon {
  local brand=$1
  
  icon 16 $brand
  icon 22 $brand
  icon 24 $brand
  icon 32 $brand
  icon 48 $brand
  icon 64 $brand
  icon 128 $brand
  icon 256 $brand
}

branding_icon "alpha"

rm -rf browser