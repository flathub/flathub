build:
	nimble build --gc:arc

setup:
	flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
	flatpak install --user org.gnome.Sdk/x86_64/3.38 -y

install:
	flatpak-builder --user --install build-aux com.gitlab.adnan338.Invoicer.yaml -y --force-clean
