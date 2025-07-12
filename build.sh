#!/bin/bash

flatpak run \
	org.flatpak.Builder \
	--force-clean \
	--sandbox \
	--user \
	--install \
	--install-deps-from=flathub \
	--ccache \
	--mirror-screenshots-url=https://dl.flathub.org/media/ \
	--repo=repo \
	builddir \
	nl.flyingpie.wtq.yml
