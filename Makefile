build:
	flatpak install -y org.freedesktop.Sdk//22.08 org.freedesktop.Platform//22.08 org.freedesktop.Sdk.Extension.golang/x86_64/22.08 org.freedesktop.Sdk.Compat.i386/x86_64/22.08 org.freedesktop.Sdk.Extension.toolchain-i386/x86_64/22.08 org.freedesktop.Sdk.Extension.mingw-w64/x86_64/22.08
	flatpak-builder --ccache --force-clean build-dir io.github.vinegarhq.Vinegar.yml
	flatpak-builder --force-clean --user --install build-dir io.github.vinegarhq.Vinegar.yml
run:
	flatpak run io.github.vinegarhq.Vinegar
clean:
	rm -rf .flatpak-builder
	rm -rf build-dir
uninstall:
	flatpak remove io.github.vinegarhq.Vinegar --delete-data

.PHONY: build run clean uninstall
