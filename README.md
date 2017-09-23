NixView Flatpak
-------------

Install the runtimes

	flatpak remote-add --from gnome https://sdk.gnome.org/gnome.flatpakrepo
	flatpak install gnome org.freedesktop.Platform//1.6 org.freedesktop.Sdk//1.6


Local building

	flatpak-builder NixView org.gnode.NixView.json
	flatpak build-export repo NixView


Add local remote

	flatpak remote-add --user --no-gpg-verify nixview-repo repo
	flatpak install --user nixview-repo org.gnode.NixView

Run the Flatpak

	flatpak run org.gnode.NixView
