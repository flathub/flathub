#!/bin/sh
# نستخدم "$@" لضمان تمرير رابط تسجيل الدخول من المتصفح إلى التطبيق
exec /app/opt/github-desktop/desktop --no-sandbox "$@"
