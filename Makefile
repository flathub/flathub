.DEFAULT_GOAL := standard
ID := org.coolercontrol.CoolerControl

BUILD := build
DIST := dist
REPO := $(BUILD)/repo

.PHONY: standard setup build bundle run clean

standard: setup build

# local node_modules folder needs to be removed due to:
# https://github.com/flatpak/flatpak-builder-tools/issues/377
setup:
	@flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
	@-$(RM) -f cargo-sources.json
	@-$(RM) -f node-sources.json
	@python3 -m pip install ./flatpak-builder-tools/node/
	@python3 ./flatpak-builder-tools/cargo/flatpak-cargo-generator.py ../coolercontrol/coolercontrol-ui/src-tauri/Cargo.lock -o cargo-sources.json
	@-$(RM) -rf ../coolercontrol/coolercontrol-ui/node_modules
	@flatpak-node-generator -r npm ../coolercontrol/coolercontrol-ui/package-lock.json -o node-sources.json

build:
	@flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/repo/screenshots --repo=$(REPO) $(BUILD) $(ID).yml
	@flatpak build-update-repo $(REPO)

bundle:
	@flatpak build-bundle $(REPO) coolercontrol.flatpak $(ID)

run:
	@flatpak run $(ID) --debug

clean:
	@-$(RM) -f cargo-sources.json
	@-$(RM) -f node-sources.json
	@-$(RM) -f coolercontrol.flatpak
	@-$(RM) -rf build/
	@-$(RM) -rf .flatpak-builder/
	@-flatpak remove $(ID)
