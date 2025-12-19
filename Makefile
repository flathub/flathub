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
