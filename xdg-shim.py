#!/usr/bin/env python3
# Minimal xdg-utils replacements for running Helium inside Flatpak.
#
# Chromium shells out to xdg-desktop-menu, xdg-icon-resource and xdg-settings
# to install web app (PWA) launchers and to manage the default browser. None of
# these exist in the Freedesktop runtime, so this script stands in for them and
# writes directly to the host locations the sandbox has been granted:
#   ~/.local/share/applications, ~/.local/share/icons, ~/.config/mimeapps.list
#
# Web app launchers are installed on the host as "<flatpak id>.<name>.desktop".
# KWin replaces the Wayland app_id of sandboxed windows with the Flatpak id
# unless it starts with that id, so helium.sh sets
# CHROME_WEB_APP_DESKTOP_ID_PREFIX to match and each PWA gets its own window
# identity and icon. Chromium itself still expects "<name>.desktop" in
# $XDG_DATA_HOME/applications to know the launcher exists, so an unprefixed
# copy is kept there, inside the sandbox where the desktop never sees it.
#
# Installed once and symlinked under each tool name; dispatches on argv[0].

import configparser
import os
import re
import shutil
import sys

HOME = os.path.expanduser("~")
HOST_APPS = os.path.join(HOME, ".local", "share", "applications")
SANDBOX_APPS = os.path.join(
    os.environ.get("XDG_DATA_HOME", os.path.join(HOME, ".local", "share")),
    "applications",
)
HOST_ICONS_ROOT = os.path.join(HOME, ".local", "share", "icons")
HOST_ICONS = os.path.join(HOST_ICONS_ROOT, "hicolor")
HOST_MIMEAPPS = os.path.join(HOME, ".config", "mimeapps.list")

BROWSER_TYPES = [
    "x-scheme-handler/http",
    "x-scheme-handler/https",
    "text/html",
    "application/xhtml+xml",
]


def flatpak_info():
    info = configparser.ConfigParser(interpolation=None)
    try:
        info.read("/.flatpak-info")
    except configparser.Error:
        pass
    app = info.get("Application", "name", fallback="net.imput.helium")
    arch = info.get("Instance", "arch", fallback="")
    branch = info.get("Instance", "branch", fallback="")
    return app, arch, branch


def host_launcher():
    app, arch, branch = flatpak_info()
    cmd = ["/usr/bin/flatpak", "run"]
    if branch:
        cmd.append(f"--branch={branch}")
    if arch:
        cmd.append(f"--arch={arch}")
    cmd += ["--command=helium", app]
    return " ".join(cmd)


def split_first_token(value):
    value = value.lstrip()
    if value.startswith('"'):
        m = re.match(r'"((?:[^"\\]|\\.)*)"', value)
        if m:
            return m.group(1), value[m.end():]
    head, _, rest = value.partition(" ")
    return head, (" " + rest) if rest else ""


def rewrite_exec(contents):
    launcher = host_launcher()
    sandbox_data = os.environ.get("XDG_DATA_HOME", "")
    out = []
    for line in contents.splitlines():
        if line.startswith("Exec="):
            prog, rest = split_first_token(line[len("Exec="):])
            if prog.startswith("/app/") or os.path.basename(prog) in (
                "helium",
                "helium.real",
            ):
                line = "Exec=" + launcher + rest
        elif line.startswith("Icon=") and sandbox_data:
            # Chromium writes icon paths under the sandbox $XDG_DATA_HOME.
            prefix = sandbox_data.rstrip("/") + "/icons/"
            if line[len("Icon="):].startswith(prefix):
                line = "Icon=" + os.path.join(
                    HOST_ICONS_ROOT, line[len("Icon=") + len(prefix):]
                )
        out.append(line)
    return "\n".join(out) + "\n"


def write_in_place(path, data):
    # Write without rename: single files may be bind-mounted into the sandbox.
    with open(path, "w", encoding="utf-8") as f:
        f.write(data)


def host_desktop_name(name):
    app = flatpak_info()[0]
    return name if name.startswith(app + ".") else f"{app}.{name}"


def unlink_quiet(path):
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass


def desktop_menu(args):
    if not args:
        return 1
    cmd, rest = args[0], args[1:]
    files = [a for a in rest if not a.startswith("--") and a not in ("user", "system")]
    if cmd == "install":
        os.makedirs(HOST_APPS, exist_ok=True)
        os.makedirs(SANDBOX_APPS, exist_ok=True)
        for src in files:
            if not src.endswith(".desktop"):
                continue  # .directory menu folders: no host access, skip
            name = os.path.basename(src)
            with open(src, encoding="utf-8") as f:
                data = rewrite_exec(f.read())
            write_in_place(os.path.join(HOST_APPS, host_desktop_name(name)), data)
            write_in_place(os.path.join(SANDBOX_APPS, name), data)
            if os.path.realpath(SANDBOX_APPS) != os.path.realpath(HOST_APPS):
                unlink_quiet(os.path.join(HOST_APPS, name))  # left by older shim
        return 0
    if cmd == "uninstall":
        for name in files:
            if name.endswith(".desktop"):
                name = os.path.basename(name)
                unlink_quiet(os.path.join(HOST_APPS, host_desktop_name(name)))
                unlink_quiet(os.path.join(SANDBOX_APPS, name))
        return 0
    if cmd == "forceupdate":
        return 0
    return 1


def icon_resource(args):
    if not args:
        return 1
    cmd, rest = args[0], args[1:]
    size, context, positional = None, "apps", []
    i = 0
    while i < len(rest):
        a = rest[i]
        if a in ("--size", "--context", "--mode", "--theme"):
            val = rest[i + 1] if i + 1 < len(rest) else ""
            if a == "--size":
                size = val
            elif a == "--context":
                context = val
            i += 2
            continue
        if not a.startswith("--"):
            positional.append(a)
        i += 1
    if cmd == "forceupdate":
        return 0
    if not size:
        return 1
    target_dir = os.path.join(HOST_ICONS, f"{size}x{size}", context)
    if cmd == "install":
        if not positional:
            return 1
        src = positional[0]
        name = positional[1] if len(positional) > 1 else os.path.splitext(os.path.basename(src))[0]
        ext = os.path.splitext(src)[1] or ".png"
        os.makedirs(target_dir, exist_ok=True)
        shutil.copyfile(src, os.path.join(target_dir, name + ext))
        return 0
    if cmd == "uninstall":
        for name in positional:
            for ext in (".png", ".xpm", ".svg"):
                try:
                    os.unlink(os.path.join(target_dir, name + ext))
                except FileNotFoundError:
                    pass
        return 0
    return 1


def read_mimeapps():
    try:
        with open(HOST_MIMEAPPS, encoding="utf-8") as f:
            return f.read().splitlines()
    except FileNotFoundError:
        return []


def get_default(mime):
    section = None
    for line in read_mimeapps():
        s = line.strip()
        if s.startswith("["):
            section = s
        elif section == "[Default Applications]" and s.startswith(mime + "="):
            return s.split("=", 1)[1].split(";")[0]
    return ""


def set_defaults(mimes, desktop):
    lines = read_mimeapps()
    out, section, seen, in_defaults = [], None, set(), False
    for line in lines:
        s = line.strip()
        if s.startswith("["):
            if in_defaults:
                blanks = 0
                while out and not out[-1].strip():
                    out.pop()
                    blanks += 1
                out += [f"{m}={desktop};" for m in mimes if m not in seen]
                out += [""] * blanks
                seen.update(mimes)
            section = s
            in_defaults = section == "[Default Applications]"
        elif in_defaults and "=" in s and s.split("=", 1)[0] in mimes:
            key = s.split("=", 1)[0]
            if key in seen:
                continue
            seen.add(key)
            line = f"{key}={desktop};"
        out.append(line)
    if in_defaults:
        out += [f"{m}={desktop};" for m in mimes if m not in seen]
    elif not any(l.strip() == "[Default Applications]" for l in lines):
        if out and out[-1].strip():
            out.append("")
        out.append("[Default Applications]")
        out += [f"{m}={desktop};" for m in mimes]
    write_in_place(HOST_MIMEAPPS, "\n".join(out) + "\n")


def settings(args):
    if len(args) < 2:
        return 1
    cmd, prop, rest = args[0], args[1], args[2:]
    if prop == "default-web-browser":
        mimes = BROWSER_TYPES
    elif prop == "default-url-scheme-handler" and rest:
        mimes = [f"x-scheme-handler/{rest[0]}"]
        rest = rest[1:]
    else:
        return 1
    try:
        if cmd == "get":
            print(get_default(mimes[0]))
            return 0
        if cmd == "check" and rest:
            print("yes" if get_default(mimes[0]) == rest[0] else "no")
            return 0
        if cmd == "set" and rest:
            set_defaults(mimes, rest[0])
            return 0
    except OSError as e:
        print(f"xdg-settings: {e}", file=sys.stderr)
        return 4
    return 1


def main():
    tool = os.path.basename(sys.argv[0])
    args = sys.argv[1:]
    try:
        if tool == "xdg-desktop-menu":
            return desktop_menu(args)
        if tool == "xdg-icon-resource":
            return icon_resource(args)
        if tool == "xdg-settings":
            return settings(args)
    except OSError as e:
        print(f"{tool}: {e}", file=sys.stderr)
        return 3
    print(f"{tool}: unsupported", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
