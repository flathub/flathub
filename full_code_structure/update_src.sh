#!/bin/bash
rm -r flatpak/src
cp -r src flatpak

rm -r flatpak/wrapper
cp -r wrapper flatpak
