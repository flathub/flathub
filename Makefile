
PROJECT ?= tv.kodi.Kodi

build:
	flatpak-builder build-dir $(PROJECT).json --repo=repo --force-clean --ccache 2>&1 | tee -a build.log
build-i386:
	flatpak-builder build-dir $(PROJECT).json --repo=repo --force-clean --ccache --arch=i386 2>&1 | tee -a build-i386.log
flatpak:
	flatpak build-bundle repo $(PROJECT).flatpak $(PROJECT)
install:
	flatpak install local $(PROJECT)
run:
	flatpak update -y $(PROJECT)
	flatpak run $(PROJECT)
clean:
	rm -rf .flatpak-builder/cache
format:
	for n in *json deps/*json; do python -m json.tool < $$n > a && mv a $$n || echo $$n ; done

.PHONY: addons
addons:
	@mkdir -p addons
	@rm -rf repo-binary-addons
	@git clone https://github.com/xbmc/repo-binary-addons
	python3 binary-addons.py repo-binary-addons addons
