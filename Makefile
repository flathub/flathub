OS := $(shell uname)
.PHONY: install gen flatpak clean

install:
	flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
	flatpak install flathub -y org.flatpak.Builder org.gnome.Platform//42 \
		org.gnome.Sdk//42 \
		runtime/org.freedesktop.Sdk.Extension.rust-stable/x86_64/21.08 \
		runtime/org.freedesktop.Sdk.Extension.node16/x86_64/21.08 \
		org.freedesktop.Sdk.Extension.rust-nightly/x86_64/21.08
	wget -N https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/cargo/flatpak-cargo-generator.py
	pip install aiohttp toml
	pip install -e "git+https://github.com/flatpak/flatpak-builder-tools.git#egg=flatpak_node_generator&subdirectory=node"

gen:
	python flatpak-cargo-generator.py -o cargo-sources.json ../EmojiMart/src-tauri/Cargo.lock
	flatpak-node-generator --no-requests-cache -r -o node-sources.json npm ../EmojiMart/package-lock.json

# Gen from Yarn not working: flatpak-node-generator --no-requests-cache -r -o node-sources.json yarn ../EmojiMart/yarn.lock

flatpak:
	flatpak-builder --user --install --force-clean build io.github.vemonet.EmojiMart.yml
	flatpak run io.github.vemonet.EmojiMart

# flatpak build-bundle _repo io.github.vemonet.EmojiMart.flatpak io.github.vemonet.EmojiMart

clean:
	rm -r .flatpak-builder build/
	flatpak remove io.github.vemonet.EmojiMart -y --delete-data

# rm -rf ../EmojiMart/src-tauri/target