#!/bin/bash
# This is a copy of the linux build steps, except modified for Flatpak.

# Changes include:
# - removing upload of artifacts to server
# - removing cleanup of artifacts (flatpak builds run from a clean clone)

# cause script to fail as soon as one command has a failing exit code,
# rather than trying to continue. See: https://stackoverflow.com/a/1379904/
set -e

if [[ "$1" != "" ]]; then
   version="$1"
else
   echo -n "Version:"
   read version
fi

if [[ "$2" != "" ]]; then
  destination="$2"
else
  echo -n "Install Destination:"
  read destination
fi

packagecache=""
if [[ "$3" != "" ]]; then
  packagecache="$3"
fi

echo "$packagecache"

printf "Version to deploy: $version\n"

# Build front-end
cd Grayjay.Desktop.Web
npm install --offline
npm run build
cd ..

runtime="linux-x64"

echo "Building for $runtime"

OWD=$(pwd)

# Publish CEF
if [[ -z "$packagecache" ]]; then
  cd Grayjay.Desktop.CEF
  dotnet publish --no-restore -r $runtime -c Release -p:AssemblyVersion=1.$version.0.0
else 
  cd Grayjay.Desktop.CEF
  dotnet publish --source "$packagecache" -r $runtime -c Release -p:AssemblyVersion=1.$version.0.0
fi
cd "${OWD}"

# Copy wwwroot
mkdir -p Grayjay.Desktop.CEF/bin/Release/net8.0/$runtime/publish/wwwroot
cp -r Grayjay.Desktop.Web/dist Grayjay.Desktop.CEF/bin/Release/net8.0/$runtime/publish/wwwroot/web

cd Grayjay.Desktop.CEF/bin/Release/net8.0/$runtime/publish	

chmod u=rwx Grayjay
chmod u=rwx cef/dotcefnative
chmod u=rwx FUTO.Updater.Client
chmod u=rwx ffmpeg

cd ../
mv publish/* "${destination}"

cd "${OWD}"


