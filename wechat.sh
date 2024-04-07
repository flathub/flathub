#!/bin/bash

get_wechat_notifier_item() {
  local notifier_items=$(
    gdbus call --session \
      --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus \
      --method org.freedesktop.DBus.ListNames |
      grep -oE 'org.kde.StatusNotifierItem-[0-9]{1,}-[0-9]'
  )

  local notifier_item
  for notifier_item in $notifier_items; do
    local notifier_id=$(
      gdbus call --session \
        --dest="${notifier_item/\// \/}" --object-path /StatusNotifierItem \
        --method org.freedesktop.DBus.Properties.Get org.kde.StatusNotifierItem Id
    )

    if [[ $notifier_id =~ "wechat" ]]; then
      echo "${notifier_item/\// \/}"
    fi
  done
}

try_open_wechat_window() {
  local notifier_item=$(get_wechat_notifier_item)

  if [ -n "$notifier_item" ]; then
    gdbus call --session \
      --dest="$notifier_item" --object-path /StatusNotifierItem \
      --method org.kde.StatusNotifierItem.Activate 0 0 >/dev/null
  fi
}

try_exit_wechat() {
  local notifier_item=$(get_wechat_notifier_item)

  if [ -n "$notifier_item" ]; then
    gdbus call --session \
      --dest="$notifier_item" --object-path /MenuBar \
      --method com.canonical.dbusmenu.Event 1 clicked '<"">' 0 >/dev/null
  fi
}

setup_ime_env() {
  if [[ "$XMODIFIERS" =~ fcitx ]]; then
    [ -z "$QT_IM_MODULE" ] && export QT_IM_MODULE=fcitx
    [ -z "$GTK_IM_MODULE" ] && export GTK_IM_MODULE=fcitx
  elif [[ "$XMODIFIERS" =~ ibus ]]; then
    [ -z "$QT_IM_MODULE" ] && export QT_IM_MODULE=ibus
    [ -z "$GTK_IM_MODULE" ] && export GTK_IM_MODULE=ibus
  fi
}

if [ "$1" == "--exit-wechat" ]; then
  try_exit_wechat
  exit
fi

try_open_wechat_window
setup_ime_env

export LD_PRELOAD=/app/lib/libredirect.so
exec /app/extra/wechat/wechat "$@"
