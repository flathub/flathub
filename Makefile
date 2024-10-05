NAME=net.sourceforge.fuse_emulator.Fuse
BUNDLE=$(NAME).flatpak
MANIFEST=$(NAME).yaml
APPDATA=$(NAME).appdata.xml
BRANCH_ID=master

default: $(BUNDLE)

$(BUNDLE): repo
	flatpak build-bundle repo $(BUNDLE) $(NAME) $(BRANCH_ID)

repo: $(MANIFEST) $(APPDATA) $(wildcard *.patch)
	flatpak-builder --ccache --force-clean --repo=repo build-dir $(MANIFEST)

clean:
	rm -f $(BUNDLE)
	rm -rf build-dir repo .flatpak-builder
