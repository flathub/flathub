REPO ?= build/flatpak-repo
STATE ?= build/flatpak-builder
TARGET ?= build/flatpak-target 
APP_ID = com.github.k4zmu2a.spacecadetpinball
ARCH ?= $(shell arch)

all:
	flatpak-builder --ccache --force-clean --arch=$(ARCH) --state-dir=$(STATE) --repo=$(REPO) $(TARGET) $(APP_ID).yml
clean:
	rm -rf $(STATE) $(REPO) $(TARGET) *.flatpak .flatpak-builder
dist:
	flatpak build-bundle --arch=$(ARCH) $(REPO) $(APP_ID)-$(ARCH).flatpak $(APP_ID)
install:
	flatpak install --reinstall --or-update -y -v --user $(shell realpath $(REPO)) $(APP_ID)
run:
	flatpak run -v $(APP_ID)
uninstall:
	flatpak uninstall --user $(APP_ID)
check:
	flatpak run org.freedesktop.appstream-glib validate $(APP_ID).metainfo.xml
	flatpak run org.flathub.flatpak-external-data-checker $(APP_ID).yml