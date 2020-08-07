#!/bin/bash

BASE_GACS="System System.Core System.Xml System.Configuration I18N I18N.CJK I18N.MidEast I18N.Other I18N.Rare I18N.West Accessibility Mono.Cairo Mono.Posix Mono.CSharp"

mkdir -p /app/etc /app/bin /app/lib/mono/4.5 /app/lib/mono/gac
cp /usr/lib/sdk/mono6/bin/mono /app/bin/mono
cp /usr/lib/sdk/mono6/lib/libMonoPosixHelper.so /app/lib/
cp /usr/lib/sdk/mono6/lib/mono/4.5/*.dll* /app/lib/mono/4.5/
rm -f /app/lib/mono/4.5/Microsoft.CodeAnalysis*
cp -ar /usr/lib/sdk/mono6/etc/mono /app/etc

for G in $BASE_GACS $@; do
    cp -ar /usr/lib/sdk/mono6/lib/mono/gac/$G /app/lib/mono/gac/
    rm -f /app/lib/mono/gac/$G/*/*.pdb
done
