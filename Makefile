mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(abspath $(patsubst %/,%,$(dir $(mkfile_path))))

all: build

build:
	flatpak-builder --verbose --repo=tntconnect-repo --force-clean --ccache build-dir com.tntware.TntConnect.yml

package: build
	flatpak build-bundle tntconnect-repo tntconnect.flatpak com.tntware.TntConnect

run:
	flatpak-builder --verbose --run build-dir com.tntware.TntConnect.yml tntconnect.sh

sign:
	flatpak build-sign --verbose --gpg-sign=E9140597606020D3 tntconnect-repo  com.tntware.TntConnect

deltas:
	flatpak build-update-repo --generate-static-deltas --verbose --gpg-sign=E9140597606020D3  --prune tntconnect-repo

complete: package sign deltas
