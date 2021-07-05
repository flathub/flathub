# This file is invoked when clicking "Hall of Fame" → "Upload Highscore".
# Highscore feature relies on external server which does not exist for many, many years.
# Since this software is proprietary, the only thing I can do is to say "I'm sorry".

gdbus call --session \
    --dest org.freedesktop.Notifications \
    --object-path /org/freedesktop/Notifications \
    --method org.freedesktop.Notifications.Notify \
    -- \
    'rollemup' \
    '0' \
    'dialog-error' \
    'Roll '\''m Up' \
    'This feature is not available anymore. Sorry.' \
    '[]' \
    '{"sound-name": <string "dialog-error">, "transient": <int32 1>}' \
    'int32 -1' \
    > /dev/null