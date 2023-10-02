#!/usr/bin/sh
unzip BFA-Assets-1.3.0.zip
mkdir -p games/doombfa/base
mkdir -p games/doombfa/base_BFG
cp -R BFA-Assets-1.3.0/base/* games/doombfa/base
cp -R BFA-Assets-1.3.0/base_BFG/* games/doombfa/base_BFG
cp -R /app/share/games/doombfa/* games/doombfa
rm -r games/doombfa/base/renderprogs
rm -r BFA-Assets-1.3.0
rm BFA-Assets-1.3.0.zip
rm games/doombfa/base/generate_zBFA_s1.cfg
rm games/doombfa/base/generate_zBFA_s2.cfg
rm games/doombfa/base/DoomBFA.sh