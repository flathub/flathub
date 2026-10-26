#!/usr/bin/env bash
# Regenerates gradle-sources.json for the tag/commit pinned in the manifest.
# Needs git, python3 and a JDK 25 with jmods (JAVA_HOME, default /usr/lib/jvm/java-25-openjdk;
# on Fedora: java-25-openjdk-devel java-25-openjdk-jmods).
# SKIP_VERIFY=1 skips re-downloading every URL to check its hash.
set -euo pipefail

MANIFEST=com.abdownloadmanager.abdownloadmanager.yml
OUT="$PWD/gradle-sources.json"
PLUGIN_VERSION="${CAPTURE_PLUGIN_VERSION:-0.2.2}"
export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-25-openjdk}"

die() { echo "error: $*" >&2; exit 1; }

for tool in git python3; do command -v "$tool" >/dev/null || die "$tool not found"; done
"$JAVA_HOME/bin/javac" -version 2>&1 | grep -q '^javac 25' || die "no JDK 25 at JAVA_HOME=$JAVA_HOME"
[ -d "$JAVA_HOME/jmods" ] || die "$JAVA_HOME has no jmods (needed by jlink)"

IFS='|' read -r TAG COMMIT < <(awk '
  /url: https:\/\/github\.com\/amir1376\/ab-download-manager\.git$/ { f=1; next }
  f && /^ *tag:/ { t=$2 }
  f && /^ *commit:/ { c=$2 }
  f && /^ *- / { exit }
  END { print t "|" c }' "$MANIFEST")
[[ $TAG && $COMMIT =~ ^[0-9a-f]{40}$ ]] || die "could not read the ab-download-manager tag/commit from $MANIFEST"

WORKDIR="$(mktemp -d -t abdm-sources-XXXXXX)"
trap 'rm -rf "$WORKDIR"' EXIT
export GRADLE_USER_HOME="$WORKDIR/gradle-home"

echo "cloning $TAG ($COMMIT)"
git -c advice.detachedHead=false clone -q --depth 1 --branch "$TAG" \
    https://github.com/amir1376/ab-download-manager.git "$WORKDIR/abdm"
[ "$(git -C "$WORKDIR/abdm" rev-parse HEAD)" = "$COMMIT" ] || die "tag $TAG does not point to $COMMIT"

cat > "$WORKDIR/capture.gradle" <<EOF
initscript {
    repositories { gradlePluginPortal() }
    dependencies {
        classpath "org.meshtastic.flatpak.sources.settings:org.meshtastic.flatpak.sources.settings.gradle.plugin:$PLUGIN_VERSION"
    }
}
beforeSettings { it.pluginManager.apply(org.meshtastic.flatpak.sources.FlatpakSourcesSettingsPlugin) }
EOF

cd "$WORKDIR/abdm"
./gradlew --no-daemon --init-script "$WORKDIR/capture.gradle" --no-build-cache --no-configuration-cache \
    --console=plain createReleaseDistributable :captureFlatpakSources

GRADLE_URL="$(sed -n 's/\\//g; s/^distributionUrl=//p' gradle/wrapper/gradle-wrapper.properties)" \
python3 - build/flatpak-sources.json "$OUT" <<'EOF'
import concurrent.futures, functools, glob, hashlib, json, os, sys, urllib.parse, urllib.request

SRC, OUT = sys.argv[1:3]
CACHE = os.path.join(os.environ["GRADLE_USER_HOME"], "caches/modules-2/files-2.1")
REPOS = ("https://repo.maven.apache.org/maven2", "https://dl.google.com/dl/android/maven2")
ARCHES = {"x86_64": "linux-x64", "aarch64": "linux-arm64"}


def open_url(url, method="GET"):
    req = urllib.request.Request(url, method=method, headers={"User-Agent": "abdm-generate-sources"})
    return urllib.request.urlopen(req, timeout=60)


def sha256(stream):
    h = hashlib.sha256()
    while chunk := stream.read(1 << 16):
        h.update(chunk)
    return h.hexdigest()


def sha256_url(url, tries=3):
    for i in range(tries):
        try:
            with open_url(url) as r:
                return sha256(r)
        except Exception:
            if i == tries - 1:
                raise


def exists(url):
    try:
        with open_url(url, "HEAD"):
            return True
    except Exception:
        return False


def base_of(e):
    tail = f"/{e['dest']}/{e['dest-filename']}"
    return e["url"][: -len(tail)] if e["url"].endswith(tail) else None


@functools.cache
def module_files(rel):
    *group, artifact, version = rel.split("/")
    for path in glob.glob(f"{CACHE}/{'.'.join(group)}/{artifact}/{version}/*/*.module"):
        data = json.load(open(path))
        return {f["name"]: f["url"] for v in data.get("variants", []) for f in v.get("files", [])}
    return {}


entries = {}
for e in json.load(open(SRC)):
    e["dest"] = e["dest"].removeprefix("./").removeprefix("offline-repository/")
    entries[e["dest"], e["dest-filename"]] = e
bases = {e["dest"]: b for e in entries.values() if (b := base_of(e))}

# Add files the capture plugin missed but Gradle still downloaded into its cache.
for path in sorted(glob.glob(f"{CACHE}/*/*/*/*/*")):
    group, artifact, version, _, fn = path.split("/")[-5:]
    rel = f"{group.replace('.', '/')}/{artifact}/{version}"
    if (rel, fn) in entries or fn.endswith(("-sources.jar", "-javadoc.jar")):
        continue
    if rel not in bases:
        bases[rel] = next((r for r in REPOS if exists(f"{r}/{rel}/{fn}")), None)
    if not bases[rel]:
        print(f"warning: {rel}/{fn} not found on Maven Central or Google Maven")
        continue
    with open(path, "rb") as f:
        entries[rel, fn] = {"type": "file", "url": f"{bases[rel]}/{rel}/{fn}", "sha256": sha256(f),
                            "dest": rel, "dest-filename": fn}

# Gradle module metadata may publish a file under another path; store it where Gradle will look.
moved = {}
for (rel, fn), e in entries.items():
    base = base_of(e)
    target = base and not fn.endswith((".pom", ".module")) and module_files(rel).get(fn)
    if target and target != fn:
        url = urllib.parse.urljoin(f"{base}/{rel}/", target)
        dest, name = os.path.split(url[len(base) + 1:])
        e = {k: v for k, v in e.items() if k != "mirror-urls"} | {"url": url, "dest": dest, "dest-filename": name}
    moved.setdefault((e["dest"], e["dest-filename"]), e)
entries = {k: e for k, e in moved.items() if "org/meshtastic/flatpak" not in e["url"]}

# Native artifacts (skiko) are only resolved for the host arch; add the other arch's counterpart.
for (rel, fn), e in list(entries.items()):
    arch = next((a for a, c in ARCHES.items() if c in fn), None)
    if not arch:
        continue
    e["only-arches"] = [arch]
    for other, classifier in ARCHES.items():
        swap = lambda s: s.replace(ARCHES[arch], classifier)
        if other != arch and (swap(rel), swap(fn)) not in entries:
            entries[swap(rel), swap(fn)] = {"type": "file", "url": swap(e["url"]), "sha256": sha256_url(swap(e["url"])),
                                            "dest": swap(rel), "dest-filename": swap(fn), "only-arches": [other]}

gradle_url = os.environ["GRADLE_URL"]
entries["wrapper", "gradle-bin.zip"] = {"type": "file", "url": gradle_url, "sha256": sha256_url(gradle_url),
                                        "dest": "wrapper", "dest-filename": "gradle-bin.zip"}

result = sorted(({**e, "dest": f"gradle/{e['dest']}"} for e in entries.values()),
                key=lambda e: (e["dest"], e["dest-filename"]))

if os.environ.get("SKIP_VERIFY") != "1":
    def check(e):
        try:
            return None if sha256_url(e["url"]) == e["sha256"] else f"hash mismatch: {e['url']}"
        except Exception as ex:
            return f"{e['url']}: {ex}"

    print(f"verifying {len(result)} URLs")
    with concurrent.futures.ThreadPoolExecutor(16) as pool:
        problems = [p for p in pool.map(check, result) if p]
    if problems:
        print(*problems, sep="\n")
        sys.exit(f"{len(problems)} problem(s), {OUT} not written")

with open(OUT, "w") as f:
    json.dump(result, f, indent=2)
    f.write("\n")
print(f"wrote {len(result)} entries to {OUT}")
EOF
