#!/bin/sh
export QT_PLUGIN_PATH=/app/extra/dingtalk:$QT_PLUGIN_PATH
cd /app/extra/dingtalk || exit 1
preload_libs="./libgbm.so ./plugins/dtwebview/libcef.so "

LD_PRELOAD="${preload_libs}" ./com.alibabainc.dingtalk "$@"