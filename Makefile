all:
	flatpak-builder --install build com.agateau.nanonote.yaml --force-clean --user

test:
	flatpak run com.agateau.nanonote
