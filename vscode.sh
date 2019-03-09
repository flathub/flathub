#!/usr/bin/bash

flatpak='flatpak-spawn --host flatpak'

# In order for debugging to work, the the Unity VS Code extension needs to be able to find
# Unity Editor's PID. Furthermore, it should be relatively unique to avoid conflicts.

# The socket to connect to is found by taking the Unity PID modulo 100 and adding 56000:
# https://github.com/Unity-Technologies/MonoDevelop.Debugger.Soft.Unity/blob/usedForVSCodeRelease/UnityProcessDiscovery.cs#L93

# The "solution" is to grab a random number and use that as the target PID, then spawn a ton of
# processes before our Unity fake shell script to ensure the PID is what we want.

# 4 comes because bash will start as PID 2 inside the sandbox, and the cp will be PID
#. Therfore, we need the minimum target PID to be 4.

# An socat is then set up on the Unity end to connect the socket our PID exposes and the socket
# that Unity Editor is on.

target_pid=$((4 + RANDOM % 100))
target_port=$((56000 + target_pid))
unity_pid=$PPID
unity_port=$((56000 + unity_pid % 1000))

not_installed() {
  local appid="$1"
  local title="$2"
  local text="$3"
  local runtime_branch="$4"
  local on_flathub_web="$5"

  local has_software
  gdbus introspect -e -d org.gnome.Software -o /org/gnome/Software >/dev/null 2>&1 && \
    has_software=1

  if [[ -n "$has_software" || -n "$on_flathub_web" ]]; then
    if zenity --question --no-wrap --title="$title" \
              --text=$"$text\nWould you like to install it?"; then
      if [[ -n "$has_software" ]]; then
        gdbus call -e -d org.gnome.Software -o /org/gnome/Software -m org.gtk.Actions.Activate \
          search "[<'$appid'>, <'$runtime_branch'>]" '[]'
      else
        xdg-open https://flathub.org/apps/search/$appid
      fi
    fi
  else
    zenity --warning --no-wrap --title="$title" \
      --text=$"$text\nPlease install it from Flathub."
  fi
}

read -r -d '' start_vscode <<'EOF'
target_pid="$1"
shift

cp /usr/bin/sleep /tmp/Unity

for (( i=4; i < $target_pid; i++ )); do /usr/bin/true; done
/tmp/Unity infinity &

if ! [[ -d /usr/lib/sdk/dotnet || -d /usr/lib/sdk/mono5 ]]; then
  zenity --warning --no-wrap --title='dotnet and Mono SDKs are required' \
    --text="The dotnet and mono SDK extensions are required to use the Unity debugger integration,
please install them from Flathub."
fi

[[ -d /usr/lib/sdk/dotnet ]] && . /usr/lib/sdk/dotnet/enable.sh
[[ -d /usr/lib/sdk/mono5 ]] && . /usr/lib/sdk/mono5/use.sh

# Note: don't do grep -q, code --list-extensions doesn't like SIGPIPE
if code --list-extensions | grep ms-vscode.csharp >/dev/null &&
  ! grep -qs '"omnisharp\.useGlobalMono"\s*:\s*"never"' $XDG_CONFIG_HOME/Code/User/settings.json; then
  zenity --warning --no-wrap --title='omnisharp.useGlobalMono should be "never"' \
    --text="omnisharp.useGlobalMono should be set to \"never\" to avoid errors when started
from within Unity Editor."
fi

code "$@"
while ps -A | grep -q code; do sleep 5; done
kill $(jobs -p)
EOF

for ref in com.visualstudio.code.oss com.visualstudio.code; do
  sdk_arch_branch=$($flatpak info --show-sdk $ref 2>/dev/null | sed 's|^[^/]*/||')
  if [[ -n "$sdk_arch_branch" ]]; then
    dotnet=$($flatpak info org.freedesktop.Sdk.Extension.dotnet/$sdk_arch_branch 2>/dev/null)
    mono=$($flatpak info org.freedesktop.Sdk.Extension.mono5/$sdk_arch_branch 2>/dev/null)

    if [[ -z "$dotnet" || -z "$mono" ]]; then
      if [[ -z "$dotnet" && -z "$mono" ]]; then
        ref=org.freedesktop.Sdk.Extension
      elif [[ -z "$dotnet" ]]; then
        ref=org.freedesktop.Sdk.Extension.dotnet
      elif [[ -z "$mono" ]]; then
        ref=org.freedesktop.Sdk.Extension.mono5
      fi

      not_installed $ref 'dotnet and mono5 SDK extensions are required' \
        'The dotnet and mono5 SDK extensions are required for the Unity debugger to work.' \
        $(echo "$sdk_arch_branch" | cut -d/ -f2) ''
    fi

    socat TCP-LISTEN:$target_port,fork,reuseaddr TCP:localhost:$unity_port &
    $flatpak run --command=bash $ref -c "$start_vscode" -- "$target_pid" "$@"
    kill $(jobs -p)
    exit $?
  fi
done

not_installed com.visualstudio.code 'Visual Studio Code is required' \
  'Visual Studio Code is required to edit Unity scripts.' '' 1
