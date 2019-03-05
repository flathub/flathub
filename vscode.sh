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
unity_pid=$(pidof Unity)
unity_port=$((56000 + unity_pid % 1000))

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

echo $target_pid | tee /tmp/vsc-unity.log
ps -A | tee -a /tmp/vsc-unity.log

code "$@"
while ps -A | grep -q code; do sleep 5; done
kill $(jobs -p)
EOF

for ref in com.visualstudio.code.oss com.visualstudio.code; do
  if $flatpak info -m $ref >/dev/null 2>&1; then
    socat TCP-LISTEN:$target_port,fork,reuseaddr TCP:localhost:$unity_port &
    $flatpak run --command=bash $ref -c "$start_vscode" -- "$target_pid" "$@"
    kill $(jobs -p)
    exit $?
  fi
done

zenity --error --no-wrap --title='Visual Studio Code is not installed' \
  --text="Visual Studio Code is required to edit scripts with the Unity Hub Flatpak, please \
install it from Flathub."
