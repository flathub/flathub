NAME=com.locomalito.MalditaCastilla
BUNDLE=$(NAME).flatpak
MANIFEST=$(NAME).yaml
APPDATA=$(NAME).appdata.xml
BRANCH_ID=stable

default: $(BUNDLE)

$(BUNDLE): repo
	flatpak build-bundle --arch=i386 repo $(BUNDLE) $(NAME) $(BRANCH_ID)

repo: $(MANIFEST) $(APPDATA)
	flatpak-builder --arch=i386 --repo=repo build-dir $(MANIFEST)

clean:
	rm -f $(BUNDLE)
	rm -rf build-dir repo .flatpak-builder
