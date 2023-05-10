# -*- Mode: Makefile; indent-tabs-mode: t; tab-width: 2 -*-
#
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: Michael Terry

.PHONY: flatpak
flatpak:
	flatpak-builder --install \
	                --user \
	                --force-clean \
	                _build \
	                app.drey.MultiplicationPuzzle.yaml

.PHONY: clean
clean:
	rm -rf _build .flatpak-builder

