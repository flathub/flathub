build-dir:
	flatpak-builder --user --force-clean build-dir org.codeberg.dnkl.foot.yml

install:
	flatpak-builder --user --install --force-clean build-dir org.codeberg.dnkl.foot.yml

run:
	flatpak run --user org.codeberg.dnkl.foot

clean:
	rm -rf build-dir
