#
# Makefile - Listing quick commands that help local development/testing.
#
build:
	flatpak-builder --repo=.localrepo --force-clean build com.github.uwlabs.BloomRPC.yaml

install:
	flatpak --user remote-delete local; \
	flatpak --user remote-add --no-gpg-verify local .localrepo; \
	flatpak --user install local com.github.uwlabs.BloomRPC; \

run:
	flatpak run com.github.uwlabs.BloomRPC

.PHONY: build install run

