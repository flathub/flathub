#!/bin/bash
set -e
APPDIR=~/.var/app/net.xmind.XMind8/data/.app
STAMP_FILE=$APPDIR/stamp
if [ ! -f $STAMP_FILE ] || [ $STAMP_FILE -ot /app/extra/xmind.zip ]; then
    mkdir -p $APPDIR ||:
    cd $APPDIR
    if [ $(uname -m) = "x86_64" ]; then
        unzip -o /app/extra/xmind.zip -x XMind_i386* fonts* &>/dev/null
        ln -s XMind_amd64 XMind ||:
    else
        unzip -o /app/extra/xmind.zip -x XMind_amd64* fonts* &>/dev/null
        ln -s XMind_i386 XMind ||:
    fi
    touch -r /app/extra/xmind.zip $STAMP_FILE
fi
cd $APPDIR/XMind
export PATH=/app/jre/bin:$PATH
export JAVA_HOME=/app/jre
exec ./XMind
