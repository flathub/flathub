#!/usr/bin/sh
unzip BFA-Assets.zip
mkdir -p games/doombfa/base
mkdir -p games/doombfa/base_BFG
cp -R base/* games/doombfa/base
cp -R base_BFG/* games/doombfa/base_BFG
cp -R /app/share/games/doombfa/* games/doombfa
rm -r games/doombfa/base/renderprogs
rm -r base
rm -r base_BFG
rm .gitignore
rm *.vdf
rm *.md
rm BFA-Assets.zip
rm games/doombfa/base/generate_zBFA_s1.cfg
rm games/doombfa/base/generate_zBFA_s2.cfg
rm games/doombfa/base/DoomBFA.sh