#!/bin/bash
if [ ! -d ~/.var/app/net.xmind.XMind8/data/app ] || [ ~/.var/app/net.xmind.XMind8/data/app -ot /app/extra/xmind.zip ]; then
    mkdir -p ~/.var/app/net.xmind.XMind8/data/app ||:
    cd ~/.var/app/net.xmind.XMind8/data/app
    if [ $(uname -m) = "x86_64" ]; then
        unzip -uo /app/extra/xmind.zip -x XMind_i386* fonts* &>/dev/null
        ln -s XMind_amd64 XMind ||:
    else
        unzip -uo /app/extra/xmind.zip -x XMind_amd64* fonts* &>/dev/null
        ln -s XMind_i386 XMind ||:
    fi
fi
cd ~/.var/app/net.xmind.XMind8/data/app/XMind
export PATH=/app/jre/bin:$PATH
export JAVA_HOME=/app/jre
exec ./XMind
