#!/usr/bin/env python3

import gi
import os
import sys
import logging

gi.require_version("Gtk", "3.0")
gi.require_version("WebKit2", "4.1")

from gi.repository import Gtk, WebKit2, GLib

USER_AGENT = (
    "Mozilla/5.0 (X11; Linux x86_64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/131.0.0.0 Safari/537.36"
)
APP_ID = "io.github.manoelmsnsi.LinuxWhatsappFlatpak"

class ClientWindow(Gtk.Window):
    def __init__(self):
        super().__init__(title="WhatsApp")
        self.set_default_size(1000, 700)


        data_home = os.environ.get(
            "XDG_DATA_HOME",
            os.path.expanduser("~/.local/share")
        )

        base_path = os.path.join(data_home, APP_ID, "wtp_data")
        log_file = os.path.join(base_path, "application.log")

        try:
            os.makedirs(base_path, exist_ok=True)
        except OSError as e:
            sys.stderr.write(f"Erro ao criar diretório de dados: {e}\n")
            sys.exit(1)

        logging.basicConfig(
            filename=log_file,
            level=logging.INFO,
            format="%(asctime)s - %(levelname)s - %(message)s"
        )

        data_manager = WebKit2.WebsiteDataManager(
            base_data_directory=base_path,
            base_cache_directory=base_path
        )

        context = WebKit2.WebContext.new_with_website_data_manager(data_manager)
        self.webview = WebKit2.WebView.new_with_context(context)

        self.webview.connect("decide-policy", self.on_decide_policy)
        self.webview.connect("create", self.on_create_web_view)

        settings = self.webview.get_settings()
        settings.set_user_agent(USER_AGENT)

        self.webview.load_uri("https://web.whatsapp.com/")
        self.add(self.webview)

    def on_decide_policy(self, webview, decision, decision_type):
        if decision_type == WebKit2.PolicyDecisionType.NAVIGATION_ACTION:
            uri = decision.get_navigation_action().get_request().get_uri()
            if uri and not uri.startswith("https://web.whatsapp.com"):
                Gtk.show_uri_on_window(self, uri, Gtk.get_current_event_time())
                decision.ignore()
                return True
        return False

    def on_create_web_view(self, webview, navigation_action):
        uri = navigation_action.get_request().get_uri()
        if uri:
            Gtk.show_uri_on_window(self, uri, Gtk.get_current_event_time())
        return None

if __name__ == "__main__":
    GLib.set_prgname("whatsapp")

    app = ClientWindow()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()
