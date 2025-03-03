#!/bin/bash

wget -N -O "Grayjay.Desktop-linux-x64.zip" https://updater.grayjay.app/Apps/Grayjay.Desktop/Grayjay.Desktop-linux-x64.zip

mkdir -p ~/.var/app/app.grayjay.Desktop/data/Grayjay/
unzip -o "Grayjay.Desktop-linux-x64.zip" "Grayjay.Desktop-linux-x64-v5/grayjay.png" -d "./"
unzip -o "Grayjay.Desktop-linux-x64.zip" "Grayjay.Desktop-linux-x64-v5/wwwroot/*" -d "./"

cp -r Grayjay.Desktop-linux-x64-v5/* ~/.var/app/app.grayjay.Desktop/data/Grayjay/


rm -f "Grayjay.Desktop-linux-x64.zip"
rm -rf "Grayjay.Desktop-linux-x64-v5/"