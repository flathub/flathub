#!/bin/sh

set -eu

bsdtar -xf szafir_Linux.zip szafir_703.jar libCCGraphiteP11.2.0.5.6.so
rm szafir_Linux.zip

for size in 16 24 32 48 64; do
	out_dir="/app/extra/export/share/icons/hicolor/${size}x${size}/apps"
	install -d "$out_dir"
	bsdtar -xOf szafir_703.jar "pl/com/kir/szafir/resources/image/icon-application${size}.png" > "$out_dir/pl.deno.kir.szafir.png"
done
