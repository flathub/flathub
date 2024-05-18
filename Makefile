APP_ID = dev.zed.Zed
MANIFEST = dev.zed.Zed.yaml
BUNDLE = $(APP_ID).flatpak
EXTENSIONS = org.freedesktop.Sdk.Extension.rust-stable//23.08 org.freedesktop.Platform.GL.default//23.08 org.flatpak.Builder///master

all: build

build: ensure-extensions sources
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

sources:
	#rm $(APP_ID)-cargo-sources.yaml
	python3 $(HOME)Developer/flathub/flatpak-builder-tools/cargo/flatpak-cargo-generator.py $(HOME)Developer/zed-industries/zed/Cargo.lock -o zed-cargo-sources.json
	# $(HOME)Developer/flathub/flatpak-builder-tools/flatpak-json2yaml.py $(APP_ID)-cargo-sources.json --output $(APP_ID)-cargo-sources.yaml
	# rm $(APP_ID)-cargo-sources.json
