#!/bin/bash

NAME=com.github.hneemann.digital
ARCH=x86_64

buildonly=0
while getopts ba: opt 2>/dev/null 1>&2
do
case ${opt} in
    "b") buildonly=1;;
    "a") ARCH=${OPTARG};;
    ?) echo "wrong parameters"; exit 0;;
esac
done

if [ $# -ge 1 ]; then
ARCH=${1}
echo registering qemu-user-static
sudo podman run --rm --privileged multiarch/qemu-user-static --reset -p yes
fi

echo Building ${NAME} for architecture: ${ARCH}

RELEASE=$(cat ${NAME}.json | grep "branch" | awk -F '\"' '{print $4}')

#git submodule init
#git submodule update
#git submodule update --remote --merge

# build 
COMMAND="flatpak-builder --arch=${ARCH} --repo=repo --force-clean build_${ARCH} ${NAME}.json"

echo running: ${COMMAND}
${COMMAND}
RC=$?

# pack everything up in a distributable file
if [ ${RC} -eq 0 ] && [ ${buildonly} -ne 1 ]; then
	COMMAND="flatpak build-bundle --arch=${ARCH} repo ${NAME}_${ARCH}.flatpak ${NAME} ${RELEASE}"
	echo running: ${COMMAND}
	${COMMAND}
fi

