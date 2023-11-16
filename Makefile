build:
	flatpak install -y org.freedesktop.Sdk//23.08 org.freedesktop.Platform//23.08 org.freedesktop.Sdk.Extension.golang/x86_64/23.08 org.freedesktop.Sdk.Compat.i386/x86_64/23.08 org.freedesktop.Sdk.Extension.toolchain-i386/x86_64/23.08 org.freedesktop.Sdk.Extension.mingw-w64/x86_64/23.08
	flatpak-builder --ccache --force-clean build-dir org.vinegarhq.Vinegar.yml
	flatpak-builder --force-clean --user --install build-dir org.vinegarhq.Vinegar.yml
run:
	flatpak run org.vinegarhq.Vinegar
clean:
	rm -rf .flatpak-builder
	rm -rf build-dir
uninstall:
	flatpak remove org.vinegarhq.Vinegar --delete-data

.PHONY: build run clean uninstall
