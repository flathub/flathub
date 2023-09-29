.PHONY: image-flatpak build-flatpak clean

CONTAINER_ENGINE=podman
IMAGE_FLATPAK=lonewolf-flatpak

defeult: build-flatpak

image-flatpak:
	$(CONTAINER_ENGINE) build . --no-cache -f flatpak.Dockerfile -t $(IMAGE_FLATPAK)

build-flatpak:
	mkdir -p .flatpak
	$(CONTAINER_ENGINE) run --rm --privileged -it --memory=26g --memory-swap=0m --cpus 12 -v .:/app-dir -w /app-dir/.flatpak $(IMAGE_FLATPAK) bash -l -c "flatpak-builder --ccache --force-clean --repo=repo application /app-dir/site.someones.Lonewolf.yml"
	$(CONTAINER_ENGINE) run --rm --privileged -it -v .:/app-dir -w /app-dir/.flatpak $(IMAGE_FLATPAK) bash -l -c "flatpak build-bundle repo lonewolf.flatpak site.someones.Lonewolf --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo"

clean:
	rm -rf .flatpak
