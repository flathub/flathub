#!/bin/sh
echo -ne '\033c\033]0;undina\a'
base_path="$(dirname "$(realpath "$0")")"
"$base_path/undina.x86_64" "$@"
