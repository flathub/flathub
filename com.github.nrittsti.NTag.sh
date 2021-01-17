#! /bin/bash
#
#   This file is part of NTag (audio file tag editor).
#
#   NTag is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   NTag is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with NTag.  If not, see <http://www.gnu.org/licenses/>.
#
#   Copyright 2020, Nico Rittstieg
#
#

HIDPI_CONFIG="${HOME}/.config/ntag/hidpi.config"
HIDPI_SCALING=100%
if test -f "${HIDPI_CONFIG}"; then
  . "${HIDPI_CONFIG}"
fi
/app/opt/ntag/jre/bin/java -Dglass.gtk.uiScale=${HIDPI_SCALING} --module-path /app/opt/ntag/lib --add-modules=javafx.controls,javafx.fxml,javafx.swing,java.logging,jaudiotagger,java.desktop,java.json -jar /app/opt/ntag/ntag.jar "$@"
