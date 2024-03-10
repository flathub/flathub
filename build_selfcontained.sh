#!/usr/bin/env bash

# Simple bash script to easily build on Linux to verify functionality.
# Uses the same commands as those found in the PKGBUILD for the AUR
# package, modified to build a self-contained version.

output="bin"
config="Release"
runtime="linux-x64" # Example runtime identifier
selfContained="true" # Enable self-contained build
options=()

while [ $# -gt 0 ]; do
  case "$1" in
    -o=*|--output=*)
      output="${1#*=}"
      ;;
    -o|--output)
      output="$2"
      shift
      ;;
    -c=*|--configuration=*)
      config="${1#*=}"
      ;;
    -c|--configuration)
      config="$2"
      shift
      ;;
    -r=*|--runtime=*)
      runtime="${1#*=}"
      ;;
    -r|--runtime)
      runtime="$2"
      shift
      ;;
    -s|--self-contained)
      selfContained="true"
      ;;
    *)
      options+=("$1")
      ;;
  esac
  shift
done

# Provide defaults, then pass everything else as-is
# Added -r (runtime) and --self-contained options for a self-contained deployment.
. "$(dirname ${BASH_SOURCE[0]})"/eng/linux/package.sh -o "${output}" -c "${config}" -r "${runtime}" --self-contained="${selfContained}" "${options[@]}"

