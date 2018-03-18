import os
import sys
import json
import subprocess
import shutil
from pathlib import Path
from xml.dom import minidom
from contextlib import contextmanager
import urllib.request
import urllib.parse
import tempfile
import re
import hashlib
import stat

PRODUCT = json.loads(Path('vscode/product.json').read_text())
PACKAGE = json.loads(Path('vscode/package.json').read_text())
RECIPE = json.loads(Path(os.environ['FLATPAK_ID'] + '.json').read_text())
ARCH = ' '.join(subprocess.run(['node', '-e', 'console.log(process.arch)'], stdout=subprocess.PIPE, universal_newlines=True).stdout.split())

sha256sums = {}
for package in [source for source in next(
    module for module in RECIPE['modules'] if module['name'] == 'vscode'
)['sources'] if source.get('dest') == '.electron']:
    if package['@comment']['version'] not in sha256sums:
        sha256sums[package['@comment']['version']] = {}
    sha256sums[package['@comment']['version']][package['dest-filename']] = package['sha256']
for version in sha256sums:
    Path('.electron/SHASUMS256.txt-' + version).write_text('\n'.join(
        sha256sums[version][filename] + ' *' + filename for filename in sha256sums[version])
    )

shutil.move('gulp-electron-cache', '/tmp')
shutil.move('.electron', str(Path.home()))
shutil.move('bin/yarn.js', '/app/local/bin')
Path('/app/local/bin/yarn.js').chmod(Path('/app/local/bin/yarn.js').stat().st_mode | stat.S_IXUSR)
Path('/app/local/bin/yarn').symlink_to('yarn.js')
subprocess.run(['yarn', 'config', 'set', 'yarn-offline-mirror', str(Path('yarn-mirror').resolve())], check=True)

shutil.unpack_archive(str(next(Path('yarn-mirror').glob('vscode-ripgrep-*'))))
shutil.move('package', 'vscode-ripgrep')
subprocess.run(['yarn', 'link'], check=True, cwd='vscode-ripgrep')
Path('vscode-ripgrep/bin').mkdir()
shutil.unpack_archive('misc/ripgrep.zip', 'vscode-ripgrep/bin')
Path('vscode-ripgrep/bin/rg').chmod(Path('vscode-ripgrep/bin/rg').stat().st_mode | stat.S_IXUSR)

os.chdir('vscode')
Path('build/builtInExtensions.json').write_text('[]')
subprocess.run(['yarn', 'link', 'vscode-ripgrep'], check=True)
package_vscode_extension = json.loads(Path('extensions/vscode-colorize-tests/package.json').read_text())
del package_vscode_extension['scripts']['postinstall']
Path('extensions/vscode-colorize-tests/package.json').write_text(json.dumps(package_vscode_extension))

subprocess.run(['yarn', 'install', '--offline', '--verbose', '--frozen-lockfile'], check=True, env={
    **os.environ,
    'npm_config_tarball': str(Path('../misc/iojs.tar.gz').resolve()),
})

Path('node_modules/vscode-ripgrep').unlink()
Path('../vscode-ripgrep').rename('node_modules/vscode-ripgrep')
shutil.copy('src/vs/vscode.d.ts', 'extensions/vscode-colorize-tests/node_modules/vscode')
subprocess.run(['node_modules/.bin/gulp', 'vscode-linux-' + ARCH + '-min', '--max_old_space_size=4096'], check=True)

os.chdir('..')
shutil.move('VSCode-linux-' + ARCH, '/app/share/' + PRODUCT['applicationName'])
os.symlink('../share/' + PRODUCT['applicationName'] + '/bin/' + PRODUCT['applicationName'], '/app/bin/' + PRODUCT['applicationName'])
Path('/app/share/icons/hicolor/1024x1024/apps').mkdir(parents=True)
shutil.copy('vscode/resources/linux/code.png', '/app/share/icons/hicolor/1024x1024/apps/' + os.environ['FLATPAK_ID'] + '.png')
for size in [16, 24, 32, 48, 64, 128, 192, 256, 512]:
    size = str(size)
    Path('/app/share/icons/hicolor/' + size + 'x' + size + '/apps').mkdir(parents=True)
    Path('/app/share/icons/hicolor/' + size + 'x' + size + '/apps/' + os.environ['FLATPAK_ID'] + '.png').write_bytes(subprocess.run([
        'magick',
        'convert',
        'vscode/resources/linux/code.png',
        '-resize',
        size + 'x' + size,
        '-'
    ], check=True, stdout=subprocess.PIPE).stdout)

Path('/app/share/applications').mkdir(parents=True)
Path('/app/share/applications/' + os.environ['FLATPAK_ID'] + '.desktop').write_text(
    Path('vscode/resources/linux/code.desktop')
    .read_text()
    .replace('Exec=/usr/share/@@NAME@@/@@NAME@@', 'Exec=' + PRODUCT['applicationName'])
    .replace('@@NAME_LONG@@', PRODUCT['nameLong'])
    .replace('@@NAME_SHORT@@', PRODUCT['nameShort'])
    .replace('@@NAME@@', os.environ['FLATPAK_ID'])
    .replace('@@ICON@@', os.environ['FLATPAK_ID'])
)

dom = minidom.parse('vscode/resources/linux/code.appdata.xml')


def remove_white(node):
    if node.nodeType == minidom.Node.TEXT_NODE and node.data.strip() == '':
        node.data = ''
    else:
        list(map(remove_white, node.childNodes))


remove_white(dom)
releases = dom.createElement('releases')
env = {
    **os.environ,
    'TZ': 'UTC',
    'GIT_DIR': 'vscode/.git'
}
for entry in RECIPE['@comments']['releases']:
    release = dom.createElement('release')
    release.setAttribute('version', entry['version'])
    release.setAttribute('date', entry['date'])
    releases.appendChild(release)
dom.getElementsByTagName('component')[0].appendChild(releases)
lines = dom.toxml(encoding='UTF-8').decode()
Path('/app/share/appdata').mkdir(parents=True)
Path('/app/share/appdata/' + os.environ['FLATPAK_ID'] + '.appdata.xml').write_text(
    lines
    .replace('@@NAME_LONG@@', PRODUCT['nameLong'])
    .replace('@@NAME@@', os.environ['FLATPAK_ID'])
    .replace('@@LICENSE@@', PRODUCT['licenseName'])
)
