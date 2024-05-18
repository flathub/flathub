APP_ID = dev.zed.Zed
MANIFEST = dev.zed.Zed.yaml
BUNDLE = $(APP_ID).flatpak
EXTENSIONS = org.freedesktop.Sdk.Extension.rust-stable//23.08 org.freedesktop.Platform.GL.default//23.08 org.flatpak.Builder///master

FLATHUB_REPO = $(PWD)
ZED_REPO = $(HOME)/Developer/zed-industries/zed
CARGO_LOCK_PATH = $(ZED_REPO)/Cargo.lock
CARGO_TOML_PATH = $(ZED_REPO)/Cargo.toml
SOURCE_PATH = $(FLATHUB_REPO)/sources

SOURCE_UPDATE_SCRIPT = $(SOURCE_PATH)/update_and_diff.sh
CARGO_SOURCES_JSON = $(SOURCE_PATH)/Cargo.sources.json
CARGO_GENERATOR_SCRIPT = $(HOME)/Developer/flathub/flatpak-builder-tools/cargo/flatpak-cargo-generator.py

all: build

build: ensure-extensions sources update-sources
	flatpak-builder build-dir $(MANIFEST) --force-clean --ccache

install:
	flatpak-builder build-dir $(MANIFEST) --force-clean --install --user

uninstall:
	flatpak uninstall --user $(APP_ID) --delete-data --assumeyes

run:
	flatpak run $(APP_ID)

clean:
	rm -rf build-dir $(BUNDLE)

ensure-extensions:
	flatpak install $(EXTENSIONS) --assumeyes

lint:
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest $(MANIFEST)

sources: update-sources
	python3 $(CARGO_GENERATOR_SCRIPT) $(CARGO_LOCK_PATH) -o $(CARGO_SOURCES_JSON)

update-sources:
	@echo "Updating Cargo.lock sources..."
	@$(SOURCE_UPDATE_SCRIPT) $(ZED_REPO) $(FLATHUB_REPO)

diff:
	@./patches/update_and_diff.sh $(ZED_REPO) $(FLATHUB_REPO)
