#!/usr/bin/sh

echo "Setting up RPC..."

for i in {0..9}; do
    test -S $XDG_RUNTIME_DIR/discord-ipc-$i || ln -sf {app/com.discordapp.Discord,$XDG_RUNTIME_DIR}/discord-ipc-$i;
done

echo "RPC set up! Launching game."

exec SS14.Launcher $@