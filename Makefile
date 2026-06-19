REPO_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

DOCKER_CMD := docker
DELTACHAT_URL := https://github.com/chatmail/core.git
DELTACHAT_COMMIT := $(shell jq -r '.modules[] | select(.name == "deltachat-rpc-server") | .sources[0].commit' io.github.trufae.Parla.json)
BUILDER_TOOLS_URL := https://github.com/flatpak/flatpak-builder-tools.git
BUILDER_TOOLS_COMMIT := 737c0085912f9f7dabf9341d4608e2a77a51a73a

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
		io.github.trufae.Parla.json

regenerate-sources:
	@if [ -z "$(DELTACHAT_COMMIT)" ]; then echo "Missing DELTACHAT_COMMIT"; exit 1; fi
	$(DOCKER_CMD) run --rm \
		--volume=$(REPO_DIR)/cargo-sources-deltachat.json:/cargo-sources-deltachat.json:rw \
		docker.io/library/python:3.12.4 \
		bash -c '\
			mkdir -p deltachat tools && \
			(cd deltachat && \
				echo Checking out deltachat... && \
				git init -q && \
				git remote add origin "$(DELTACHAT_URL)" && \
				git fetch --depth 1 origin "$(DELTACHAT_COMMIT)" && \
				git checkout -q FETCH_HEAD) && \
			(cd tools && \
				echo Checking out builder tools... && \
				git init -q && \
				git remote add origin "$(BUILDER_TOOLS_URL)" && \
				git fetch --depth 1 origin "$(BUILDER_TOOLS_COMMIT)" && \
				git checkout -q FETCH_HEAD) && \
			\
			echo Installing dependencies... && \
			pip install tomlkit aiohttp && \
			echo Regenerating sources... && \
			python3 /tools/cargo/flatpak-cargo-generator.py /deltachat/Cargo.lock -o /cargo-sources-deltachat.json'

clean:
	rm -rf .flatpak-builder builddir repo

.PHONY: install clean regenerate-sources
