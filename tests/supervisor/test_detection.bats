#!/usr/bin/env bats

# Mock HTTP server uses python3's http.server (in the host env + GNOME runtime),
# avoiding any sudo-dependent install of nc/ncat.

setup() {
  TMPROOT="$(mktemp -d)"
  export XDG_RUNTIME_DIR="$TMPROOT/run"
  export FLATPAK_ID="ai.lemonade_server.Lemonade"
  unset LEMONADE_FLATPAK_FORCE_BUNDLED LEMONADE_FLATPAK_NO_BEACON LEMONADE_HOST LEMONADE_PORT
  SUPERVISOR="$BATS_TEST_DIRNAME/../../lemonade-supervisor.sh"
  MOCK_PID=""
  BEACON_PID=""
}

teardown() {
  [ -n "${MOCK_PID:-}" ] && kill "$MOCK_PID" 2>/dev/null || true
  [ -n "${BEACON_PID:-}" ] && kill "$BEACON_PID" 2>/dev/null || true
  rm -rf "$TMPROOT"
}

# start_beacon <advertise_port> — repeatedly send a lemond beacon advertising
# 127.0.0.1:<advertise_port> to UDP 13305, so the supervisor's 1s listen window
# always catches one.
start_beacon() {
  local adv="$1"
  python3 -c "
import socket, json, time
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
msg = json.dumps({'host': '127.0.0.1', 'port': ${adv}}).encode()
while True:
    try: s.sendto(msg, ('127.0.0.1', 13305))
    except Exception: pass
    time.sleep(0.2)
" >/dev/null 2>&1 &
  BEACON_PID=$!
}

# start_mock_lemond_health <port> — launch a tiny python http server that
# answers GET /api/v1/health with 200, anything else with 404.
start_mock_lemond_health() {
  local port="$1"
  python3 -c "
import http.server, sys
class H(http.server.BaseHTTPRequestHandler):
    def log_message(self,*a,**k): pass
    def do_GET(self):
        if self.path == '/api/v1/health':
            self.send_response(200); self.end_headers(); self.wfile.write(b'OK')
        else:
            self.send_response(404); self.end_headers()
http.server.HTTPServer(('127.0.0.1', ${port}), H).serve_forever()
" >/dev/null 2>&1 &
  MOCK_PID=$!
  # Wait briefly for the listener to bind.
  for _ in 1 2 3 4 5 6 7 8 9 10; do
    if curl --silent --fail --max-time 0.2 \
         "http://127.0.0.1:${port}/api/v1/health" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.1
  done
  return 1
}

@test "no host server -> detect returns 1" {
  # Port 1 is reserved; no server will ever respond there.
  LEMONADE_HOST=127.0.0.1 LEMONADE_PORT=1 \
    run "$SUPERVISOR" --detect-host
  [ "$status" -eq 1 ]
}

@test "FORCE_BUNDLED bypasses detection -> returns 2" {
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 run "$SUPERVISOR" --detect-host
  [ "$status" -eq 2 ]
}

@test "host server reachable via HTTP -> returns 0 with host:port" {
  start_mock_lemond_health 12701
  LEMONADE_HOST=127.0.0.1 LEMONADE_PORT=12701 \
    run "$SUPERVISOR" --detect-host
  [ "$status" -eq 0 ]
  [[ "$output" == *"LEMOND_HOST=127.0.0.1"* ]]
  [[ "$output" == *"LEMOND_PORT=12701"* ]]
}

@test "beacon-advertised server is discovered when the direct check misses" {
  start_mock_lemond_health 12777
  start_beacon 12777
  # Direct check targets a port with no server; only the beacon can find it.
  LEMONADE_PORT=12999 run "$SUPERVISOR" --detect-host
  [ "$status" -eq 0 ]
  [[ "$output" == *"LEMOND_PORT=12777"* ]]
}

@test "NO_BEACON ignores a beacon-advertised server -> returns 1" {
  start_mock_lemond_health 12777
  start_beacon 12777
  LEMONADE_PORT=12999 LEMONADE_FLATPAK_NO_BEACON=1 \
    run "$SUPERVISOR" --detect-host
  [ "$status" -eq 1 ]
}
