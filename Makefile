APP_ID := org.speedcrunch.SpeedCrunch
RUNTIMES := org.kde.Sdk//5.11 org.kde.Platform//5.11

BUILD := build
DIST := dist
REPO := $(BUILD)/repo

all: $(REPO) $(DIST)/$(APP_ID).flatpak $(DIST)/$(APP_ID).Debug.flatpak

# ----

install: $(BUILD)/$(APP_ID).flatpak
	flatpak install -y --user $(BUILD)/$(APP_ID).flatpak

$(REPO): *.yaml
	flatpak-builder --force-clean --ccache --repo=$@ $(BUILD)/build --state-dir=$(BUILD)/.flatpak-builder $(APP_ID).yaml
	flatpak build-update-repo $(REPO)

$(BUILD)/$(APP_ID).flatpak: $(REPO)
	flatpak build-bundle $(REPO) $@ $(APP_ID)

$(BUILD)/$(APP_ID).Locale.flatpak: $(REPO)
	flatpak build-bundle $(REPO) $@ $(APP_ID).Locale --runtime

$(BUILD)/$(APP_ID).Debug.flatpak: $(REPO)
	flatpak build-bundle $(REPO) $@ $(APP_ID).Debug --runtime

$(DIST):
	mkdir -p $(DIST)

$(DIST)/%.flatpak: $(BUILD)/%.flatpak $(DIST)
	cp $< $@

setup:
	flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
	flatpak install -y flathub $(RUNTIMES)

.PHONY: all install setup
