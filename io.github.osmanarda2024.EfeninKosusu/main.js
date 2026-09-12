#!/usr/bin/env gjs
imports.gi.versions.Gtk = '4.0';
imports.gi.versions.WebKit = '6.0';

const { Gtk, WebKit, Gio, GLib } = imports.gi;

const APP_ID = 'io.github.osmanarda2024.EfeninKosusu';
const GAME_FILE = '/app/share/' + APP_ID + '/index.html';

const app = new Gtk.Application({ application_id: APP_ID });

app.connect('activate', () => {
    const win = new Gtk.ApplicationWindow({
        application: app,
        title: "Efe'nin Koşusu",
        default_width: 480,
        default_height: 854,
    });

    const webview = new WebKit.WebView();
    const settings = webview.get_settings();
    settings.enable_developer_extras = false;
    settings.enable_smooth_scrolling = true;

    webview.load_uri(GLib.filename_to_uri(GAME_FILE, null));
    win.set_child(webview);
    win.present();
});

app.run([imports.system.programInvocationName].concat(ARGV));
