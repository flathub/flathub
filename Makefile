NAME=com.locomalito.MalditaCastilla
BUNDLE=$(NAME).flatpak
MANIFEST=$(NAME).yaml
APPDATA=$(NAME).metainfo.xml
BRANCH_ID=stable

default: $(BUNDLE)

$(BUNDLE): repo
	flatpak build-bundle repo $(BUNDLE) $(NAME) $(BRANCH_ID)

repo: $(MANIFEST) $(APPDATA)
	flatpak-builder --repo=repo build-dir $(MANIFEST)

clean:
	rm -f $(BUNDLE)
	rm -rf build-dir repo .flatpak-builder
