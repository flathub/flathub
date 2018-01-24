TARGET_REPO = local
FLATPAK_BUILDER = $(shell which flatpak-builder)
MANIFEST = org.mozilla.Firefox.json

all: build

build: $(MANIFEST)
	$(FLATPAK_BUILDER) \
		$(BUILDER_OPTIONS) \
		--repo=$(TARGET_REPO) \
		app \
		$(MANIFEST)

clean:
	rm -rf app
