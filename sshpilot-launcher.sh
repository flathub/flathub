#!/usr/bin/env bash
export SSHPILOT_FLATPAK=1
exec python3 -m sshpilot.main "$@"