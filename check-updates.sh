#!/bin/bash

# Uncomment for debugging use
# set -o xtrace
set -o errexit
set -o pipefail
set -o nounset

# Dependencies:
# - curl
# - dpkg-deb

PREV_ETAG="08ace90add2c53e2ed98a1d7aea7c6d4-13"
PREV_RELEASE="1.0.9105465"
DEB_URL="https://www.guilded.gg/downloads/Guilded-Linux.deb"
DEB_FILE=$(basename ${DEB_URL})

function check_deb() {
	LATEST_ETAG=$(curl --head --silent ${DEB_URL} | \
		sed --quiet --regexp-extended 's/etag: \"(.*)\"/\1/p' | \
		tr -d '\r')

	if [[ "${LATEST_ETAG}" != "${PREV_ETAG}" ]]; then
		echo "Etag has changed."
		echo "${LATEST_ETAG} > ${PREV_ETAG}."

		curl --silent --remote-name ${DEB_URL}
		LATEST_RELEASE=$(dpkg-deb --field "${DEB_FILE}" Version | sed 's/-master//')

		if [[ "${LATEST_RELEASE}" != "${PREV_RELEASE}" ]]; then
			echo "Deb version has changed."
			echo "${LATEST_RELEASE} > ${PREV_RELEASE}."
		else
			echo "Deb version has not changed."
			echo "${LATEST_RELEASE} == ${PREV_RELEASE}."
		fi
	else
		echo "Etag has not changed."
		echo "${LATEST_ETAG} == ${PREV_ETAG}."
	fi

	rm -f Guilded-Linux.deb
}

function check_appimage() {
	# Example download URL:
	# https://s3-us-west-2.amazonaws.com/www.guilded.gg/AppBuilds/linux/Guilded-1.0.9105465-master.AppImage
	APP_UPDATE="https://s3-us-west-2.amazonaws.com/www.guilded.gg/AppBuilds/linux/master-linux.yml"

	yaml() {
		python3 -c "import yaml;print(yaml.safe_load(open('$1'))$2)"
	}
	
	curl --silent --remote-name ${APP_UPDATE}
	LATEST_RELEASE=$(yaml master-linux.yml "['version']" | sed 's/-master//')

	if [[ "${LATEST_RELEASE}" != "${PREV_RELEASE}" ]]; then
		echo "AppImage version has changed."
		echo "${LATEST_RELEASE} > ${PREV_RELEASE}."
	else
		echo "AppImage version has not changed."
		echo "${LATEST_RELEASE} == ${PREV_RELEASE}."
	fi

	rm -f master-linux.yml
}

case "$@" in
	deb)
		check_deb
		;;

	appimage)
		check_appimage
		;;

	*)
		echo "Usage: $0 {deb|appimage}"
		;;
esac
