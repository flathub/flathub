#!/bin/sh
# Flatpak 진입점 — /app/bin/lorem.
#
# Electron(Chromium)은 Flatpak 안에서 제 샌드박스를 세우지 못한다(setuid·네임스페이스가 막혀 있다).
# zypak이 Chromium의 샌드박스 요청을 Flatpak 샌드박스(flatpak-spawn)로 바꿔 준다 — Electron BaseApp이 싣는다.
#
# CHROME_DESKTOP: Wayland app_id·창 묶음이 .desktop 이름(kr.anzi.Lorem)과 맞아야 독·작업 표시줄이 이 앱의 아이콘을 쓴다.
# TMPDIR: Electron의 단일 인스턴스 잠금 소켓이 샌드박스마다 갈리는 /tmp가 아니라 같은 앱 인스턴스끼리 보이는 자리에 서야
#         두 번째 실행(lorem:// 링크 등)이 첫 창으로 넘어간다.
export CHROME_DESKTOP=kr.anzi.Lorem.desktop
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
exec zypak-wrapper /app/lorem/lorem "$@"
