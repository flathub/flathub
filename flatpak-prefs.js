//This pref file disables checking of automatic updates
//and checking the browser is the default, as flatpak
//doesn't play well with these features.

pref("browser.shell.checkDefaultBrowser", false);
pref("app.update.auto", false);
pref("app.update.enabled", false);
pref("app.update.autoInstallEnabled", false);