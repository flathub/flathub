#!/usr/bin/env python3
import os
import signal
import subprocess
import time
import urllib.request
import webbrowser

def wait_for_server(url: str, timeout: float = 10.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with urllib.request.urlopen(url, timeout=0.5) as resp:
                if resp.status == 200:
                    return
        except Exception:
            pass
        time.sleep(0.2)

def main() -> None:
    bin_dir = os.path.dirname(os.path.abspath(__file__))
    server_bin = os.path.join(bin_dir, "litho-hub")
    proc = subprocess.Popen([server_bin], cwd=bin_dir)
    try:
        wait_for_server("http://127.0.0.1:8080/api/health")
        try:
            import gi
            gi.require_version("Gtk", "3.0")
            gi.require_version("WebKit2", "4.1")
            from gi.repository import Gtk, WebKit2

            win = Gtk.Window(title="Lithotheque Server")
            win.set_default_size(1280, 820)
            webview = WebKit2.WebView()
            webview.load_uri("http://127.0.0.1:8080/")
            win.add(webview)
            win.connect("destroy", Gtk.main_quit)
            win.show_all()
            Gtk.main()
        except Exception:
            webbrowser.open("http://127.0.0.1:8080/")
            proc.wait()
    finally:
        if proc.poll() is None:
            proc.send_signal(signal.SIGTERM)
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()

if __name__ == "__main__":
    main()
