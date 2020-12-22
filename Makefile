build:
	cargo build --release

install:
	install -D target/release/nixwriter /usr/bin/nixwriter
	install -Dm644 com.gitlab.adnan338.Nixwriter.desktop /usr/share/applications/com.gitlab.adnan338.Nixwriter.desktop
	install -Dm644 com.gitlab.adnan338.Nixwriter.svg /usr/share/icons/hicolor/scalable/apps/com.github.adnan338.Nixwriter.svg

uninstall:
	rm /usr/bin/nixwriter
	rm /usr/share/applications/com.gitlab.adnan338.Nixwriter.desktop
	rm /usr/share/icons/hicolor/scalable/apps/com.github.adnan338.Nixwriter.svg

flatpak-initiate:
	flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
	flatpak install --user org.gnome.Sdk/x86_64/3.38 -y
	flatpak install --user org.freedesktop.Sdk.Extension.rust-stable/x86_64/19.08 -y

flatpak-build: 
	flatpak-builder build-aux flatpak/com.gitlab.adnan338.Nixwriter.yaml

flatpak-install:
	rm -fr .flatpak-builder/ build-aux/
	make flatpak-initiate
	python flatpak-cargo-generator.py -o deptree.json -d Cargo.lock
	flatpak-builder --user --install build-aux com.gitlab.adnan338.Nixwriter.yaml -y

flatpak-run:
	flatpak run com.gitlab.adnan338.Nixwriter

flatpak-uninstall:
	flatpak remove com.gitlab.adnan338.Nixwriter

clean:
	rm -fr .flatpak-builder/ build-aux/
