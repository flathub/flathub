.PHONY: build run repo install

build:
	flatpak-builder --sandbox --force-clean build org.clawsmail.Claws-Mail.json | tee build.log

run: build
	flatpak-builder --run build org.clawsmail.Claws-Mail.json claws-mail

repo: build
	flatpak-builder --sandbox --force-clean --repo=repo build-dir org.flatpak.Hello.json

install: build
	flatpak --user install ./repo org.clawsmail.Claws-Mail

uninstall:
	flatpak --user uninstall org.clawsmail.Claws-Mail
