#!/usr/bin/python3

from __future__ import annotations

import os
import socket
import subprocess
import sys
import urllib.error
import urllib.request

import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
gi.require_version("WebKit", "6.0")

from gi.repository import Adw, Gio, GLib, WebKit  # noqa: E402


APP_DIR = "/app/share/ubi-app"
APP_BIN = os.path.join(APP_DIR, "UBI.App")
APP_ID = "io.github.matthewpchapdelaine.ubi-system"
START_PATH = os.environ.get("UBI_APP_START_PATH", "/").strip() or "/"


def allocate_base_url() -> str:
    requested_port = os.environ.get("UBI_APP_PORT")
    if requested_port:
        return f"http://127.0.0.1:{requested_port}"

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]

    return f"http://127.0.0.1:{port}"


def build_target_url(base_url: str) -> str:
    normalized_path = START_PATH if START_PATH.startswith("/") else f"/{START_PATH}"
    if normalized_path == "/":
        return base_url
    return f"{base_url}{normalized_path}"


class UbiWindow(Adw.ApplicationWindow):
    def __init__(self, app: "UbiNativeHost") -> None:
        super().__init__(application=app, title="UBI System")
        self.set_default_size(1440, 960)

        header = Adw.HeaderBar()
        header.set_title_widget(Adw.WindowTitle(title="UBI System", subtitle="Managed UBI dashboards"))

        self._status = Adw.StatusPage(
            title="Starting UBI System",
            description="Launching the local application server and preparing the native dashboard shell.",
            icon_name=APP_ID,
        )

        self._webview = WebKit.WebView()
        self._webview.connect("load-changed", self._on_load_changed)

        toolbar_view = Adw.ToolbarView()
        toolbar_view.add_top_bar(header)
        toolbar_view.set_content(self._status)
        self.set_content(toolbar_view)

        self._toolbar_view = toolbar_view

    def load_app(self, url: str) -> None:
        self._webview.load_uri(url)

    def show_error(self, message: str) -> None:
        self._status.set_title("Unable to start UBI System")
        self._status.set_description(message)
        self._toolbar_view.set_content(self._status)

    def _on_load_changed(self, _: WebKit.WebView, event: WebKit.LoadEvent) -> None:
        if event == WebKit.LoadEvent.FINISHED:
            self._toolbar_view.set_content(self._webview)
            print("[ubi-native-host] webview-loaded", flush=True)


class UbiNativeHost(Adw.Application):
    def __init__(self) -> None:
        super().__init__(
            application_id=APP_ID,
            flags=Gio.ApplicationFlags.DEFAULT_FLAGS,
        )
        self._server: subprocess.Popen[str] | None = None
        self._window: UbiWindow | None = None
        self._attempts = 0
        self._base_url = allocate_base_url()
        self._target_url = build_target_url(self._base_url)

    def do_activate(self) -> None:
        if self._window is None:
            self._window = UbiWindow(self)
            self._start_server()
        self._window.present()

    def do_shutdown(self) -> None:
        self._stop_server()
        super().do_shutdown()

    def _start_server(self) -> None:
        if self._server is not None:
            return

        if not os.path.exists(APP_BIN):
            raise FileNotFoundError(f"Missing application binary: {APP_BIN}")

        env = os.environ.copy()
        env["UBI_DISABLE_HTTPS_REDIRECT"] = "1"
        command = [
            APP_BIN,
            "--urls",
            self._base_url,
            "--contentRoot",
            APP_DIR,
            "--webroot",
            os.path.join(APP_DIR, "wwwroot"),
        ]
        self._server = subprocess.Popen(
            command,
            cwd=APP_DIR,
            env=env,
            stdout=sys.stdout,
            stderr=sys.stderr,
            text=True,
        )
        print(f"[ubi-native-host] server-started {self._base_url}", flush=True)
        GLib.timeout_add(250, self._poll_server_ready)

    def _poll_server_ready(self) -> bool:
        self._attempts += 1

        if self._server is not None and self._server.poll() is not None:
            if self._window is not None:
                self._window.show_error("The local server exited before the dashboard window could load.")
            return False

        try:
            with urllib.request.urlopen(self._base_url, timeout=1) as response:
                if response.status < 500:
                    if self._window is not None:
                        self._window.load_app(self._target_url)
                    print(f"[ubi-native-host] server-ready {self._target_url}", flush=True)
                    return False
        except (urllib.error.URLError, TimeoutError):
            pass

        if self._attempts >= 80:
            if self._window is not None:
                self._window.show_error("Timed out while waiting for the embedded UBI application server.")
            return False

        return True

    def _stop_server(self) -> None:
        if self._server is None:
            return

        if self._server.poll() is None:
            self._server.terminate()
            try:
                self._server.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._server.kill()
                self._server.wait(timeout=5)

        self._server = None


def main() -> int:
    app = UbiNativeHost()
    return app.run(sys.argv)


if __name__ == "__main__":
    raise SystemExit(main())
