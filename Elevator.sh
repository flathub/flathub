#!/bin/sh
export QT_QPA_PLATFORM=xcb
export QT_PLUGIN_PATH=/app/bin/7.6.45-Release.5032501
cd /app/bin/7.6.45-Release.5032501
preload_libs="./libgbm.so "

# check os_info
os_name=`cat /etc/os-release | grep ^ID= | cut -d'=' -f 2`
echo ${os_name}

# check architecture
os_machine=`uname -m`

libc_version=`ldd --version | grep ldd | cut -d' ' -f5`
libc_version_num=`echo "${libc_version}" | tr '.' ' '`
libc_version_m=0
libc_version_b=0
libc_lower=false
libc_lower_29=false # for cef109

is_enable_cef109=true
if [ "${is_enable_cef109}" = "true" ]; then
    if [ "$os_machine" = "aarch64" ]; then
        if [ "${libc_lower_29}" = "true" ]; then
            preload_libs="${preload_libs} ./libm-2.31.so "
        fi
    fi
    preload_libs="${preload_libs} ./plugins/dtwebview/libcef.so "
else
if [ "$os_machine" = "mips64" ]; then
    echo mips64el branch
    preload_libs="${preload_libs} ./plugins/dtwebview/libcef.so "
fi
fi

# preload_libs
echo preload_libs=${preload_libs}
if [ ! -z "${preload_libs}" ]; then
    LD_PRELOAD="${preload_libs}" ./com.alibabainc.dingtalk $1
else
    ./com.alibabainc.dingtalk $1
fi
