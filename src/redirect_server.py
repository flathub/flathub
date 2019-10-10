#!/usr/bin/env python

import fcntl
import http.server
import io
import os
import socket
import socketserver
import stat
import subprocess
import sys
import threading
import time

from contextlib import closing
from kolibri.utils import conf
from kolibri.utils import server
from kolibri.utils import cli

REDIRECT_PORT = int(os.environ.get("REDIRECT_PORT", 8081))
IDLE_TIMEOUT_MINS = int(os.environ.get("IDLE_TIMEOUT_MINS", 20))
LOCKFILE = os.path.join(conf.KOLIBRI_HOME, "flatpak.lock")

heartbeat_timestamp = time.time()


try:
    f = open(LOCKFILE, "w+")
    f.write("")
    fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
except io.BlockingIOError:
    print("Kolibri flatpak is already running; not starting server again.")
    sys.exit()


def kolibri_status():
    return server.get_urls()[0]


def kolibri_port():
    if kolibri_status() == server.STATUS_RUNNING:
        return server.get_status()[2]


def port_in_use(port=REDIRECT_PORT):
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        return sock.connect_ex(("127.0.0.1", port)) == 0


class RedirectHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # anytime the redirect server receives a request, reset the heartbeat timer
        self.server.last_heartbeat = time.time()
        # determine what port Kolibri is running on, or if not running at all
        port = kolibri_port()
        if (not port) or ("heartbeat.png" in self.path):
            # serve up the static files
            return http.server.SimpleHTTPRequestHandler.do_GET(self)
        else:
            # redirect to the Kolibri server
            return self.redirect(port)

    def redirect(self, port):
        self.send_response(302)
        self.send_header("Location", "http://127.0.0.1:{}/".format(port))
        self.end_headers()


# start Kolibri if it's not already running
status = kolibri_status()
print("Kolibri status ({}): {}".format(status, cli.status.codes[status]))
if status in [
    server.STATUS_STOPPED,
    server.STATUS_FAILED_TO_START,
    server.STATUS_UNKNOWN,
]:
    print("Starting Kolibri!")
    subprocess.Popen(["kolibri", "start"])
elif status in [server.STATUS_NOT_RESPONDING]:
    print("Restarting Kolibri!")
    subprocess.Popen(["kolibri", "restart"])
elif status in [server.STATUS_UNCLEAN_SHUTDOWN, server.STATUS_FAILED_TO_START]:
    print("Clearing lock files and starting Kolibri!")
    if os.path.exists(server.STARTUP_LOCK):
        os.remove(server.STARTUP_LOCK)
    if os.path.exists(server.PID_FILE):
        os.remove(server.PID_FILE)
    subprocess.Popen(["kolibri", "start"])
else:
    print("Warning: not starting Kolibri.")


# if the redirect server port is in use, assume it's already running and bail
if port_in_use():
    sys.exit()

# start the redirect server in a daemon thread
socketserver.TCPServer.allow_reuse_address = True
httpd = socketserver.TCPServer(("", REDIRECT_PORT), RedirectHandler)
httpd.last_heartbeat = time.time()
thread = threading.Thread(target=httpd.serve_forever)
thread.daemon = True
thread.start()

# monitor time since last heartbeat and exit if idle
while True:
    time.sleep(30)
    seconds_since_heartbeat = time.time() - httpd.last_heartbeat
    print("Last heartbeat was {} seconds ago...".format(seconds_since_heartbeat))
    if seconds_since_heartbeat > (IDLE_TIMEOUT_MINS * 60):
        # ensure there aren't any jobs still running; if so, refrain from shutting down for now
        job_count = subprocess.run("/app/bin/check_for_running_tasks.sh").returncode
        if job_count > 0:
            print(
                "There are still {} jobs running; not shutting down despite idle.".format(
                    job_count
                )
            )
            continue
        print("Server is idle; shutting down...")
        subprocess.Popen(["kolibri", "stop"])
        httpd.shutdown()
        time.sleep(10)
        sys.exit()
