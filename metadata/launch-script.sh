#!/bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Flatpak launch script for Midori Browser

export MOZ_APP_LAUNCHER="${FLATPAK_ID}"
export MOZ_LEGACY_PROFILES=1
export MOZ_ALLOW_DOWNGRADE=1

# Set Wayland display if available
if [ "$XDG_SESSION_TYPE" = "wayland" ]; then
  export MOZ_ENABLE_WAYLAND=1
fi

exec /app/midori/midori "$@"
