NAME=net.sf.fuse_emulator
BUNDLE=$(NAME).flatpak
MANIFEST=$(NAME).json
APPDATA=$(NAME).appdata.xml
BRANCH_ID=stable

default: $(BUNDLE)

$(BUNDLE): repo
	flatpak build-bundle repo $(BUNDLE) $(NAME) $(BRANCH_ID)

repo: $(MANIFEST) $(APPDATA) $(wildcard *.patch)
	flatpak-builder --ccache --force-clean --repo=repo build-dir $(MANIFEST)

clean:
	rm -f $(BUNDLE)
	rm -rf build-dir repo .flatpak-builder
