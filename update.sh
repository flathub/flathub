# Whether or not to skip git commands
NO_GIT=false
# Whether or not to check if the json needs updating
NO_CHECK_NEEDS_UPDATING=false

for arg in "$@"; do
  case $arg in
    --no-git)
      NO_GIT=true
      shift
      ;;
    --no-check)
      NO_CHECK_NEEDS_UPDATING=true
      shift
      ;;
    --help)
      echo "Usage: update.sh [options]"
      echo "Options:"
      echo "  --no-git"
      echo "    Skips git commands"
      echo "  --no-check"
      echo "    Skips checking whether the json needs updating"
      echo "  --help"
      echo "    Shows this help message"
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg"
      exit 1
      ;;
  esac
done

echo "Updating com.adilhanney.ricochlime.json"
if [ $NO_GIT == true ]; then
  echo "- Git commands disabled"
fi
if [ $NO_CHECK_NEEDS_UPDATING == true ]; then
  echo "- Not checking if json needs updating"
fi
echo "---------------------------------"

echo -n "1. Make sure we're on the master branch... "
if [ $NO_GIT == true ]; then
  echo "Skipped"
else
  git checkout master
  git fetch origin
  git reset --hard origin/master
fi

echo -n "2. Finding latest tag and archive... "
json=$(curl -s https://api.github.com/repos/adil192/ricochlime/releases/latest)
LATEST_TAG=$(echo "$json" | jq -r ".tag_name")
ARCHIVE_NAME=$(echo "$json" | jq -r ".assets[].name" | grep "Ricochlime_.*.tar.gz")
echo "Found $LATEST_TAG and $ARCHIVE_NAME"

echo -n "3. Check if json needs updating... "
if [ $NO_CHECK_NEEDS_UPDATING == true ]; then
  echo "Skipped"
else
  if grep -q "adil192/ricochlime/$LATEST_TAG" com.adilhanney.ricochlime.json; then
    echo "No"
    exit 1
  else
    echo "Yes"
  fi
fi

echo -n "4. Hashing $ARCHIVE_NAME... "
curl -L -o "$ARCHIVE_NAME" https://github.com/adil192/ricochlime/releases/latest/download/$ARCHIVE_NAME
ARCHIVE_HASH=$(sha256sum "$ARCHIVE_NAME" | cut -d ' ' -f 1)
rm "$ARCHIVE_NAME"
echo "Hashed $ARCHIVE_HASH"

echo -n "5. Hashing com.adilhanney.ricochlime.metainfo.xml"
curl -L -o com.adilhanney.ricochlime.metainfo.xml https://raw.githubusercontent.com/adil192/ricochlime/$LATEST_TAG/flatpak/com.adilhanney.ricochlime.metainfo.xml
METAINFO_HASH=$(sha256sum com.adilhanney.ricochlime.metainfo.xml | cut -d ' ' -f 1)
rm com.adilhanney.ricochlime.metainfo.xml
echo "Hashed $METAINFO_HASH"

echo -n "6. Updating com.adilhanney.ricochlime.json... "
# com.adilhanney.ricochlime.metainfo.xml
sed -i "s/\"url\": \"https:\/\/raw.githubusercontent.com\/adil192\/ricochlime\/v[0-9]\+\.[0-9]\+\.[0-9]\+\/flatpak\/com.adilhanney.ricochlime.metainfo.xml\", \"sha256\": \"[a-z0-9]\{64\}\"/\"url\": \"https:\/\/raw.githubusercontent.com\/adil192\/ricochlime\/$LATEST_TAG\/flatpak\/com.adilhanney.ricochlime.metainfo.xml\", \"sha256\": \"$METAINFO_HASH\"/g" com.adilhanney.ricochlime.json
# Replace Ricochlime_v0.12.5.tar.gz ($ARCHIVE_NAME)
sed -i "s/\"url\": \"https:\/\/github.com\/adil192\/ricochlime\/releases\/download\/v[0-9]\+\.[0-9]\+\.[0-9]\+\/Ricochlime_v[0-9]\+\.[0-9]\+\.[0-9]\+\.tar\.gz\", \"sha256\": \"[a-z0-9]\{64\}\"/\"url\": \"https:\/\/github.com\/adil192\/ricochlime\/releases\/download\/$LATEST_TAG\/$ARCHIVE_NAME\", \"sha256\": \"$ARCHIVE_HASH\"/g" com.adilhanney.ricochlime.json
echo "Done"

echo -n "7. Committing changes... "
if [ $NO_GIT == true ]; then
  echo "Skipped"
  exit 0
fi
git checkout -b update/"$LATEST_TAG"
git add com.adilhanney.ricochlime.json
git commit -m "$LATEST_TAG"
git push --set-upstream origin update/"$LATEST_TAG"
echo "Done"

echo -n "8. Creating pull request... "
PULL_REQUEST_URL=https://github.com/flathub/com.adilhanney.ricochlime/pull/new/update/"$LATEST_TAG"
echo "See browser, or click here:"
echo "    $PULL_REQUEST_URL"
xdg-open "$PULL_REQUEST_URL"

echo "Done!"
