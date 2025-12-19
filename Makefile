# BrowserOS Flatpak Makefile
# Provides targets for cleaning, building, testing, and uninstalling

# Variables
APP_ID = com.browseros.BrowserOS
MANIFEST = com.browseros.BrowserOS.yml
REPO_DIR = browseros_repo
BUILD_DIR = build-dir
FLATPAK_BUILDER = flatpak-builder

# Default target
.PHONY: all
all: build

# Help target
.PHONY: help
help:
	@echo "BrowserOS Flatpak Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  help        - Show this help message"
	@echo "  clean       - Clean build artifacts and temporary files"
	@echo "  build       - Build the flatpak package"
	@echo "  lint        - Lint the flatpak manifest"
	@echo "  lint-repo   - Lint the built repository (requires build first)"
	@echo "  install     - Install the flatpak package"
	@echo "  test        - Test the installed flatpak"
	@echo "  run         - Run the application"
	@echo "  uninstall   - Uninstall the flatpak package"
	@echo "  clean-all   - Remove all generated files including cache"
	@echo "  update-repo - Update the flatpak repository"
	@echo "  update-version - Update BrowserOS to latest version"
	@echo "  check-version - Check for available updates"
	@echo "  show-version - Show current version information"
	@echo "  backup-files - Create backups of configuration files"
	@echo "  restore-files - Restore files from latest backup"
	@echo "  clean-backups - Remove backup directory"
	@echo ""

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -rf squashfs-root
	rm -f *.AppImage
	rm -f .flatpak-builder/lock
	@echo "Build artifacts cleaned."

# Clean everything including cache
.PHONY: clean-all
clean-all: clean
	@echo "Cleaning all generated files..."
	rm -rf .flatpak-builder
	rm -rf $(REPO_DIR)
	@echo "All generated files cleaned."

# Build the flatpak package
.PHONY: build
build:
	@echo "Building BrowserOS flatpak..."
	$(FLATPAK_BUILDER) --force-clean --repo=$(REPO_DIR) $(BUILD_DIR) $(MANIFEST)
	@echo "Build completed successfully."

# Install the flatpak package
.PHONY: install
install:
	@echo "Installing BrowserOS flatpak..."
	@if ! flatpak --user remote-list | grep -q "browseros-local"; then \
		flatpak --user remote-add --no-gpg-verify browseros-local $(REPO_DIR); \
	fi
	flatpak --user install -y --reinstall browseros-local $(APP_ID)
	@echo "Installation completed."

# Test the installed flatpak
.PHONY: test
test:
	@echo "Testing BrowserOS flatpak installation..."
	@if flatpak --user info $(APP_ID) > /dev/null 2>&1; then \
		echo "✓ $(APP_ID) is installed"; \
		echo "Application info:"; \
		flatpak --user info $(APP_ID); \
	else \
		echo "✗ $(APP_ID) is not installed"; \
		exit 1; \
	fi

# Run the application
.PHONY: run
run:
	@echo "Running BrowserOS..."
	flatpak run $(APP_ID)

# Uninstall the flatpak package
.PHONY: uninstall
uninstall:
	@echo "Uninstalling BrowserOS flatpak..."
	@if flatpak --user info $(APP_ID) > /dev/null 2>&1; then \
		flatpak --user uninstall $(APP_ID); \
		echo "$(APP_ID) uninstalled successfully."; \
	else \
		echo "$(APP_ID) is not installed."; \
	fi

# Update the flatpak repository
.PHONY: update-repo
update-repo:
	@echo "Updating flatpak repository..."
	flatpak build-update-repo $(REPO_DIR)
	@echo "Repository updated."

# Show package information
.PHONY: info
info:
	@echo "BrowserOS Flatpak Information:"
	@echo "  App ID: $(APP_ID)"
	@echo "  Manifest: $(MANIFEST)"
	@echo "  Build Dir: $(BUILD_DIR)"
	@echo "  Repo Dir: $(REPO_DIR)"
	@if [ -f $(MANIFEST) ]; then \
		echo ""; \
		echo "Manifest details:"; \
		grep -E "^(app-id|runtime|runtime-version|command)" $(MANIFEST) || true; \
	fi

# Check dependencies
.PHONY: check-deps
check-deps:
	@echo "Checking dependencies..."
	@command -v flatpak >/dev/null 2>&1 || { echo "flatpak is required but not installed." >&2; exit 1; }
	@command -v $(FLATPAK_BUILDER) >/dev/null 2>&1 || { echo "flatpak-builder is required but not installed." >&2; exit 1; }
	@echo "Dependencies check passed."

# Install remotes (if needed)
.PHONY: setup-remotes
setup-remotes:
	@echo "Setting up flatpak remotes..."
	flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
	@echo "Remotes setup completed."

# Lint the flatpak manifest
.PHONY: lint
lint:
	@echo "Linting BrowserOS flatpak manifest..."
	@if flatpak info org.flatpak.Builder >/dev/null 2>&1; then \
		flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest $(MANIFEST); \
	else \
		echo "org.flatpak.Builder is not installed. Installing..."; \
		flatpak install -y flathub org.flatpak.Builder; \
		flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest $(MANIFEST); \
	fi
	@echo "Linting completed."

# Lint the built repository (requires build first)
.PHONY: lint-repo
lint-repo:
	@echo "Linting BrowserOS flatpak repository..."
	@if [ ! -d "$(REPO_DIR)" ]; then \
		echo "Repository directory $(REPO_DIR) not found. Run 'make build' first."; \
		exit 1; \
	fi
	@if flatpak info org.flatpak.Builder >/dev/null 2>&1; then \
		flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo $(REPO_DIR); \
	else \
		echo "org.flatpak.Builder is not installed. Installing..."; \
		flatpak install -y flathub org.flatpak.Builder; \
		flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo $(REPO_DIR); \
	fi
	@echo "Repository linting completed."

# Full setup (deps + remotes + install)
.PHONY: setup
setup: check-deps setup-remotes install
	@echo "Full setup completed."

# Configuration
GITHUB_API := https://api.github.com/repos/browseros-ai/BrowserOS/releases/latest
BACKUP_DIR := backups
TIMESTAMP := $(shell date +%Y%m%d_%H%M%S)

# Update BrowserOS to latest version
.PHONY: update-version
update-version: _check-version-and-show
	@$(MAKE) _update-or-skip

.PHONY: _update-or-skip
_update-or-skip:
	@if [ ! -f .version-info ]; then echo "Error: .version-info not found"; exit 1; fi
	@eval $$(cat .version-info | grep -v '^RELEASE_BODY='); \
	if [ "$$CURRENT_VERSION" = "$$NEW_VERSION" ]; then \
		echo "No update needed, already at latest version $$CURRENT_VERSION"; \
		rm -f .version-info; \
	else \
		echo "Updating BrowserOS from $$CURRENT_VERSION to $$NEW_VERSION..."; \
		$(MAKE) backup-files; \
		$(MAKE) _apply-updates; \
		$(MAKE) _validate-updates; \
		echo "Update completed successfully!"; \
	fi

# Check for updates without applying
.PHONY: check-version
check-version:
	@echo "Checking for BrowserOS updates..."
	@$(MAKE) _check-version-and-show

# Internal targets (prefixed with _ to hide from help)
.PHONY: _check-version-and-show
_check-version-and-show:
	@if ! command -v yq >/dev/null 2>&1; then \
		echo "Error: yq is required but not installed"; exit 1; \
	fi
	@if ! command -v xmlstarlet >/dev/null 2>&1; then \
		echo "Error: xmlstarlet is required but not installed"; exit 1; \
	fi
	@echo "Fetching latest release information..."
	@if ! LATEST_RELEASE=$$(curl -s "$(GITHUB_API)"); then \
		echo "Error: Failed to fetch release information"; exit 1; \
	fi; \
	LATEST_VERSION=$$(echo "$$LATEST_RELEASE" | yq '.tag_name' | sed 's/^v//'); \
	APPIMAGE_NAME=$$(echo "$$LATEST_RELEASE" | yq '.assets[] | select(.name | test(".*_x64.AppImage$$")) | .name'); \
	APPIMAGE_URL=$$(echo "$$LATEST_RELEASE" | yq '.assets[] | select(.name | test(".*_x64.AppImage$$")) | .browser_download_url'); \
	SHA256=$$(echo "$$LATEST_RELEASE" | yq '.assets[] | select(.name | test(".*_x64.AppImage$$")) | .digest' | sed 's/sha256://'); \
	RELEASE_DATE=$$(echo "$$LATEST_RELEASE" | yq '.published_at' | cut -d'T' -f1); \
	RELEASE_BODY=$$(echo "$$LATEST_RELEASE" | yq '.body' | tr '\n' '│' | sed 's/"/\\"/g'); \
	CURRENT_VERSION=$$(yq eval '.modules[0].sources[4].url' com.browseros.BrowserOS.yml | grep -o 'BrowserOS_v[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+' | sed 's/BrowserOS_v//' || echo "unknown"); \
	NEW_VERSION=$$(echo "$$APPIMAGE_NAME" | sed 's/BrowserOS_v//' | sed 's/_x64\.AppImage//'); \
	echo "LATEST_VERSION=$$LATEST_VERSION" > .version-info; \
	echo "APPIMAGE_NAME=$$APPIMAGE_NAME" >> .version-info; \
	echo "APPIMAGE_URL=$$APPIMAGE_URL" >> .version-info; \
	echo "SHA256=$$SHA256" >> .version-info; \
	echo "RELEASE_DATE=$$RELEASE_DATE" >> .version-info; \
	echo "RELEASE_BODY=\"$$RELEASE_BODY\"" >> .version-info; \
	echo "CURRENT_VERSION=$$CURRENT_VERSION" >> .version-info; \
	echo "NEW_VERSION=$$NEW_VERSION" >> .version-info; \
	if [ "$$CURRENT_VERSION" = "$$NEW_VERSION" ]; then \
		echo "✓ BrowserOS is up to date (version $$CURRENT_VERSION)"; \
	else \
		echo "→ Update available: $$CURRENT_VERSION → $$NEW_VERSION"; \
	fi

.PHONY: _apply-updates
_apply-updates:
	@if [ ! -f .version-info ]; then echo "Error: .version-info not found"; exit 1; fi
	@eval $$(cat .version-info | grep -v '^RELEASE_BODY='); \
	if [ -n "$(DRY_RUN)" ]; then \
		echo "DRY RUN: Would update from $$CURRENT_VERSION to $$NEW_VERSION"; \
		echo "YAML changes:"; \
		echo "  URL: $$APPIMAGE_URL"; \
		echo "  SHA256: $$SHA256"; \
		echo "  Filename: $$APPIMAGE_NAME"; \
		echo "XML changes:"; \
		echo "  New release entry for version $$NEW_VERSION"; \
		rm -f .version-info; \
	else \
		echo "Applying updates..."; \
		$(MAKE) _update-yaml; \
		$(MAKE) _update-xml; \
	fi

.PHONY: _update-yaml
_update-yaml:
	@eval $$(cat .version-info | grep -v '^RELEASE_BODY='); \
	echo "Updating YAML manifest..."; \
	yq eval ".modules[0].sources[4].url = \"$$APPIMAGE_URL\"" -i com.browseros.BrowserOS.yml; \
	yq eval ".modules[0].sources[4].sha256 = \"$$SHA256\"" -i com.browseros.BrowserOS.yml; \
	yq eval ".modules[0].build-commands[1] = \"chmod +x $$APPIMAGE_NAME\"" -i com.browseros.BrowserOS.yml; \
	yq eval ".modules[0].build-commands[2] = \"./$$APPIMAGE_NAME --appimage-extract\"" -i com.browseros.BrowserOS.yml; \
	yq eval ".modules[0].build-commands[14] = \"rm -rf squashfs-root $$APPIMAGE_NAME\"" -i com.browseros.BrowserOS.yml; \
	echo "✓ YAML updated successfully"

.PHONY: _update-xml
_update-xml:
	@eval $$(cat .version-info | grep -v '^RELEASE_BODY='); \
	echo "Updating XML metainfo..."; \
	if xmlstarlet sel -t -v "//component/releases/release[@version='$$NEW_VERSION']" com.browseros.BrowserOS.metainfo.xml >/dev/null 2>&1; then \
		echo "Version $$NEW_VERSION already exists in metainfo.xml"; \
	else \
		BODY=$$(echo "$$RELEASE_BODY" | tr '│' '\n' | sed 's/^"//; s/"$$//'); \
		STANDARDIZED_BODY=$$(echo "$$BODY" | grep -E "^-|^##" | head -5 | sed 's/^- /• /; s/^## /• /'); \
		if [ -z "$$STANDARDIZED_BODY" ]; then \
			STANDARDIZED_BODY="• Latest stable release with bug fixes and improvements"; \
		fi; \
		ESCAPED_BODY=$$(echo "$$STANDARDIZED_BODY" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g'); \
		xmlstarlet ed -s "//component/releases" -t elem -n "temp_release" -i "//component/releases/temp_release[1]" -t attr -n "version" -v "$$NEW_VERSION" -i "//component/releases/temp_release[1]" -t attr -n "date" -v "$$RELEASE_DATE" -s "//component/releases/temp_release[1]" -t elem -n "description" -v "" -s "//component/releases/temp_release[1]/description" -t elem -n "p" -v "Stable release with enhanced features" -s "//component/releases/temp_release[1]/description" -t elem -n "ul" -v "" -r "//component/releases/temp_release[1]" -v "release" com.browseros.BrowserOS.metainfo.xml > tmp_metainfo.xml && mv tmp_metainfo.xml com.browseros.BrowserOS.metainfo.xml; \
		echo "$$ESCAPED_BODY" | while IFS= read -r line; do \
			if [ -n "$$line" ]; then \
				xmlstarlet ed -s "//component/releases/release[@version='$$NEW_VERSION']/description/ul" -t elem -n "li" -v "$$line" com.browseros.BrowserOS.metainfo.xml > tmp_metainfo.xml && mv tmp_metainfo.xml com.browseros.BrowserOS.metainfo.xml; \
			fi; \
		done; \
		$(MAKE) _manage-release-history; \
		echo "✓ XML updated successfully"; \
	fi

.PHONY: _manage-release-history
_manage-release-history:
	@echo "Managing release history (keeping last 5)..."
	@total=$$(xmlstarlet sel -t -v "count(//component/releases/release)" com.browseros.BrowserOS.metainfo.xml); \
	if [ "$$total" -gt 5 ]; then \
		remove_count=$$((total - 5)); \
		xmlstarlet ed -d "//component/releases/release[position() > 5]" com.browseros.BrowserOS.metainfo.xml > tmp_metainfo.xml && mv tmp_metainfo.xml com.browseros.BrowserOS.metainfo.xml; \
		echo "Removed $$remove_count old release entries"; \
	fi

.PHONY: _validate-updates
_validate-updates:
	@if [ -n "$(DRY_RUN)" ]; then \
		echo "DRY RUN: Skipping validation"; \
		rm -f .version-info; \
	else \
		echo "Validating updated files..."; \
		yq eval '.' com.browseros.BrowserOS.yml >/dev/null || { echo "❌ YAML validation failed"; exit 1; }; \
		xmllint --noout com.browseros.BrowserOS.metainfo.xml || { echo "❌ XML validation failed"; exit 1; }; \
		eval $$(cat .version-info | grep -v '^RELEASE_BODY='); \
		yaml_version=$$(yq eval '.modules[0].sources[4].url' com.browseros.BrowserOS.yml | grep -o 'BrowserOS_v[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+' | sed 's/BrowserOS_v//'); \
		xml_version=$$(xmlstarlet sel -t -v "//component/releases/release[1]/@version" com.browseros.BrowserOS.metainfo.xml); \
		if [ "$$yaml_version" != "$$NEW_VERSION" ] || [ "$$xml_version" != "$$NEW_VERSION" ]; then \
			echo "❌ Version mismatch: YAML=$$yaml_version, XML=$$xml_version, Expected=$$NEW_VERSION"; exit 1; \
		fi; \
		echo "✅ All validations passed"; \
		rm -f .version-info; \
	fi

# Backup files before update
.PHONY: backup-files
backup-files:
	@mkdir -p $(BACKUP_DIR)
	@cp com.browseros.BrowserOS.yml $(BACKUP_DIR)/com.browseros.BrowserOS.yml.$(TIMESTAMP)
	@cp com.browseros.BrowserOS.metainfo.xml $(BACKUP_DIR)/com.browseros.BrowserOS.metainfo.xml.$(TIMESTAMP)
	@echo "Files backed up to $(BACKUP_DIR)/"

# Restore files from backup
.PHONY: restore-files
restore-files:
	@latest_backup=$$(ls -t $(BACKUP_DIR)/com.browseros.BrowserOS.yml.* 2>/dev/null | head -1); \
	if [ -n "$$latest_backup" ]; then \
		timestamp=$$(echo "$$latest_backup" | sed 's/.*\.//'); \
		echo "Restoring from backup (timestamp: $$timestamp)..."; \
		cp $(BACKUP_DIR)/com.browseros.BrowserOS.yml.$$timestamp com.browseros.BrowserOS.yml; \
		cp $(BACKUP_DIR)/com.browseros.BrowserOS.metainfo.xml.$$timestamp com.browseros.BrowserOS.metainfo.xml; \
		echo "✓ Files restored from backup"; \
	else \
		echo "❌ No backup found"; exit 1; \
	fi

# Clean backup directory
.PHONY: clean-backups
clean-backups:
	@rm -rf $(BACKUP_DIR)
	@echo "Backup directory cleaned"

# Show current version information
.PHONY: show-version
show-version:
	@current_version=$$(yq eval '.modules[0].sources[4].url' com.browseros.BrowserOS.yml | grep -o 'v[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+' | sed 's/^v//'); \
	echo "Current BrowserOS version: $$current_version"; \
	xml_version=$$(xmlstarlet sel -t -v "//component/releases/release[1]/@version" com.browseros.BrowserOS.metainfo.xml 2>/dev/null); \
	echo "XML metainfo version: $$xml_version"
