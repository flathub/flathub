#!/bin/sh

export QT_VULKAN_LIB=/usr/lib/x86_64-linux-gnu/libvulkan.so.1
exec /app/mupen64plus/mupen64plus-gui "$@"
