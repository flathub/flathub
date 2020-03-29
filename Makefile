#
# Makefile - Listing quick commands that help local development/testing.
#
# System requirements:
# - flatpak
# - flatpak-builder
# - flatpak-node-generator
# - wget
#
build:
	flatpak-builder --repo=.localrepo --force-clean build com.github.uwlabs.BloomRPC.yaml

install:
	flatpak --user remote-delete local; \
	flatpak --user remote-add --no-gpg-verify local .localrepo; \
	flatpak --user install local com.github.uwlabs.BloomRPC; \

run:
	flatpak run com.github.uwlabs.BloomRPC

generate:
	wget https://raw.githubusercontent.com/uw-labs/bloomrpc/1.4.1/yarn.lock; \
	flatpak-node-generator yarn yarn.lock; \
	rm yarn.lock*; \

.PHONY: build install run generate

