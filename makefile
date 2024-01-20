flatpak:
	flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
	flatpak install --noninteractive org.gnome.Platform/x86_64/45 org.gnome.Sdk/x86_64/45
	flatpak-builder --repo=repo build finance.reckoner.Reckoner.yaml --force-clean
	flatpak build-bundle repo reckoner.flatpak finance.reckoner.Reckoner

clean:
	rm -rf .flatpak-builder repo build reckoner.flatpak