#!/bin/bash
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak-builder build io.frappe.books.yml \
	 --install-deps-from=flathub \
	 --force-clean \
	 --user \
	 --install
