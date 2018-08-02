
build:
	flatpak-builder --force-clean \
		--ccache \
		--require-changes \
		--repo=repo \
		app io.github.mujx.Nheko

install-repo:
	flatpak --user remote-add --if-not-exists --no-gpg-verify local-nheko ./repo
	flatpak --user -v install --reinstall local-nheko io.github.mujx.Nheko || true

clean-cache:
	rm -rf .flatpak-builder/build
