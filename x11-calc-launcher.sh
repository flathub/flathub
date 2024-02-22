#!/bin/sh

# Simple launcher program to set default emulator model and
# eventual run options.
# Copyright(C) 2024 macmpi
#
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.


MODEL=""
DEFAULT_MODEL=25c
OPTS=""
CMD_OPTS=""


_launch() {
[ -z "$MODEL" ] && exit 1

case $MODEL in
	10c|11c|12c|15c|16c)
		# if OPTS does not point to a rom file, set expected option to default location
		# no need to check file existence: app will error-out with proper message if missing
		[ -n "${OPTS##*.rom*}" ] || [ -z "${OPTS}" ] && OPTS="-r ${XDG_DATA_HOME}/x11-calc/${MODEL}.rom"
	;;
esac
# eventual command-line options take precedence
[ -n "$CMD_OPTS" ] && OPTS="$CMD_OPTS"

# shellcheck disable=SC2086  # intended for parameter passsing
exec /app/bin/x11-calc-${MODEL} ${OPTS}
}

_gui_conf (){
local model opts
local models="35 80 45 70 21 22 25 25c 27 29c \
	31e 32e 33e 33c 34c 37e 38e 38c 67 \
	10c 11c 12c 15c 16c"

# shellcheck disable=SC2086  # intended for listing models in dialog
model=$(zenity --list --title="Calculator selection" \
	--text="Choose preferred calculator model:" --column="HP model" $models \
	--ok-label="OK" --height=300 --width=225 2>/dev/null)

opts=$(zenity --entry --title="Expert settings: optional arguments" \
	--text="OPTS line:" --entry-text="$OPTS" \
	--ok-label="Set" --height=100 --width=300 2>/dev/null)

[ -z "$model" ] && model=$DEFAULT_MODEL
sed -i 's/^MODEL=.*/MODEL='"$model"'/' "${XDG_CONFIG_HOME}"/x11-calc/x11-calc.conf
sed -i 's/^OPTS=.*/OPTS=\"'"$opts"'\"/' "${XDG_CONFIG_HOME}"/x11-calc/x11-calc.conf
}

_setup() {
if command -v zenity >/dev/null 2>&1; then
	_gui_conf
else
	nano "${XDG_CONFIG_HOME}"/x11-calc/x11-calc.conf
fi
# reload modified settings to prep upcoming launch
# shellcheck disable=SC1090  # intended include
. "${XDG_CONFIG_HOME}"/x11-calc/x11-calc.conf
CMD_OPTS=""
}

## Main

if ! [ -f "${XDG_CONFIG_HOME}"/x11-calc/x11-calc.conf ]; then
	mkdir -p "${XDG_CONFIG_HOME}"/x11-calc
	cat <<-EOF >"${XDG_CONFIG_HOME}"/x11-calc/x11-calc.conf
		# Select which emulator to run by setting the MODEL to one
		# of the following:
		# 35, 80, 45, 70, 21, 22, 25, 25c, 27, 29c,
		# 31e, 32e, 33e, 33c, 34c, 37e, 38e, 38c, 67,
		# 10c, 11c, 12c, 15c, or 16c
		MODEL=$DEFAULT_MODEL

		# OPTS may contain options as one-liner string to specify:
		# # preferred non-default save-state file path to be loaded
		#  (like sample prg presets from /app/share/x11-calc/prg/)
		# # non-default .rom file path (-r prefix)
		# # other debug options...
		# For more complete list of options, run from command-line:
		# flatpak run io.github.mike632t.x11_calc --help
		# To test OPTS line and diagnose errors, run from command-line:
		# flatpak run io.github.mike632t.x11_calc OPTS
		OPTS=""

		# To call this setup again:
		# flatpak run io.github.mike632t.x11_calc --setup
		EOF
fi
# shellcheck disable=SC1090  # intended include
. "${XDG_CONFIG_HOME}"/x11-calc/x11-calc.conf
CMD_OPTS="$*"

[ "$CMD_OPTS" = "--setup" ] && _setup
_launch

