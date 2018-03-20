all:
	rm -rf openttd 
	flatpak-builder --repo=repo --ccache openttd org.openttd.openttd.json 
	flatpak build-bundle repo openttd-1.7.1.flatpak org.openttd.openttd
clean:
	rm -rf build repo openttd *.flatpak .flatpak-builder
install:
	flatpak install --user --bundle openttd-1.7.1.flatpak
uninstall:
	flatpak uninstall --user org.openttd.openttd
