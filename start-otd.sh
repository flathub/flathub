#!/bin/bash

# Check if OpenTabletDriver.Daemon is running
if ! ps ax | grep -v grep | grep "OpenTabletDriver.Daemon" > /dev/null
then
    echo "OpenTabletDriver.Daemon is not running, starting it now..."
    # Start Daemon from the current directory
    /app/bin/OpenTabletDriver.Daemon &
else
    echo "OpenTabletDriver.Daemon is already running."
fi

# Start OpenTabletDriver.UX.Gtk with exec, replacing the current shell
echo "Starting OpenTabletDriver.UX.Gtk with exec..."
exec /app/bin/OpenTabletDriver.UX.Gtk

