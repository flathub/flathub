#!/bin/sh -e
set -o errexit

###
# Copyright (c) 2015-2018, Antoine Le Gonidec
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# This software is provided by the copyright holders and contributors "as is"
# and any express or implied warranties, including, but not limited to, the
# implied warranties of merchantability and fitness for a particular purpose
# are disclaimed. In no event shall the copyright holder or contributors be
# liable for any direct, indirect, incidental, special, exemplary, or
# consequential damages (including, but not limited to, procurement of
# substitute goods or services; loss of use, data, or profits; or business
# interruption) however caused and on any theory of liability, whether in
# contract, strict liability, or tort (including negligence or otherwise)
# arising in any way out of the use of this software, even if advised of the
# possibility of such damage.
###

###
# Little Big Adventure 2
# build native Linux packages from the original installers
# send your bug reports to vv221@dotslashplay.it
###

script_version=20180224.1

# Set game-specific variables

GAME_ID='little-big-adventure-2'
GAME_NAME='Little Big Adventure 2'

ARCHIVES_LIST='ARCHIVE_GOG'

ARCHIVE_GOG='setup_lba2_2.1.0.8.exe'
ARCHIVE_GOG_URL='https://www.gog.com/game/little_big_adventure_2'
ARCHIVE_GOG_MD5='9909163b7285bd37417f6d3c1ccfa3ee'
ARCHIVE_GOG_SIZE='750000'
ARCHIVE_GOG_VERSION='1.0-gog12.1.0.8'

CONFIG_FILES='./lba2.cfg'

ARCHIVE_DOC_DATA_PATH='app'
ARCHIVE_DOC_DATA_FILES='./*.pdf ./*.txt'

ARCHIVE_GAME_BIN_PATH='app'
ARCHIVE_GAME_BIN_FILES='./dos4gw.exe ./language.exe ./install.ini ./lba2.exe ./setup.exe'

ARCHIVE_GAME_DATA_PATH='app'
ARCHIVE_GAME_DATA_FILES='./*.hqr ./*.ile ./*.obl ./drivers ./lba2.cfg ./lba2.dat ./lba2.dos ./lba2.gog ./lba2.ogg ./save ./setsound.bat ./vox'

GAME_IMAGE='lba2.dat'

APP_MAIN_TYPE='dosbox'
APP_MAIN_EXE='lba2.exe'
APP_MAIN_ICON='lba2.exe'
APP_MAIN_ICON_RES='16 32 48'

PACKAGES_LIST='PKG_BIN PKG_DATA'

PKG_DATA_ID="${GAME_ID}-data"
PKG_DATA_DESCRIPTION='data'

PKG_BIN_ARCH='32'
PKG_BIN_DEPS="$PKG_DATA_ID dosbox"

# Load common functions

target_version='2.5'

if [ -z "$PLAYIT_LIB2" ]; then
	IFS=:
	for path in \
		'.' \
		"${XDG_DATA_HOME:=$HOME/.local/share}" \
		${XDG_DATA_DIRS:-/usr/local/share:/usr/share}
	do
		for path in "$path" "$path/games"; do
			path="$path/play.it/libplayit2.sh"
			if [ -e "$path" ]; then
				PLAYIT_LIB2="$path"
				break 2
			fi
		done
	done
	unset IFS


	if [ -z "$PLAYIT_LIB2" ]; then
		printf '\n\033[1;31mError:\033[0m\n'
		printf 'libplayit2.sh not found.\n'
		exit 1
	fi
fi
. "$PLAYIT_LIB2"

# Extract game data

extract_data_from "$SOURCE_ARCHIVE"

for PKG in $PACKAGES_LIST; do
	organize_data "DOC_${PKG#PKG_}"    "$PATH_DOC"
	organize_data "GAME_${PKG#PKG_}"   "$PATH_GAME"
done

PKG='PKG_BIN'
extract_and_sort_icons_from 'APP_MAIN'

rm --recursive "$PLAYIT_WORKDIR/gamedata"

# Write launchers

PKG='PKG_BIN'
write_launcher 'APP_MAIN'

# Build package

write_metadata
build_pkg

# Clean up

rm --recursive "$PLAYIT_WORKDIR"

# Print instructions

print_instructions

exit 0
