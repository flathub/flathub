REPO_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

DOCKER_CMD := docker
RUFFLE_URL := https://github.com/ruffle-rs/ruffle.git
RUFFLE_COMMIT :=
BUILDER_TOOLS_URL := https://github.com/flatpak/flatpak-builder-tools.git
BUILDER_TOOLS_COMMIT := 338ce9c6512d49d98ae9a508d219ffe19b5144e8

install:
	flatpak run org.flatpak.Builder \
		--force-clean \
		--sandbox \
		--user \
		--install \
		--install-deps-from=flathub \
		--ccache \
		--mirror-screenshots-url=https://dl.flathub.org/media/ \
		--repo=repo \
		builddir \
		rs.ruffle.Ruffle.yaml

regenerate-sources:
	@if [[ -z "$(RUFFLE_COMMIT)" ]]; then echo "Missing RUFFLE_COMMIT"; exit 1; fi
	$(DOCKER_CMD) run --rm \
		--volume=$(REPO_DIR)/cargo-sources.json:/cargo-sources.json:rw \
		python:3.12.4 \
		bash -c '\
			mkdir -p ruffle tools && \
			(cd ruffle && \
				echo Checking out ruffle... && \
				git init -q && \
				git remote add origin "$(RUFFLE_URL)" && \
				git fetch --depth 1 origin "$(RUFFLE_COMMIT)" && \
				git checkout -q FETCH_HEAD) && \
			(cd tools && \
				echo Checking out builder tools... && \
				git init -q && \
				git remote add origin "$(BUILDER_TOOLS_URL)" && \
				git fetch --depth 1 origin "$(BUILDER_TOOLS_COMMIT)" && \
				git checkout -q FETCH_HEAD) && \
			\
		  	echo Installing dependencies... && \
			pip install toml aiohttp && \
		  	echo Regenerating sources... && \
			python3 /tools/cargo/flatpak-cargo-generator.py /ruffle/Cargo.lock -o /cargo-sources.json'

clean:
	rm -rf .flatpak-builder builddir repo

.PHONY: install clean regenerate-sources
