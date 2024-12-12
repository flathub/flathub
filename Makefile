build:
	flatpak run --command=flatpak-builder org.flatpak.Builder --user --force-clean build-dir io.github.heathcliff26.go-minesweeper.yaml

install:
	flatpak run --command=flatpak-builder org.flatpak.Builder --user --install --force-clean build-dir io.github.heathcliff26.go-minesweeper.yaml

uninstall:
	flatpak uninstall --user -y io.github.heathcliff26.go-minesweeper

run:
	flatpak run --user io.github.heathcliff26.go-minesweeper

lint:
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.heathcliff26.go-minesweeper.yaml

clean:
	rm -rf .flatpak-builder build-dir
