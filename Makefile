# Requires Python packages:
# requirements-parser
# aiohttp (python-aiohttp)

.PHONY: clean
clean:
	rm -rf build dist .flatpak-builder python3-jsbeautifier.json go-sources.json yarn-sources.json

.PHONY: gen-deps
gen-deps: clean gen-js-beautify-deps gen-go-yarn-deps

.PHONY: gen-go-yarn-deps
gen-go-yarn-deps:
	flatpak-builder build crankshaft-dependencies.yml \
		--keep-build-dirs \
		--force-clean \
		--verbose
	./flatpak-builder-tools/go-get/flatpak-go-get-generator.py \
		-o go-sources.json \
		.flatpak-builder/build/crankshaft
	./flatpak-builder-tools/node/flatpak-node-generator.py \
		yarn \
		-o yarn-sources.json \
		.flatpak-builder/build/crankshaft/src/git.sr.ht/~avery/crankshaft/injected/yarn.lock

.PHONY: gen-js-beautify-deps
gen-js-beautify-deps:
	rm -rf js-beautify
	git clone https://github.com/coolavery/js-beautify.git
	cd js-beautify && git checkout bc4dd9e787ff8ee9f4709d76633d71e44c4ac4e2
	cd js-beautify && npm i
	./flatpak-builder-tools/node/flatpak-node-generator.py \
		npm \
		-o js-beautify-sources.json \
		js-beautify/package-lock.json

.PHONY: build
build:
	flatpak-builder --force-clean build space.crankshaft.Crankshaft.yml

.PHONY: install
install:
	flatpak-builder --user --install --force-clean build space.crankshaft.Crankshaft.yml

.PHONY: run
run:
	flatpak run $(ARGS) space.crankshaft.Crankshaft

.PHONY: bundle
bundle:
	flatpak-builder --repo=./repo build space.crankshaft.Crankshaft.yml --force-clean
	flatpak build-bundle ./repo crankshaft.flatpak  space.crankshaft.Crankshaft