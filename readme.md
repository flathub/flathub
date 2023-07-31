

test compile

	flatpak-builder --repo=wetgenes --force-clean --install-deps-from=flathub build-dir com.wetgenes.gamecake.yaml


test install

	flatpak remote-add wetgenes wetgenes --no-gpg-verify
	flatpak install wetgenes com.wetgenes.gamecake


test delte

	flatpak remove com.wetgenes.gamecake
	flatpak remote-delete wetgenes



