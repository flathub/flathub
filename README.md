# flathub
Pull requests for new applications to be added
# Simple to build
$ flatpak-builder --repo=repo --force-clean build-dir uk.co.wagemaker.SmallTextPad.json
$ flatpak build-bundle repo smalltextpad-1.3.1-include-jdk.flatpak uk.co.wagemaker.SmallTextPad
$ flatpak install smalltextpad-1.3.1-include-jdk.flatpak
$ flatpak run uk.co.wagemaker.SmallTextPad
