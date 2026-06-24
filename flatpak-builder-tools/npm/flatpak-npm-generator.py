#!/usr/bin/env python3

import argparse
import sys
import json
import base64
import binascii
import urllib.request
import urllib.parse
import re
import os

electron_arches = {
    "ia32": "i386",
    "x64": "x86_64",
    "armv7l": "arm",
    "arm64": "aarch64",
}

def isGitUrl(url):
    return url.startswith("github:") or url.startswith("gitlab:") or url.startswith("bitbucket:") or url.startswith("git")

def getPathandCommitInfo(strippedUrl):
    parsedUrl = {}
    parsedUrl["path"] = re.split(r'#[0-9a-fA-F]*', strippedUrl)[0]
    parsedUrl["commit"] = re.findall(r'#[0-9a-fA-F]*', strippedUrl)[0][1:]
    parsedUrl["name"] = "-".join(re.findall(r'[0-9a-zA-Z_-]+',parsedUrl["path"]))
    return parsedUrl;

def parseGitUrl(url):
    if url.startswith("github:"):
        prefixStrippedUrl = re.split("github:", url)[1]
        parsedUrl = getPathandCommitInfo(prefixStrippedUrl)
        parsedUrl["url"] = "https://github.com/" + parsedUrl["path"]
        parsedUrl["sedCommand"] = (
            "sed -r -i 's^\"github:" + parsedUrl["path"] +
            "(#.*)?\"^\"git+file:/var/tmp/build-dir/npm-cache/git/" +
            parsedUrl["name"] + "\\#" + parsedUrl["commit"] +
            "\"^g' package.json")

    elif url.startswith("gitlab:"):
        prefixStrippedUrl = re.split("gitlab:", url)[1]
        parsedUrl = getPathandCommitInfo(prefixStrippedUrl)
        parsedUrl["url"] = "https://gitlab.com/" + parsedUrl["path"]
        parsedUrl["sedCommand"] = (
            "sed -r -i 's^\"gitlab:" + parsedUrl["path"] +
            "(#.*)?\"^\"git+file:/var/tmp/build-dir/npm-cache/git/" +
            parsedUrl["name"] + "\\#" + parsedUrl["commit"] +
            "\"^g' package.json")

    elif url.startswith("bitbucket:"):
        prefixStrippedUrl = re.split("bitbucket:", url)[1]
        parsedUrl = getPathandCommitInfo(prefixStrippedUrl)
        parsedUrl["url"] = "https://bitbucket.org/" + parsedUrl["path"]
        parsedUrl["sedCommand"] = (
            "sed -r -i 's^\"bitbucket:" + parsedUrl["path"] +
            "(#.*)?\"^\"git+file:/var/tmp/build-dir/npm-cache/git/" +
            parsedUrl["name"] + "\\#" + parsedUrl["commit"] +
            "\"^g' package.json")

    elif url.startswith("git://"):
        prefixStrippedUrl = re.split(r'\w+\.\w+\/',url)[1]
        parsedUrl = getPathandCommitInfo(prefixStrippedUrl)
        domain = re.findall(r'\w+\.\w+\/',url)[0]
        parsedUrl["url"] = "git://" + domain + parsedUrl["path"]
        parsedUrl["sedCommand"] = (
            "sed -r -i 's^\"git://" + domain + parsedUrl["path"] +
            "(#.*)?\"^\"git+file:/var/tmp/build-dir/npm-cache/git/" +
            parsedUrl["name"] + "\\#" + parsedUrl["commit"] +
            "\"^g' package.json")

    elif url.startswith("git+https://"):
        prefixStrippedUrl = re.split(r'\w+\.\w+\/',url)[1]
        parsedUrl = getPathandCommitInfo(prefixStrippedUrl)
        domain = re.findall(r'\w+\.\w+\/',url)[0]
        parsedUrl["url"] = "https://" + domain + parsedUrl["path"]
        parsedUrl["sedCommand"] = (
            "sed -r -i 's^\"git+https://" + domain + parsedUrl["path"] +
            "(#.*)?\"^\"git+file:/var/tmp/build-dir/npm-cache/git/" +
            parsedUrl["name"] + "\\#" + parsedUrl["commit"] +
            "\"^g' package.json")

    elif url.startswith("git+http://"):
        prefixStrippedUrl = re.split(r'\w+\.\w+\/',url)[1]
        parsedUrl = getPathandCommitInfo(prefixStrippedUrl)
        domain = re.findall(r'\w+\.\w+\/',url)[0]
        parsedUrl["url"] = "http://" + domain + parsedUrl["path"]
        parsedUrl["sedCommand"] = (
            "sed -r -i 's^\"git+http://" + domain + parsedUrl["path"] +
            "(#.*)?\"^\"git+file:/var/tmp/build-dir/npm-cache/git/" +
            parsedUrl["name"] + "\\#" + parsedUrl["commit"] +
            "\"^g' package.json")

    elif url.startswith("git+ssh://"):
        print("ssh protocol not supported")
        print("Found url is: " + url)
    else:
        print("Unsupported git url type for " + url)

    return parsedUrl

def getModuleSources(module, name, seen=None, include_devel=True, npm3=False):
    sources = []
    patches = []
    seen = seen or {}

    version = module.get("version", "")
    added_url = None

    if module.get("dev", False) and not include_devel:
        pass
    if module.get("bundled", False):
        pass
    elif module.get("resolved", False) or (version.startswith("http") and not version.endswith(".git")):
        if module.get("resolved", False):
            url = module["resolved"]
        else:
            url = module["version"]
        added_url = url
        integrity = module["integrity"]

        integrity_type, integrity_base64 = integrity.split("-", 2)
        hex = binascii.hexlify(base64.b64decode(integrity_base64)).decode('utf8')

        if npm3:
            dest = "npm-cache/" + name + "/" + module["version"] + "/"
            destFilename = "package.tgz"
        else:
            dest = "npm-cache/_cacache/content-v2/%s/%s/%s" % (integrity_type, hex[0:2], hex[2:4])
            destFilename = hex[4:]

        if integrity not in seen:
            seen[integrity] = True
            source = {"type": "file",
                      "url": url,
                      "dest": dest,
                      "dest-filename": destFilename}
            source[integrity_type] = hex
            sources.append(source)
    elif isGitUrl(module["version"]):
        parsedUrl = parseGitUrl(module["version"])
        subdir = "npm-cache/git/" + parsedUrl["name"]
        source = {
            "type": "git",
            "url": parsedUrl["url"],
            "commit": parsedUrl["commit"],
            "dest": subdir
        }
        sources.append(source)
        parsedUrl["sedCommandLock"] = "sed -i 's^" + module["version"] + "^git+file:/var/tmp/build-dir/" + subdir + "#" + parsedUrl["commit"] + "^g' package-lock.json"

        parsedFromUrl = getPathandCommitInfo(module["version"])
        parsedUrl["sedCommandFrom"] = (
            "sed -i 's^\"from\": \"" + parsedFromUrl["path"] +
            "\",^^g' package-lock.json")

        patches.append(parsedUrl["sedCommand"])
        patches.append(parsedUrl["sedCommandLock"])
        patches.append(parsedUrl["sedCommandFrom"])

    if added_url:
        # Special case electron, adding sources for the electron binaries
        tarname = added_url[added_url.rfind("/")+1:]
        if tarname.startswith("electron-") and tarname[len("electron-")].isdigit() and tarname.endswith(".tgz"):
            electron_version = tarname[len("electron-"):-len(".tgz")]

            shasums_url = "https://github.com/electron/electron/releases/download/v" + electron_version + "/SHASUMS256.txt"
            f = urllib.request.urlopen(shasums_url)
            shasums = {}
            shasums_data = f.read().decode("utf8")
            for line in shasums_data.split('\n'):
                l = line.split()
                if len(l) == 2:
                    shasums[l[1][1:]] = l[0]

            mini_shasums = ""
            for arch in electron_arches.keys():
                basename = "electron-v" + electron_version + "-linux-" + arch + ".zip"
                if not basename in shasums:
                      continue
                source = {"type": "file",
                          "only-arches": [electron_arches[arch]],
                          "url": "https://github.com/electron/electron/releases/download/v" + electron_version + "/" + basename,
                          "sha256": shasums[basename],
                          "dest": "npm-cache"}
                sources.append(source)
                mini_shasums = mini_shasums + shasums[basename] + " *" + basename + "\n"
            source = {"type": "file",
                      "url": "data:" + urllib.parse.quote(mini_shasums.encode("utf8")),
                      "dest": "npm-cache",
                      "dest-filename": "SHASUMS256.txt-" + electron_version}
            sources.append(source)

    if "dependencies" in module:
        deps = module["dependencies"]
        for dep in sorted(deps):
            child_sources = getModuleSources(deps[dep], seen, include_devel=include_devel)
            sources += child_sources["sources"]
            patches += child_sources["patches"]

    return {"sources": sources, "patches": patches}

def main():
    parser = argparse.ArgumentParser(description='Flatpak NPM generator')
    parser.add_argument('lockfile', type=str)
    parser.add_argument('-o', type=str, dest='sourcesOutFile', default='generated-sources.json')
    parser.add_argument('--production', action='store_true', default=False)
    parser.add_argument('--recursive', action='store_true', default=False)
    parser.add_argument('--npm3',action='store_true',default=False)
    args = parser.parse_args()

    include_devel = not args.production

    npm3 =args.npm3
    sourcesOutFile = args.sourcesOutFile

    if args.recursive:
        import glob
        lockfiles = glob.iglob('**/%s' % args.lockfile, recursive=True)
    else:
        lockfiles = [args.lockfile]

    sources = []
    patches = []
    seen = {}

    # We add a symlink in /var/tmp with a fixed absolute pathname so we can use it in git file: uris
    sources += [
        {
            "type": "shell",
            "commands": [ "ln -fs `pwd` /var/tmp/build-dir" ]
        }
    ]

    for lockfile in lockfiles:
        print('Scanning "%s" ' % lockfile, file=sys.stderr)

        with open(lockfile, 'r') as f:
            root = json.loads(f.read())

        s = getModuleSources(root, None, seen, include_devel=include_devel, npm3=npm3)
        sources += s["sources"]
        patches += s["patches"]
        print(' ... %d new sources' % len(s["sources"]), file=sys.stderr)

    print('%d total sources' % len(sources), file=sys.stderr)

    if len(patches) > 0:
        sources += [
            {
                "type": "shell",
                "commands": patches
            }
        ]

    print('Writing to "%s"' % sourcesOutFile)
    with open(sourcesOutFile, 'w') as f:
        f.write(json.dumps(sources, indent=4, sort_keys = True))

if __name__ == '__main__':
    main()
