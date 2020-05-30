#!/bin/bash

set -e
shopt -s nullglob

function msg() {
  echo "flatpak-vscode-oss: $*" >&2
}

function exec_vscode() {
  exec /app/main/bin/code-oss \
       --extensions-dir="${XDG_DATA_HOME}/vscode/extensions" \
       "$@"
}

if [ -n "${FLATPAK_VSCODE_ENV}" ]; then
  msg "Environment is already set up"
  exec_vscode "$@"
fi

PYTHON_SITEDIR=$(python3 <<EOFPYTHON
import sysconfig
print(sysconfig.get_path('purelib', scheme='posix_prefix', vars={'base': '.'}))
EOFPYTHON
)

for tool_dir in /app/tools/*; do
  tool_bindir="$tool_dir/bin"
  if [ -d "$tool_bindir" ]; then
    msg "Adding $tool_bindir to PATH"
    export PATH=$PATH:$tool_bindir
  fi
  tool_pythondir="$tool_dir/$PYTHON_SITEDIR"
  if [ -d "$tool_pythondir" ]; then
    msg "Adding $tool_pythondir to PYTHONPATH"
    if [ -z "$PYTHONPATH" ]; then
      export PYTHONPATH="$tool_pythondir"
    else
      export PYTHONPATH="$PYTHONPATH:$tool_pythondir"
    fi
  fi
done

if [ "$FLATPAK_ENABLE_SDK_EXT" = "*" ]; then
  SDK=()
  for d in /usr/lib/sdk/*; do
    SDK+=("${d##*/}")
  done
else
  IFS=',' read -ra SDK <<< "$FLATPAK_ENABLE_SDK_EXT"
fi

for i in "${SDK[@]}"; do
  if [[ -d "/usr/lib/sdk/$i" ]]; then
    msg "Enabling SDK extension \"$i\""
    if [[ -f "/usr/lib/sdk/$i/enable.sh" ]]; then
      # shellcheck source=/dev/null
      . "/usr/lib/sdk/$i/enable.sh"
    else
      export PATH="$PATH:/usr/lib/sdk/$i/bin"
    fi
  else
    msg "Requested SDK extension \"$i\" is not installed"
  fi
done

msg "Setting up Node.js packages"
export NPM_CONFIG_USERCONFIG="$XDG_CONFIG_HOME/npmrc"
if [ ! -f "$NPM_CONFIG_USERCONFIG" ]; then
cat <<EOF_NPM_CONFIG > "$NPM_CONFIG_USERCONFIG"
prefix=\${XDG_DATA_HOME}/node
init-module=\${XDG_CONFIG_HOME}/npm-init.js
tmp=\${XDG_CACHE_HOME}/tmp
EOF_NPM_CONFIG
fi
export PATH="$PATH:$XDG_DATA_HOME/node/bin"

msg "Setting up Cargo packages"
export CARGO_INSTALL_ROOT="$XDG_DATA_HOME/cargo"
export CARGO_HOME="$CARGO_INSTALL_ROOT"
export PATH="$PATH:$CARGO_INSTALL_ROOT/bin"

msg "Setting up Python packages"
export PYTHONUSERBASE="$XDG_DATA_HOME/python"
export PATH="$PATH:$PYTHONUSERBASE/bin"

export FLATPAK_VSCODE_ENV=1
exec_vscode "$@"
