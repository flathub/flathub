

test compile

	flatpak-builder --repo=wetgenes --force-clean --install-deps-from=flathub build-dir com.wetgenes.gamecake.yaml


test install

	flatpak remote-add wetgenes wetgenes --no-gpg-verify
	flatpak remove com.wetgenes.gamecake
	flatpak install wetgenes com.wetgenes.gamecake



