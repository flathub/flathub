.DEFAULT_GOAL := help

APP_ID    := ai.lemonade_server.Lemonade
MANIFEST  := $(APP_ID).yaml
BUILD_DIR := build-dir

FLATPAK         ?= flatpak
FLATPAK_BUILDER ?= flatpak-builder
SHELLCHECK      ?= shellcheck

# Tests run via the official bats container (https://bats-core.readthedocs.io/en/stable/docker-usage.html).
# Prefer podman, fall back to docker; override with `make test CONTAINER_ENGINE=docker`.
CONTAINER_ENGINE ?= $(shell command -v podman >/dev/null 2>&1 && echo podman || echo docker)
# Registry-qualified so podman (which enforces short-name resolution) works too.
BATS_IMAGE       ?= docker.io/bats/bats:latest
TEST_DIR         := tests/supervisor

# Shared flatpak-builder flags: --user keeps deps/install off the system remote
# (no sudo prompt); --install-deps-from=flathub pulls the runtime + SDK.
FB_FLAGS := --force-clean --user --install-deps-from=flathub

# Offline source generation: regenerate generated-{node,cargo}-sources.json with the
# flatpak-builder-tools generators run from the official uv image. uv resolves each
# generator's deps on the fly — the cargo generator from its PEP 723 inline metadata,
# the node generator from its git subdirectory — so there's no venv to bootstrap. A
# named volume persists uv's download cache across runs.
UV_IMAGE        ?= ghcr.io/astral-sh/uv:python3.14-bookworm
UV_CACHE_VOLUME ?= lemonade-flatpak-uv-cache
UV_CACHE        := /uv-cache
# flatpak-builder-tools commit pinned for reproducibility; override to bump.
FBT_REF  ?= ee65dc7a798be56de8c4c1ab73411461cac020b4
FBT_RAW  := https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/$(FBT_REF)
# \# escapes make's comment char so the URL fragment survives into the recipe.
FBT_NODE := git+https://github.com/flatpak/flatpak-builder-tools@$(FBT_REF)\#subdirectory=node
# Upstream lemonade ref to fetch lock files at: the commit pinned under the
# &lemonade-source anchor in the manifest. Override e.g. LEMONADE_REF=v10.6.0.
LEMONADE_REF ?= $(shell awk '/&lemonade-source/{f=1} f&&/commit:/{print $$2; exit}' $(MANIFEST))
LEMONADE_RAW := https://raw.githubusercontent.com/lemonade-sdk/lemonade/$(LEMONADE_REF)

# Forward goals after a run/* target as arguments to the launched command, e.g.
# `make run/lemond -- --help`. The `--` stops make's own option parsing; the
# trailing words become extra goals we capture here and turn into no-op targets
# so make doesn't try to build them.
ifeq (run,$(firstword $(subst /, ,$(MAKECMDGOALS))))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif

.PHONY: build install uninstall test lint clean \
        run run/lemond run/tray run/desktop status help \
        sources sources/node sources/cargo sources/clean

##@ Build & Quality

build: ## Build the flatpak into build-dir (no install)
	$(FLATPAK_BUILDER) $(FB_FLAGS) $(BUILD_DIR) $(MANIFEST)

install: ## Build and install the flatpak for the current user
	$(FLATPAK_BUILDER) $(FB_FLAGS) --install $(BUILD_DIR) $(MANIFEST)

uninstall: ## Remove the installed flatpak
	$(FLATPAK) uninstall --user -y $(APP_ID)

# The official bats image is minimal Alpine; the supervisor's health checks need
# curl and its mock server needs python3, so install both before running bats.
# `:z` relabels the bind mount for SELinux (a no-op on non-SELinux hosts).
test: ## Run the supervisor bats suite in the official bats container
	$(CONTAINER_ENGINE) run --rm -v "$(CURDIR):/code:z" --entrypoint sh \
	  $(BATS_IMAGE) -c 'apk add --no-cache curl python3 >/dev/null && exec bats $(TEST_DIR)'

lint: ## Shellcheck the supervisor script
	$(SHELLCHECK) lemonade-supervisor.sh

clean: ## Remove flatpak-builder artifacts (build-dir, cache, repo)
	rm -rf $(BUILD_DIR) .flatpak-builder repo

##@ Run

run: ## Run the default command (supervisor: detect/start lemond + tray + app)
	$(FLATPAK) run $(APP_ID) $(RUN_ARGS)

run/lemond: ## Run lemond (the LLM server) directly
	$(FLATPAK) run --command=lemond $(APP_ID) $(RUN_ARGS)

run/tray: ## Run the system-tray indicator directly
	$(FLATPAK) run --command=lemonade-tray $(APP_ID) $(RUN_ARGS)

run/desktop: ## Run the desktop UI (lemonade-app) directly
	$(FLATPAK) run --command=lemonade-app $(APP_ID) $(RUN_ARGS)

##@ Utilities

status: ## Print flatpak metadata, permissions, and resolved sandbox vars (debug)
	@$(FLATPAK) info $(APP_ID) >/dev/null 2>&1 || { printf '%s is not installed — run: make install\n' '$(APP_ID)'; exit 0; }
	@printf '\n\033[1m== info ==\033[0m\n'
	@$(FLATPAK) info $(APP_ID)
	@printf '\n\033[1m== permissions ==\033[0m\n'
	@$(FLATPAK) info --show-permissions $(APP_ID)
	@printf '\n\033[1m== bundled binaries (/app/bin) ==\033[0m\n'
	@$(FLATPAK) run --command=ls $(APP_ID) -1 /app/bin
	@printf '\n\033[1m== resolved sandbox env (LEMONADE/HF/XDG/FLATPAK) ==\033[0m\n'
	@$(FLATPAK) run --command=sh $(APP_ID) -c 'env | grep -Ei "^(LEMONADE|HF_|XDG_|FLATPAK)" | sort' || true
	@printf '\n\033[1m== data resolution ==\033[0m\n'
	@$(FLATPAK) run --command=lemonade-supervisor $(APP_ID) --print-data-resolution || true
	@printf '\n\033[1m== host lemond detection ==\033[0m\n'
	@$(FLATPAK) run --command=lemonade-supervisor $(APP_ID) --detect-host \
	  && printf '(external lemond reachable — flatpak would connect to it)\n' \
	  || printf '(no external lemond — flatpak would start the bundled one)\n'

##@ Maintenance

# Shared container invocation: mount the uv cache volume + the repo, point uv at the
# cached downloads. UV_LINK_MODE=copy silences hardlink warnings — the cache volume and
# uv's ephemeral install dir are on separate filesystems.
UV_RUN := $(CONTAINER_ENGINE) run --rm \
	-v $(UV_CACHE_VOLUME):$(UV_CACHE) -v "$(CURDIR):/work:z" -w /work \
	-e HOME=/tmp -e UV_CACHE_DIR=$(UV_CACHE) -e UV_LINK_MODE=copy $(UV_IMAGE)

# @ suppresses make's echo of the long podman/docker invocation; the generators still
# stream their own progress to the terminal.
sources/node: ## Regenerate generated-node-sources.json
	@$(UV_RUN) sh -c 'curl -fsSL $(LEMONADE_RAW)/src/app/package-lock.json -o /tmp/lock.json && \
	  uvx --from "$(FBT_NODE)" flatpak-node-generator npm /tmp/lock.json -o generated-node-sources.json'

sources/cargo: ## Regenerate generated-cargo-sources.json
	@$(UV_RUN) sh -c 'curl -fsSL $(LEMONADE_RAW)/src/app/src-tauri/Cargo.lock -o /tmp/Cargo.lock && \
	  uv run $(FBT_RAW)/cargo/flatpak-cargo-generator.py /tmp/Cargo.lock -o generated-cargo-sources.json'

sources: sources/node sources/cargo ## Regenerate both offline source manifests

sources/clean: ## Remove the uv download-cache volume
	-$(CONTAINER_ENGINE) volume rm $(UV_CACHE_VOLUME)

help: ## Show this help
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make \033[36m<target>\033[0m\n"} \
	  /^[a-zA-Z0-9_/-]+:.*?##/ { printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2 } \
	  /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)
