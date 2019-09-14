#!/bin/bash

flatpak-builder --repo=testing-repo --force-clean build-dir com.etlegacy.ETL.yaml
flatpak --user remote-add --if-not-exists --no-gpg-verify etl-testing-repo testing-repo
flatpak --user install etl-testing-repo com.etlegacy.ETL -y
flatpak update -y
