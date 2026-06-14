#!/usr/bin/env bash
# Fake rustup wrapper to satisfy Cargokit inside
# network-isolated flathub build environment.

case "$1" in
  run)
    if [[ "$2" == "stable" ]]; then
      shift 2
      if [[ $# -eq 0 ]]; then
        echo "fake rustup: no command given" >&2
        exit 1
      fi
      exec "$@"
      exit 0
    fi
    ;;

  toolchain)
    if [[ "$2" == "list" ]]; then
      echo "stable (default)"
      exit 0
    fi
    ;;
    
  target)
    if [[ "$2" == "list" && "$3" == "--toolchain" && "$4" == "stable" && "$5" == "--installed" ]]; then
      echo "$(uname -m)-unknown-linux-gnu"
      exit 0
    fi
    ;;
esac

echo "fake rustup: the command:" >&2
echo "  rustup $*" >&2
echo "…is not mocked yet" >&2
exit 1
