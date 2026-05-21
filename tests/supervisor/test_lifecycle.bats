#!/usr/bin/env bats

setup() {
  TMPROOT="$(mktemp -d)"
  export XDG_RUNTIME_DIR="$TMPROOT/run"
  export FLATPAK_ID="ai.lemonade_server.Lemonade"
  export PATH="$TMPROOT/bin:$PATH"
  export LEMONADE_DATA_DIR="$TMPROOT/data"
  mkdir -p "$TMPROOT/bin" "$TMPROOT/data/logs" "$TMPROOT/run"
  unset LEMONADE_FLATPAK_FORCE_BUNDLED LEMONADE_FLATPAK_NO_BEACON LEMONADE_HOST LEMONADE_PORT
  SUPERVISOR="$BATS_TEST_DIRNAME/../../lemonade-supervisor.sh"
  export LEMONADE_BIN_LEMOND="$TMPROOT/bin/lemond"
  export LEMONADE_BIN_TRAY="$TMPROOT/bin/lemonade-tray"
  export LEMONADE_BIN_APP="$TMPROOT/bin/lemonade-app"

  # Mock lemond: HTTP server on chosen port; /api/v1/health -> 200,
  # POST /internal/shutdown -> writes marker then exits.
  # `exec python3` so SIGTERM to the wrapper propagates to python (no orphans).
  cat > "$LEMONADE_BIN_LEMOND" <<EOF
#!/usr/bin/env bash
echo "\$@" > "$TMPROOT/lemond.argv"
port=13305
while [ \$# -gt 0 ]; do
  case "\$1" in
    --port) port="\$2"; shift 2;;
    --host) shift 2;;
    *) shift;;
  esac
done
exec python3 -c "
import http.server, os, threading, time
PORT = int('\$port')
class H(http.server.BaseHTTPRequestHandler):
    def log_message(self,*a,**k): pass
    def do_GET(self):
        if self.path == '/api/v1/health':
            self.send_response(200); self.end_headers(); self.wfile.write(b'OK')
        else: self.send_response(404); self.end_headers()
    def do_POST(self):
        if self.path == '/internal/shutdown':
            open('$TMPROOT/lemond.shutdown', 'w').close()
            self.send_response(200); self.end_headers(); self.wfile.write(b'OK')
            # os._exit terminates the whole process from any thread; sys.exit
            # only terminates the calling thread when invoked from a daemon.
            threading.Thread(target=lambda: (time.sleep(0.2), os._exit(0)), daemon=True).start()
        else: self.send_response(404); self.end_headers()
http.server.HTTPServer(('127.0.0.1', PORT), H).serve_forever()
"
EOF
  chmod +x "$LEMONADE_BIN_LEMOND"

  # Mock tray: writes its PID file, sleeps until killed.
  cat > "$LEMONADE_BIN_TRAY" <<EOF
#!/usr/bin/env bash
echo \$\$ > "$TMPROOT/tray.pid"
trap 'rm -f "$TMPROOT/tray.pid"; exit 0' TERM INT
while true; do sleep 1; done
EOF
  chmod +x "$LEMONADE_BIN_TRAY"

  # Mock app: records args, writes its PID, sleeps until killed. Mirrors a real
  # Tauri app that stays open after launching, so the supervisor's
  # wait-n loop has something live to observe.
  cat > "$LEMONADE_BIN_APP" <<EOF
#!/usr/bin/env bash
echo "\$@" > "$TMPROOT/app.argv"
echo \$\$ > "$TMPROOT/app.pid"
trap 'rm -f "$TMPROOT/app.pid"; exit 0' TERM INT
while true; do sleep 1; done
EOF
  chmod +x "$LEMONADE_BIN_APP"
}

teardown() {
  # After exec, the lemond mock's cmdline is `python3 -c "..."` containing the
  # tmpdir-keyed shutdown-marker path — pkill -f against that path catches it.
  pkill -f "$TMPROOT/lemond.shutdown" 2>/dev/null || true
  pkill -f "$TMPROOT/bin/lemonade-tray" 2>/dev/null || true
  pkill -f "$TMPROOT/bin/lemonade-app"  2>/dev/null || true
  pkill -f "$TMPROOT/bin/lemond"        2>/dev/null || true
  rm -rf "$TMPROOT"
}

@test "no host lemond: supervisor starts bundled, app runs, tray quit triggers /internal/shutdown" {
  export LEMONADE_PORT=13399
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 timeout 15 "$SUPERVISOR" deep-link://test &
  SUP_PID=$!
  for i in $(seq 1 100); do [ -f "$TMPROOT/tray.pid" ] && [ -f "$TMPROOT/app.pid" ] && break; sleep 0.1; done
  [ -f "$TMPROOT/tray.pid" ]
  [ -f "$TMPROOT/app.pid" ]
  grep -q "deep-link://test" "$TMPROOT/app.argv"
  # owner=flatpak: tray quit -> /internal/shutdown AND desktop app gets closed.
  kill -TERM "$(cat $TMPROOT/tray.pid)"
  for i in $(seq 1 50); do [ -f "$TMPROOT/lemond.shutdown" ] && break; sleep 0.1; done
  [ -f "$TMPROOT/lemond.shutdown" ]
  for i in $(seq 1 30); do [ ! -f "$TMPROOT/app.pid" ] && break; sleep 0.1; done
  [ ! -f "$TMPROOT/app.pid" ]
  wait "$SUP_PID" 2>/dev/null || true
}

@test "LEMONADE_HOST/LEMONADE_PORT reach the bundled server's bind args" {
  export LEMONADE_PORT=13396
  export LEMONADE_HOST=0.0.0.0
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 timeout 15 "$SUPERVISOR" >/dev/null &
  SUP_PID=$!
  for i in $(seq 1 100); do [ -f "$TMPROOT/lemond.argv" ] && [ -f "$TMPROOT/tray.pid" ] && break; sleep 0.1; done
  [ -f "$TMPROOT/lemond.argv" ]
  # Bundled lemond is invoked with the host/port the user set. (Mock binds
  # loopback regardless, so the 0.0.0.0 -> 127.0.0.1 connect mapping lets it
  # come up.)
  grep -q -- "--host 0.0.0.0" "$TMPROOT/lemond.argv"
  grep -q -- "--port 13396" "$TMPROOT/lemond.argv"
  kill -TERM "$(cat $TMPROOT/tray.pid)" 2>/dev/null || true
  wait "$SUP_PID" 2>/dev/null || true
}

@test "external host lemond: tray quit does NOT shut down host server (but closes app)" {
  "$LEMONADE_BIN_LEMOND" --port 13398 &
  HOST_PID=$!
  for i in $(seq 1 30); do
    curl -sf --max-time 0.2 http://127.0.0.1:13398/api/v1/health && break
    sleep 0.1
  done
  rm -f "$TMPROOT/lemond.shutdown"
  LEMONADE_HOST=127.0.0.1 LEMONADE_PORT=13398 \
    timeout 15 "$SUPERVISOR" >/dev/null &
  SUP_PID=$!
  for i in $(seq 1 100); do [ -f "$TMPROOT/tray.pid" ] && [ -f "$TMPROOT/app.pid" ] && break; sleep 0.1; done
  [ -f "$TMPROOT/tray.pid" ]
  [ -f "$TMPROOT/app.pid" ]
  kill -TERM "$(cat $TMPROOT/tray.pid)"
  for i in $(seq 1 30); do [ ! -f "$TMPROOT/app.pid" ] && break; sleep 0.1; done
  [ ! -f "$TMPROOT/app.pid" ]
  # Ownership split: external server is never shut down by the supervisor.
  [ ! -f "$TMPROOT/lemond.shutdown" ]
  curl -sf --max-time 0.2 http://127.0.0.1:13398/api/v1/health
  kill -KILL $HOST_PID 2>/dev/null || true
  wait "$SUP_PID" 2>/dev/null || true
}

@test "tray quit closes apps across multiple sandboxes (sidecar pattern)" {
  export LEMONADE_PORT=13395
  # Mock app: each invocation writes its PID to a unique slot under app-pids/
  # and stays running until killed. We count live instances by listing the dir.
  cat > "$LEMONADE_BIN_APP" <<EOF
#!/usr/bin/env bash
mkdir -p "$TMPROOT/app-pids"
touch "$TMPROOT/app-pids/\$\$"
trap 'rm -f "$TMPROOT/app-pids/\$\$"; exit 0' TERM INT
while true; do sleep 1; done
EOF
  chmod +x "$LEMONADE_BIN_APP"

  # First invocation: holds the lock, starts lemond + tray + initial app.
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 timeout 25 "$SUPERVISOR" >/dev/null &
  SUP_PID=$!
  for i in $(seq 1 100); do [ -f "$TMPROOT/tray.pid" ] && [ -n "$(ls $TMPROOT/app-pids 2>/dev/null)" ] && break; sleep 0.1; done
  [ -f "$TMPROOT/tray.pid" ]
  [ "$(ls $TMPROOT/app-pids 2>/dev/null | wc -l)" -eq 1 ]

  # User closes the first app window. Tray + supervisor stay alive.
  for f in "$TMPROOT/app-pids"/*; do kill -TERM "$(basename $f)"; done
  for i in $(seq 1 30); do [ -z "$(ls $TMPROOT/app-pids 2>/dev/null)" ] && break; sleep 0.1; done
  [ -z "$(ls $TMPROOT/app-pids 2>/dev/null)" ]
  [ -f "$TMPROOT/tray.pid" ]

  # Second `flatpak run` (background — the sidecar blocks on its app).
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 timeout 25 "$SUPERVISOR" "deep-link://from-second-run" >/dev/null &
  SIDECAR_B=$!
  for i in $(seq 1 100); do [ "$(ls $TMPROOT/app-pids 2>/dev/null | wc -l)" -ge 1 ] && break; sleep 0.1; done
  [ "$(ls $TMPROOT/app-pids 2>/dev/null | wc -l)" -eq 1 ]

  # Third `flatpak run` while the second's app is still up: TWO live apps now.
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 timeout 25 "$SUPERVISOR" >/dev/null &
  SIDECAR_C=$!
  for i in $(seq 1 100); do [ "$(ls $TMPROOT/app-pids 2>/dev/null | wc -l)" -ge 2 ] && break; sleep 0.1; done
  [ "$(ls $TMPROOT/app-pids 2>/dev/null | wc -l)" -eq 2 ]

  # Quit tray → supervisor exits → exclusive lock released → both sidecars
  # unblock on their shared-flock wait and tear down their apps.
  kill -TERM "$(cat $TMPROOT/tray.pid)"
  for i in $(seq 1 50); do [ -z "$(ls $TMPROOT/app-pids 2>/dev/null)" ] && break; sleep 0.1; done
  [ -z "$(ls $TMPROOT/app-pids 2>/dev/null)" ]
  for i in $(seq 1 50); do [ -f "$TMPROOT/lemond.shutdown" ] && break; sleep 0.1; done
  [ -f "$TMPROOT/lemond.shutdown" ]
  wait "$SUP_PID" "$SIDECAR_B" "$SIDECAR_C" 2>/dev/null || true
}

@test "sidecar killed externally still cleans up its app (no orphans)" {
  export LEMONADE_PORT=13394
  # Mock app: persistent + tracks via pid file under app-pids/.
  cat > "$LEMONADE_BIN_APP" <<EOF
#!/usr/bin/env bash
mkdir -p "$TMPROOT/app-pids"
touch "$TMPROOT/app-pids/\$\$"
trap 'rm -f "$TMPROOT/app-pids/\$\$"; exit 0' TERM INT
while true; do sleep 1; done
EOF
  chmod +x "$LEMONADE_BIN_APP"

  # Supervisor + initial app.
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 timeout 25 "$SUPERVISOR" >/dev/null &
  SUP_PID=$!
  for i in $(seq 1 100); do [ -f "$TMPROOT/tray.pid" ] && [ -n "$(ls $TMPROOT/app-pids 2>/dev/null)" ] && break; sleep 0.1; done

  # Close the initial app so the next state is "tray alive, no app".
  for f in "$TMPROOT/app-pids"/*; do kill -TERM "$(basename $f)"; done
  for i in $(seq 1 30); do [ -z "$(ls $TMPROOT/app-pids 2>/dev/null)" ] && break; sleep 0.1; done

  # Launch a sidecar and wait for its app to come up. We invoke the
  # supervisor directly (no `timeout` wrapper) so the SIGTERM below
  # targets only the sidecar bash, not the whole process group — that's
  # what proves the trap registration is what cleans the app up.
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 "$SUPERVISOR" >/dev/null &
  SIDECAR_PID=$!
  for i in $(seq 1 100); do [ -n "$(ls $TMPROOT/app-pids 2>/dev/null)" ] && break; sleep 0.1; done
  [ -n "$(ls $TMPROOT/app-pids 2>/dev/null)" ]

  # SIGTERM the sidecar bash. Without the early trap registration the
  # bash terminates without running on_exit, the app inherits init as
  # its parent, and it stays alive as an orphan.
  kill -TERM "$SIDECAR_PID"
  for i in $(seq 1 30); do [ -z "$(ls $TMPROOT/app-pids 2>/dev/null)" ] && break; sleep 0.1; done
  [ -z "$(ls $TMPROOT/app-pids 2>/dev/null)" ]

  # Tray quit to wrap up the supervisor.
  kill -TERM "$(cat $TMPROOT/tray.pid)" 2>/dev/null || true
  wait "$SUP_PID" "$SIDECAR_PID" 2>/dev/null || true
}

@test "tray fails to initialize: supervisor falls back to app-only mode, app close triggers shutdown" {
  # Replace the persistent mock tray with one that exits immediately,
  # simulating libayatana-appindicator failing because no
  # StatusNotifierWatcher is on the bus (e.g. stock GNOME without the
  # AppIndicator extension).
  cat > "$LEMONADE_BIN_TRAY" <<EOF
#!/usr/bin/env bash
exit 1
EOF
  chmod +x "$LEMONADE_BIN_TRAY"

  export LEMONADE_PORT=13397
  rm -f "$TMPROOT/lemond.shutdown"
  LEMONADE_FLATPAK_FORCE_BUNDLED=1 timeout 15 "$SUPERVISOR" >/dev/null &
  SUP_PID=$!
  # App must start despite the tray failing.
  for i in $(seq 1 100); do [ -f "$TMPROOT/app.pid" ] && break; sleep 0.1; done
  [ -f "$TMPROOT/app.pid" ]
  # No tray.pid was ever written (mock tray exited before reaching that line).
  [ ! -f "$TMPROOT/tray.pid" ]
  # User must close the app window to stop everything.
  kill -TERM "$(cat $TMPROOT/app.pid)"
  for i in $(seq 1 50); do [ -f "$TMPROOT/lemond.shutdown" ] && break; sleep 0.1; done
  [ -f "$TMPROOT/lemond.shutdown" ]
  wait "$SUP_PID" 2>/dev/null || true
}

