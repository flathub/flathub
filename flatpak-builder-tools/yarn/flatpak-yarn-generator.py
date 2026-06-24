#!/usr/bin/env python3

__license__ = "MIT"

import argparse
import sys
import json
import re
import urllib.request
import urllib.parse
import hashlib 

electron_arches = {
    "armv7l": "arm",
    "arm64": "aarch64",
    "ia32": "i386",
    "x64": "x86_64"
}

def getModuleSources(lockfile, include_devel=True):
    sources = []
    currentSource = ''
    currentSourceVersion = ''
    yarnVersion = ''
    for line in lockfile:
        if '# yarn lockfile' in line:
           yarnVersion = re.split('# yarn lockfile ', line)[1].strip('\n')
        if line.endswith(':\n') and 'dependencies' not in line and 'optionalDependencies' not in line:
            listOfNames = re.split(',', line[:-1])
            currentSource = re.split(r'\@[\^\>\=\<\~]*[\d\s\*]', listOfNames[0])[0]
            currentSource = currentSource.strip('"').replace('/','-')
        if 'version ' in line and currentSource:
            currentSourceVersion = re.split('version ', line)[1].strip('\n').strip('"')
        if 'resolved ' in line and currentSource and currentSourceVersion:
            if currentSource == 'electron':
                shasums_url = "https://github.com/electron/electron/releases/download/v" + currentSourceVersion + "/SHASUMS256.txt"
                f = urllib.request.urlopen(shasums_url)
                shasums = {}
                shasums_data = f.read().decode("utf8")
                for cksumline in shasums_data.split('\n'):
                    l = cksumline.split()
                    if len(l) == 2:
                        shasums[l[1][1:]] = l[0]
                
                mini_shasums = ""
                for arch in electron_arches.keys():
                    zipName = "electron-v" + currentSourceVersion + "-linux-" + arch + ".zip"
                    source = {'type': 'file',
                        'url': 'https://github.com/electron/electron/releases/download/v' + currentSourceVersion + '/' + zipName,
                        'sha256': shasums[zipName],
                        'dest': 'electron-cache',
                        'only-arches': [electron_arches[arch]],
                        'dest-filename': currentSource + '-v' + currentSourceVersion + '-linux-' + arch + '.zip'}
                    sources.append(source)
                    mini_shasums = mini_shasums + shasums[zipName] + " *" + zipName + "\n"
                source = {"type": "file",
                    "url": "data:" + urllib.parse.quote(mini_shasums.encode("utf8")),
                    "dest": "electron-cache",
                    "dest-filename": "SHASUMS256.txt-" + currentSourceVersion}
                sources.append(source)
            
            resolvedStrippedStr = re.split('resolved ', line)[1].strip('\n').strip('"')
            tempList = re.split('#', resolvedStrippedStr)
            if len(tempList) == 1:
                filename = re.split('/', tempList[0])[-1].strip('\n')
                shasum = hashlib.sha1()
                with urllib.request.urlopen(tempList[0]) as f:
                    buf = f.read()
                    shasum.update(buf)
                tempList.append(shasum.hexdigest())
                source = {'type': 'file',
                      'url': tempList[0],
                      'sha1': tempList[1],
                      'dest': 'yarn-mirror',
                      'dest-filename': filename}
            else:
                source = {'type': 'file',
                        'url': tempList[0],
                        'sha1': tempList[1],
                        'dest': 'yarn-mirror',
                        'dest-filename': currentSource + '-' + currentSourceVersion + '.tgz'}
            currentSource = ''
            sources.append(source)
    
    return sources

def main():
    parser = argparse.ArgumentParser(description='Flatpak Yarn generator')
    parser.add_argument('lockfile', type=str)
    parser.add_argument('-o', type=str, dest='outfile', default='generated-sources.json')
    parser.add_argument('--production', action='store_true', default=False)
    parser.add_argument('--recursive', action='store_true', default=False)
    args = parser.parse_args()

    include_devel = not args.production

    outfile = args.outfile

    if args.recursive:
        import glob
        lockfiles = glob.iglob('**/%s' % args.lockfile, recursive=True)
    else:
        lockfiles = [args.lockfile]

    sources = []
    for lockfile in lockfiles:
        print('Scanning "%s" ' % lockfile, file=sys.stderr)

        with open(lockfile, 'r') as f:
            s = getModuleSources(f ,include_devel=include_devel)
            sources += s

        print(' ... %d new entries' % len(s), file=sys.stderr)

    sources = remove_duplicates(sources)
    print('%d total entries after removing duplicates' % len(sources), file=sys.stderr)

    print('Writing to "%s"' % outfile)
    with open(outfile, 'w') as f:
        f.write(json.dumps(sources, indent=4))

def remove_duplicates(items):
    new_list = []
    for obj in items:
        if obj not in new_list:
            new_list.append(obj)
    return new_list


if __name__ == '__main__':
    main()
