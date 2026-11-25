#!/bin/sh
_REAL_CONFIG="$(realpath "${XDG_CONFIG_HOME}" 2>/dev/null || printf '%s' "${XDG_CONFIG_HOME}")"
WIDEVINE_DIR="${_REAL_CONFIG}/net.imput.helium/WidevineCdm"

CDM_VERSION=$(python3 - "${WIDEVINE_DIR}" <<'PYEOF'
import sys, json, urllib.request, hashlib, zipfile, os, tempfile, shutil

base_dir = sys.argv[1]
meta_url = "https://raw.githubusercontent.com/mozilla-firefox/firefox/refs/heads/main/toolkit/content/gmp-sources/widevinecdm.json"

def find_installed():
    if not os.path.isdir(base_dir):
        return None
    for entry in sorted(os.listdir(base_dir), reverse=True):
        cdm = os.path.join(base_dir, entry, "_platform_specific", "linux_x64", "libwidevinecdm.so")
        if os.path.isfile(cdm):
            return entry
    return None

installed = find_installed()
if installed:
    print(installed)
    sys.exit(0)

try:
    with urllib.request.urlopen(meta_url, timeout=15) as r:
        meta = json.load(r)

    platform = meta["vendors"]["gmp-widevinecdm"]["platforms"]["Linux_x86_64-gcc3"]
    url = platform["mirrorUrls"][0]
    expected_hash = platform["hashValue"]

    with urllib.request.urlopen(url) as r:
        data = r.read()

    if hashlib.sha512(data).hexdigest() != expected_hash:
        raise ValueError("Widevine checksum mismatch")

    with tempfile.NamedTemporaryFile(suffix=".crx3", delete=False) as f:
        f.write(data)
        tmp = f.name

    try:
        with zipfile.ZipFile(tmp) as z:
            with z.open("manifest.json") as mf:
                version = json.load(mf).get("version", "")
            if not version:
                raise ValueError("could not read version from manifest.json")
            install_dir = os.path.join(base_dir, version)
            os.makedirs(install_dir, exist_ok=True)
            for name in ["manifest.json", "_platform_specific/linux_x64/libwidevinecdm.so"]:
                z.extract(name, install_dir)
        os.chmod(
            os.path.join(install_dir, "_platform_specific", "linux_x64", "libwidevinecdm.so"),
            0o755,
        )
    finally:
        os.unlink(tmp)

    for entry in os.listdir(base_dir):
        p = os.path.join(base_dir, entry)
        if os.path.isdir(p) and entry != version:
            shutil.rmtree(p, ignore_errors=True)
        elif os.path.isfile(p) and entry in ("manifest.json", "latest-component-updated-widevine-cdm"):
            try:
                os.unlink(p)
            except OSError:
                pass

    print(version)

except Exception as e:
    print(f"Widevine setup failed: {e}", file=sys.stderr)
    v = find_installed()
    if v:
        print(v)
PYEOF
)

if [ -n "${CDM_VERSION}" ] && [ -f "${WIDEVINE_DIR}/${CDM_VERSION}/_platform_specific/linux_x64/libwidevinecdm.so" ]; then
    export ZYPAK_EXPOSE_WIDEVINE_PATH="${WIDEVINE_DIR}"
fi

# Merge the policies with the host ones.
for proot in "etc/chromium/policies" "etc/static/chromium/policies"; do
  for ptype in managed recommended enrollment; do
    if [ -d "/run/host/$proot/$ptype" ]; then
      mkdir -p "/etc/chromium/policies/$ptype"
      ln -sf "/run/host/$proot/$ptype"/*.json "/etc/chromium/policies/$ptype" 2>/dev/null
    fi
  done
done

if [ -n "${XDG_DATA_HOME}" ]; then
  mkdir -p "${HOME}/.local/share/icons"
  if [ -d "${XDG_DATA_HOME}/icons" ] && [ ! -L "${XDG_DATA_HOME}/icons" ]; then
    cp -an "${XDG_DATA_HOME}/icons"/. "${HOME}/.local/share/icons"/ 2>/dev/null
    rm -rf "${XDG_DATA_HOME}/icons"
  fi
  [ -L "${XDG_DATA_HOME}/icons" ] || ln -s "${HOME}/.local/share/icons" "${XDG_DATA_HOME}/icons"
  [ -L "${XDG_DATA_HOME}/applications" ] && rm -f "${XDG_DATA_HOME}/applications"
  mkdir -p "${XDG_DATA_HOME}/applications"
fi

exec zypak-wrapper /app/lib/helium/helium.real --class=net.imput.helium "$@" --no-default-browser-check
