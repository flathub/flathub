#!/usr/bin/env bash
set -euo pipefail

# Encrypt safeStorage data with the key from the Secret portal (`org.freedesktop.portal.Secret`,
# ciphertext prefix `v12`). Without this feature, Chromium only uses the portal key for decryption
# and encrypts new data with the insecure fallback key (`v10`), which the app refuses to store.
portal_features=(--enable-features=SecretPortalKeyProviderUseForEncryption)

exec /app/main/ThreemaDesktopLauncher \
  --launcher-target-bin /app/bin/zypak-wrapper.sh \
  /app/main/ThreemaDesktop \
  "${portal_features[@]}" \
  "$@"
