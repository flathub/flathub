TARGET_REPO = repo
FLATPAK_BUILDER = $(shell which flatpak-builder)
MANIFEST = org.mozilla.Thunderbird.json

all: build

build: $(MANIFEST)
	$(FLATPAK_BUILDER) \
		$(BUILDER_OPTIONS) \
		--repo=$(TARGET_REPO) \
		app \
		$(MANIFEST)

clean:
	rm -rf app
