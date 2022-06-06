#!/usr/bin/sh

ar x freedownloadmanager.deb data.tar.xz
rm freedownloadmanager.deb
tar -xf data.tar.xz ./opt
rm data.tar.xz
