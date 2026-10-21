#!/bin/sh
# file_picker 8.x uses `which` to locate Zenity. Keep this helper in /app.
result_code=0
for executable in "$@"; do
  if command -v "$executable" >/dev/null 2>&1; then
    command -v "$executable"
  else
    result_code=1
  fi
done
exit "$result_code"
