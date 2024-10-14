#!/bin/bash
flatpak-builder build io.frappe.books.yml \
	 --install-deps-from=flathub \
	 --force-clean \
	 --user \
	 --install
