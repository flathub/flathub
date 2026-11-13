#!/bin/bash
set -e

tar -xzf gtask-for-desktop.tar.gz --no-same-owner

chmod -R a+x jre/bin
chmod -R a+x jre/javafx/lib
chmod a+x jre/lib/jspawnhelper

rm -f gtask-for-desktop.tar.gz
