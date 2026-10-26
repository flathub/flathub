#!/usr/bin/env bash
#!/usr/bin/env bash

set -euo pipefail

CAPTURE_PLUGIN_VERSION="${CAPTURE_PLUGIN_VERSION:-0.2.2}"

CUR_DIR="$PWD"
SOURCE_URL="https://github.com/amir1376/ab-download-manager"


COMMIT=$(awk '
  /url: https:\/\/github\.com\/amir1376\/ab-download-manager\.git$/ { found=1; next }
  found && /^[[:space:]]*commit:/ { print $2; exit }
  found && /^[[:space:]]*- / { exit }
' "$CUR_DIR/com.abdownloadmanager.abdownloadmanager.yml")

WORKDIR="$(mktemp -d -t abdm-gradle-sources-XXXXXX)"

cd "$WORKDIR"

export GRADLE_USER_HOME="$WORKDIR/gradle-home"

mkdir -p "$GRADLE_USER_HOME"

echo "writing helping scripts"

cat > "$WORKDIR/capture-init.gradle" <<'EOF'
initscript {
    repositories { gradlePluginPortal() }
    dependencies {
        classpath "org.meshtastic.flatpak.sources.settings:org.meshtastic.flatpak.sources.settings.gradle.plugin:@@VERSION@@"
    }
}

def pluginClassName = null
initscript.configurations.classpath.files.each { f ->
    if (f.name.endsWith(".jar")) {
        def zip = new java.util.zip.ZipFile(f)
        try {
            def e = zip.getEntry("META-INF/gradle-plugins/org.meshtastic.flatpak.sources.settings.properties")
            if (e != null) {
                def p = new Properties(); p.load(zip.getInputStream(e))
                pluginClassName = p.getProperty("implementation-class")
            }
        } finally { zip.close() }
    }
}
println "capture plugin class: ${pluginClassName}"
def pluginClass = this.class.classLoader.loadClass(pluginClassName)

beforeSettings { settings ->
    settings.pluginManager.apply(pluginClass)
}
EOF

sed -i "s/@@VERSION@@/$CAPTURE_PLUGIN_VERSION/" "$WORKDIR/capture-init.gradle"

cat > "$WORKDIR/postprocess.py" <<'EOF'
#!/usr/bin/env python3
import collections
import concurrent.futures
import glob
import hashlib
import json
import os
import sys
import urllib.parse
import urllib.request

SRC, OUT, NEW_PREFIX = sys.argv[1:4]
OLD_PREFIX = "offline-repository"
CACHE = os.path.join(os.environ["GRADLE_USER_HOME"], "caches", "modules-2", "files-2.1")
GOOGLE = "https://dl.google.com/dl/android/maven2"
CENTRAL = "https://repo.maven.apache.org/maven2"
EXCLUDE = ("org/meshtastic/flatpak",)
UA = {"User-Agent": "abdm-gen-gradle-sources"}


def norm(dest):
    if dest.startswith("./"):
        dest = dest[2:]
    if dest.startswith(OLD_PREFIX + "/"):
        dest = dest[len(OLD_PREFIX) + 1:]
    return dest


def exists(url):
    try:
        req = urllib.request.Request(url, method="HEAD", headers=UA)
        return urllib.request.urlopen(req, timeout=20).status == 200
    except Exception:
        return False


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


entries = json.load(open(SRC))
for e in entries:
    e["dest"] = norm(e["dest"])
print(f"captured entries: {len(entries)}")


have, bases = set(), {}
for e in entries:
    rel, fn = e["dest"], e["dest-filename"]
    have.add((rel, fn))
    tail = "/" + rel + "/" + fn
    if e["url"].endswith(tail):
        bases[rel] = e["url"][: -len(tail)]

probed = {}


def find_base(rel, fn):
    if rel in probed:
        return probed[rel]
    google_first = rel.startswith(("androidx/", "com/android/", "com/google/android/"))
    for base in ([GOOGLE, CENTRAL] if google_first else [CENTRAL, GOOGLE]):
        if exists(f"{base}/{rel}/{fn}"):
            probed[rel] = base
            return base
    probed[rel] = None
    return None


added, unresolved = [], []
if os.path.isdir(CACHE):
    for group in sorted(os.listdir(CACHE)):
        for artifact in os.listdir(os.path.join(CACHE, group)):
            for version in os.listdir(os.path.join(CACHE, group, artifact)):
                rel = f"{group.replace('.', '/')}/{artifact}/{version}"
                vdir = os.path.join(CACHE, group, artifact, version)
                for h in os.listdir(vdir):
                    for fn in os.listdir(os.path.join(vdir, h)):
                        if fn.endswith(("-sources.jar", "-javadoc.jar")) or (rel, fn) in have:
                            continue
                        base = bases.get(rel) or find_base(rel, fn)
                        if base is None:
                            unresolved.append(f"{rel}/{fn}")
                            continue
                        added.append({
                            "type": "file",
                            "url": f"{base}/{rel}/{fn}",
                            "sha256": sha256_file(os.path.join(vdir, h, fn)),
                            "dest": rel,
                            "dest-filename": fn,
                        })
                        have.add((rel, fn))
else:
    print(f"warning: Gradle cache not found at {CACHE}; skipping fill")
entries += added
print(f"added from cache: {len(added)}")
for u in unresolved:
    print(f"  unresolved (not on Google Maven or Central): {u}")


maps = {}


def name_to_url(rel, base):
    if rel not in maps:
        parts = rel.split("/")
        version, artifact, group = parts[-1], parts[-2], ".".join(parts[:-2])
        data = None
        for m in glob.glob(os.path.join(CACHE, group, artifact, version, "*", "*.module")):
            try:
                data = json.load(open(m))
                break
            except Exception:
                pass
        if data is None:
            try:
                req = urllib.request.Request(f"{base}/{rel}/{artifact}-{version}.module", headers=UA)
                data = json.load(urllib.request.urlopen(req, timeout=30))
            except Exception:
                data = None
        mapping = {}
        for variant in (data or {}).get("variants", []):
            for f in variant.get("files", []):
                if "name" in f and "url" in f:
                    mapping[f["name"]] = f["url"]
        maps[rel] = mapping
    return maps[rel]


renamed, seen, result = 0, set(), []
for e in entries:
    fn = e["dest-filename"]
    if not fn.endswith((".pom", ".module")):
        rel = e["dest"]
        tail = "/" + rel + "/" + fn
        if e["url"].endswith(tail):
            base = e["url"][: -len(tail)]
            target = name_to_url(rel, base).get(fn)
            if target and target != fn:
                new_url = urllib.parse.urljoin(f"{base}/{rel}/", target)
                new_dir, new_name = os.path.split(new_url[len(base) + 1:])
                e = dict(e, url=new_url, dest=new_dir)
                e["dest-filename"] = new_name
                e.pop("mirror-urls", None)  # stale after a rename
                renamed += 1
    key = (e["dest"], e["dest-filename"])
    if key not in seen:
        seen.add(key)
        result.append(e)
entries = result
print(f"renamed: {renamed}")


before = len(entries)
entries = [e for e in entries if not any(x in e["url"] for x in EXCLUDE)]
print(f"dropped build-only artifacts: {before - len(entries)}")


for e in entries:
    e["dest"] = f"{NEW_PREFIX}/{e['dest']}"
with open(OUT, "w") as f:
    json.dump(entries, f, indent=2)

counts = collections.Counter(os.path.splitext(e["dest-filename"])[1].lstrip(".") for e in entries)
print(f"final entries: {len(entries)}  ({', '.join(f'{k}={v}' for k, v in counts.most_common(5))})")


if os.environ.get("SKIP_VERIFY") == "1":
    print("verification skipped (SKIP_VERIFY=1)")
    sys.exit(0)


def check(e):
    if not e.get("sha256"):
        return f"NO SHA256 {e['url']}"
    err = ""
    for _ in range(3):
        try:
            h = hashlib.sha256()
            req = urllib.request.Request(e["url"], headers=UA)
            with urllib.request.urlopen(req, timeout=60) as r:
                for chunk in iter(lambda: r.read(1 << 16), b""):
                    h.update(chunk)
            return None if h.hexdigest() == e["sha256"] else f"MISMATCH {e['url']}"
        except Exception as ex:
            err = f"ERROR {e['url']} ({ex})"
    return err


print(f"verifying {len(entries)} URLs ...")
with concurrent.futures.ThreadPoolExecutor(16) as pool:
    problems = [p for p in pool.map(check, entries) if p]
for p in problems:
    print("  " + p)
if problems:
    print(f"{len(problems)} problem(s); not copying the result back")
    sys.exit(1)
print("all URLs resolve and all hashes match")
EOF


echo "cloning/starting"

git clone $SOURCE_URL abdm

git -C abdm checkout "$COMMIT"


echo "building"

cd "abdm"
./gradlew --init-script "$WORKDIR/capture-init.gradle" --no-build-cache --no-configuration-cache --console=plain createReleaseDistributable :captureFlatpakSources



echo "post processing"
python3 "$WORKDIR/postprocess.py" "build/flatpak-sources.json" "$WORKDIR/gradle-sources.json" "gradle"

echo "getting gradle"

GRADLE_URL=$(grep -oPm1 '(https\\:\/\/services\.gradle\.org.+)' gradle/wrapper/gradle-wrapper.properties | sed 's/\\//g')
GRADLE_SHA=$(curl -fsSL "$GRADLE_URL" | sha256sum | cut -d ' ' -f 1)

jq --arg url "$GRADLE_URL" --arg sha "$GRADLE_SHA" \
  '. += [{
    "type": "file",
    "url": $url,
    "sha256": $sha,
    "dest": "gradle/wrapper",
    "dest-filename": "gradle-bin.zip"
  }]' "$WORKDIR/gradle-sources.json" > "$CUR_DIR/gradle-sources.json"
