all:
	flatpak-builder --install build com.agateau.Nanonote.yaml --force-clean --user

test:
	flatpak run com.agateau.Nanonote
