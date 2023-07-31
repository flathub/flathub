

compile

	flatpak-builder --repo=wetgenes --force-clean --install-deps-from=flathub build-dir com.wetgenes.gamecake.yaml


install

	flatpak remote-add wetgenes wetgenes --no-gpg-verify
	flatpak install wetgenes com.wetgenes.gamecake



