#!/usr/bin/env python3
"""ROMarr desktop shell: a loopback-only ROMarr server inside a native window.

Used by the Flatpak (and anywhere GTK 4 + WebKitGTK 6 are present). The
server binds 127.0.0.1 only, so a desktop install is never reachable from the
network and never sits unclaimed on the LAN waiting for a stranger to set its
password.

    romarr-desktop             server + window (the normal launcher)
    romarr-desktop --headless  server only, for Steam Deck game mode, SSH, or
                               anyone who prefers their own browser

Environment overrides: ROMARR_PORT (default 6868, or a free port when 6868 is
taken), ROMARR_DATA, ROMARR_LIBRARY (the game library on first start),
ROMARR_LIBRARY_ROOT (where the ROMarr code lives; the Flatpak installs it under
/app/lib/romarr).

An inherited LIBRARY_PATH is deliberately ignored: it is ROMarr's name for the
library, but on a desktop it is far more often the compiler's linker search
path, and ROMarr would file games into it.
"""

from __future__ import annotations

import json
import logging
import os
import socket
import sys
import threading
import time
import urllib.request
from pathlib import Path

APP_ID = "app.romarr.ROMarr"
DEFAULT_PORT = 6868
HEALTH_TIMEOUT_S = 60
POLL_INTERVAL_MS = 250

log = logging.getLogger("romarr-desktop")


def xdg_dir(var: str, fallback: str) -> Path:
    return Path(os.environ.get(var) or Path.home() / fallback)


def data_home() -> Path:
    return xdg_dir("XDG_DATA_HOME", ".local/share") / "romarr"


def default_library(media: Path = Path("/run/media")) -> Path:
    """The first ROM folder that already exists, else ~/Games/ROMs.

    EmuDeck and RetroDECK put ROMs under ~/Emulation/roms (or the same path on
    the SD card on a Steam Deck), so an existing tree there is the library the
    user already has. Only read once: ROMarr stores the library after first
    start, and the Settings page changes it from then on.
    """
    home = Path.home()
    candidates = [home / "Emulation" / "roms", home / "retrodeck" / "roms"]
    if media.is_dir():
        candidates += sorted(media.glob("*/Emulation/roms"))
        candidates += sorted(media.glob("*/*/Emulation/roms"))
    for path in candidates:
        if path.is_dir():
            return path
    return home / "Games" / "ROMs"


def port_is_free(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        try:
            sock.bind(("127.0.0.1", port))
        except OSError:
            return False
    return True


def choose_port() -> int:
    try:
        wanted = int(os.environ.get("ROMARR_PORT") or DEFAULT_PORT)
    except ValueError:
        log.warning("ROMARR_PORT is not a number; using %d", DEFAULT_PORT)
        wanted = DEFAULT_PORT
    if port_is_free(wanted):
        return wanted
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def prepare_environment() -> int:
    """Point ROMarr at per-user paths and loopback, and return the port."""
    data = data_home()
    data.mkdir(parents=True, exist_ok=True)
    port = choose_port()
    os.environ["ROMARR_HOST"] = "127.0.0.1"
    os.environ["ROMARR_PORT"] = str(port)
    os.environ.setdefault("ROMARR_DATA", str(data / "romarr.json"))
    os.environ.setdefault("ROM_HUB_HOME", str(data / "rom-hub"))
    os.environ.pop("ROMM_LIBRARY", None)
    library = Path(os.environ.get("ROMARR_LIBRARY") or default_library())
    library.mkdir(parents=True, exist_ok=True)
    os.environ["LIBRARY_PATH"] = str(library)
    code = os.environ.get("ROMARR_LIBRARY_ROOT", "/app/lib/romarr")
    if Path(code, "romarr").is_dir() and code not in sys.path:
        sys.path.insert(0, code)
    return port


def start_server(serve, port: int) -> None:
    """Run ROMarr's own HTTP server on a daemon thread; it dies with the app."""
    def run() -> None:
        try:
            serve(port=port)
        except Exception:  # surfaced in the log and by the health wait
            log.exception("ROMarr server stopped")

    threading.Thread(target=run, name="romarr-server", daemon=True).start()


# Loopback never goes through a proxy, whatever http_proxy says.
_DIRECT = urllib.request.build_opener(urllib.request.ProxyHandler({}))


def is_healthy(port: int) -> bool:
    try:
        with _DIRECT.open(
                f"http://127.0.0.1:{port}/api/health", timeout=0.5) as answer:
            return json.load(answer).get("ok") is True
    except (OSError, ValueError, AttributeError):
        return False


def run_headless(serve, port: int) -> int:
    log.info("ROMarr is serving on http://127.0.0.1:%d", port)
    serve(port=port)
    return 0


def unique_path(folder: Path, suggested: str) -> Path:
    """`name.ext`, or `name (2).ext` and so on when that file already exists."""
    name = Path(suggested or "download").name or "download"
    stem, suffix = Path(name).stem, Path(name).suffix
    candidate, n = folder / name, 2
    while candidate.exists():
        candidate = folder / f"{stem} ({n}){suffix}"
        n += 1
    return candidate


LOADING_PAGE = """<!doctype html><html><head><meta charset="utf-8">
<style>body{margin:0;height:100vh;display:grid;place-items:center;
background:#0b1626;color:#d5e6fa;font:16px system-ui,sans-serif}</style>
</head><body><p>{message}</p></body></html>"""


def run_window(serve, port: int) -> int:
    import gi

    gi.require_version("Gtk", "4.0")
    gi.require_version("WebKit", "6.0")
    from gi.repository import Gio, GLib, Gtk, WebKit

    url = f"http://127.0.0.1:{port}/"
    data = data_home() / "webview"
    cache = xdg_dir("XDG_CACHE_HOME", ".cache") / "romarr" / "webview"

    def on_policy(_view, decision, kind) -> bool:
        if kind != WebKit.PolicyDecisionType.NEW_WINDOW_ACTION:
            return False
        uri = decision.get_navigation_action().get_request().get_uri()
        Gio.AppInfo.launch_default_for_uri(uri, None)
        decision.ignore()
        return True

    def on_download(_session, download) -> None:
        def decide(dl, suggested: str) -> bool:
            folder = GLib.get_user_special_dir(
                GLib.UserDirectory.DIRECTORY_DOWNLOAD) or str(Path.home())
            dl.set_destination(str(unique_path(Path(folder), suggested)))
            return True
        download.connect("decide-destination", decide)

    def build_view() -> WebKit.WebView:
        session = WebKit.NetworkSession.new(str(data), str(cache))
        # The window only ever loads 127.0.0.1; links elsewhere open in the
        # system browser. A system or http_proxy proxy would swallow loopback.
        session.set_proxy_settings(WebKit.NetworkProxyMode.NO_PROXY, None)
        session.get_cookie_manager().set_persistent_storage(
            str(data / "cookies.sqlite"), WebKit.CookiePersistentStorage.SQLITE)
        session.connect("download-started", on_download)
        view = WebKit.WebView(network_session=session)
        view.connect("decide-policy", on_policy)
        view.load_html(LOADING_PAGE.replace("{message}", "Starting ROMarr…"), url)
        return view

    def wait_then_load(view, started: float) -> bool:
        if is_healthy(port):
            view.load_uri(url)
            return GLib.SOURCE_REMOVE
        if time.monotonic() - started > HEALTH_TIMEOUT_S:
            view.load_html(LOADING_PAGE.replace(
                "{message}", "ROMarr did not start. Run "
                "<code>flatpak run app.romarr.ROMarr --headless</code> "
                "in a terminal to see why."), url)
            return GLib.SOURCE_REMOVE
        return GLib.SOURCE_CONTINUE

    app = Gtk.Application(application_id=APP_ID)

    def on_activate(application) -> None:
        if application.get_active_window():
            application.get_active_window().present()
            return
        window = Gtk.ApplicationWindow(application=application, title="ROMarr")
        window.set_default_size(1280, 820)
        view = build_view()
        window.set_child(view)
        window.present()
        GLib.timeout_add(POLL_INTERVAL_MS, wait_then_load, view, time.monotonic())

    app.connect("startup", lambda _app: start_server(serve, port))
    app.connect("activate", on_activate)
    return app.run([sys.argv[0]])


def main(argv: list[str]) -> int:
    logging.basicConfig(
        level=os.environ.get("LOG_LEVEL", "INFO").upper(),
        format="%(asctime)s %(levelname)s %(name)s %(message)s")
    port = prepare_environment()
    from romarr.app import serve  # fail here, loudly, not inside GTK's startup
    headless = "--headless" in argv or os.environ.get("ROMARR_HEADLESS") == "1"
    no_display = not (os.environ.get("WAYLAND_DISPLAY") or os.environ.get("DISPLAY"))
    if headless or no_display:
        return run_headless(serve, port)
    try:
        return run_window(serve, port)
    except (ImportError, ValueError) as err:
        log.warning("no GTK 4 / WebKitGTK 6 window available (%s); "
                    "serving headless instead", err)
        return run_headless(serve, port)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
