
build:
	flatpak-builder --force-clean \
		--ccache \
		--require-changes \
		--repo=repo \
		app com.github.Nheko.json

install-repo:
	flatpak --user remote-add --if-not-exists --no-gpg-verify local-nheko ./repo
	flatpak --user -v install --reinstall local-nheko com.github.Nheko//v0.5.2 || true

clean-cache:
	rm -rf .flatpak-builder/build
