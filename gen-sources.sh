#!/bin/bash
if [ ! -d joplin ];then
	url=`grep -oE "https://[^\"]*" ./*.yml | grep joplin`
	wget $url -O joplin.tar && tar -xf joplin.tar
	mv joplin-* joplin
fi

if [ ! -d joplin/git ];then
    mkdir joplin/git
	pushd joplin/git
	git clone https://github.com/laurent22/node-emoji.git
	git clone https://github.com/laurent22/uslug.git
	cd uslug && git checkout emoji-support
	popd
fi

nodegen=./flatpak-builder-tools/node/flatpak-node-generator.py
packages="app-cli app-desktop fork-htmlparser2 renderer turndown fork-sax lib tools turndown-plugin-gfm"

for pack in $packages;do
	echo "add $pack"
	pattern="$pattern -R joplin/packages/$pack/package-lock.json"
done
pattern="$pattern -R joplin/git/uslug/package-lock.json"
pattern="$pattern -R joplin/git/node-emoji/package-lock.json"
$nodegen --xdg-layout -r -R joplin/package-lock.json $pattern npm joplin/package-lock.json
