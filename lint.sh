#!/bin/sh
# Open Chakra Toning
# Copyright (C) 2024  Nicco Kunzmann
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

#
# Lint the flatpak
#

set -e
cd "`dirname \"$0\"`"

id="io.github.niccokunzmann.python_dhcp_server"
manifest="$id.yml"
repo=".repo"

echo "Linter 1"
## see https://docs.flathub.org/docs/for-app-authors/submission/#before-submission
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest "$manifest"
echo "Linter 2"
errors=".flatpak-lint.error.txt"
flatpak run --command=flatpak-builder-lint org.flatpak.Builder --exceptions "repo" "$repo" | tee "$errors"
if [ "`cat \"$errors\"`" != "`cat flatpak-lint.expected-error.txt`" ]; then
  echo "Linter error!"
  exit 1
else
  echo "OK! Exceptions are known!"
fi
