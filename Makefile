help:	## Show all Makefile targets.
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[33m%-30s\033[0m %s\n", $$1, $$2}'

build: io.github.bruttazz.FirefoxDevFlatpak.yml ## build and install the application for testing
	@flatpak-builder --force-clean --user --install .build io.github.bruttazz.FirefoxDevFlatpak.yml

run: io.github.bruttazz.FirefoxDevFlatpak.yml build  ## run the application after build
	@flatpak run io.github.bruttazz.FirefoxDevFlatpak

remove:   ## remove application from system
	@flatpak uninstall io.github.bruttazz.FirefoxDevFlatpak

bundle:		build ## create bundle for the applicatoin
	@flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --ccache --repo=repo .build io.github.bruttazz.FirefoxDevFlatpak.yml
	@echo -e "\nGenerating Bundle .."
	@flatpak build-bundle repo firefoxDev.flatpak io.github.bruttazz.FirefoxDevFlatpak --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo