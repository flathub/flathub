#!/bin/bash
if [[ "$XMODIFIERS" =~ fcitx ]]; then
    [ -z "$QT_IM_MODULE" ] && export QT_IM_MODULE=fcitx
    [ -z "$GTK_IM_MODULE" ] && export GTK_IM_MODULE=fcitx
elif [[ "$XMODIFIERS" =~ ibus ]]; then
    [ -z "$QT_IM_MODULE" ] && export QT_IM_MODULE=ibus
    [ -z "$GTK_IM_MODULE" ] && export GTK_IM_MODULE=ibus
fi
export QT_QPA_PLATFORM=xcb
export QT_PLUGIN_PATH=/app/extra/dingtalk:$QT_PLUGIN_PATH
cd /app/extra/dingtalk || exit 1
./com.alibabainc.dingtalk "$@"