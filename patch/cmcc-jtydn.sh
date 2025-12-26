#!/bin/bash

mkdir -p /app/bin

CY_INS_DIR=/app/extra/chuanyun-vdi-client
CY_ADDON_PATH=resources/app.asar.unpacked/node_modules
CY_CCSDK_PATH=${CY_INS_DIR}/${CY_ADDON_PATH}/chuanyunAddOn/ccsdk/uos

export CY_BIN_PATH="${CY_CCSDK_PATH}/bin"
export LD_LIBRARY_PATH="${CY_CCSDK_PATH}/lib:$LD_LIBRARY_PATH"
export GST_PLUGIN_PATH="${CY_CCSDK_PATH}/lib"
export GST_PLUGIN_PATH_1_0="${CY_CCSDK_PATH}/lib"
export LD_LIBRARY_PATH="${CY_INS_DIR}/${CY_ADDON_PATH}/netdetectAddOn/ntsdk/lib:$LD_LIBRARY_PATH"
echo ${CY_CCSDK_PATH}
echo ${CY_BIN_PATH}
echo ${LD_LIBRARY_PATH}

# try fix log file access error
mkdir -p /var/cache/vdi_log
mkdir -p /var/cache/log
mkdir -p /var/cache/teml
ln -sf /var/cache/vdi_log /var/cache/log
mkdir -p /var/cache/ZTE/uSmartView/log
export LD_PRELOAD="/app/lib/libpath_redirect.so"

export PATH="/app/bin:$PATH" # patch/lsb_release

exec $CY_INS_DIR/cmcc-jtydn --no-sandbox "$@"