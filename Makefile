FLATPAK_BUILDER = flatpak-builder
BUILDER_OPTIONS = --force-clean --ccache --require-changes
TARGET_REPO = repo
APP_ID = so.notion.Notion
MANIFEST := $(APP_ID).yaml
COMMAND = notion
NOTION_VERSION = 2.0.11

help: ## Show help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

notion.exe:
	@curl -L https://desktop-release.notion-static.com/Notion%20Setup%20$(NOTION_VERSION).exe -o $@

generated-sources.json: notion.exe
	@./generate.sh

build: generated-sources.json ## Build Flatpak
	@$(FLATPAK_BUILDER) $(BUILDER_OPTIONS) --repo=$(TARGET_REPO) build-dir $(MANIFEST)

run: ## Run Flatpak directly
	@$(FLATPAK_BUILDER) --run build-dir $(MANIFEST) $(COMMAND)

install: ## Install Flatpak locally
	@$(FLATPAK_BUILDER) --install --user --force-clean build-dir $(MANIFEST)

clean: ## Cleanup
	@rm -rf build-dir

.PHONY: help build run install clean
