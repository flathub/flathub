Trayscale
=========

Trayscale is an unofficial GUI wrapper around the Tailscale CLI client, particularly for use on Linux, as no official Linux GUI client exists. Despite the name, it does _not_ provide a tray icon, as support for them has been removed in Gtk4. If support can ever be brought back, however, a tray icon would be nice.

![image](https://user-images.githubusercontent.com/326750/188052311-2267af08-82a1-422f-b6ad-bc2cd4de0ac5.png)

Tailscale Config
----------------

Trayscale makes calls to the Tailscale CLI for some operations. In order for this to work, the `tailscale` command must be in your `$PATH`. Additionally, the daemon must have been configured with the current user as the "operator". To do this, run `sudo tailscale up --operator=$USER` from the command-line at least once manually.
