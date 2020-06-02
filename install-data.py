#!/usr/bin/env python3
import os
import sys
import subprocess
import json
import datetime
from xml.dom import minidom

FLATPAK_ID = os.environ.get('FLATPAK_ID')

EXTRA_DESCRIPTION = [
"""This is the Open Source build of Visual Studio Code, packaged into a Flatpak. Some features are
different from the proprietary version: There is no telemetry nor Twitter integration, and the
logo is a different one without copyright issue. This OSS repackaging, as well as the proprietary
repackaging in Flathub, are not supported by Microsoft.""",
"""This OSS build is created due to the proprietarily licensed official binary. For more information,
see https://github.com/flathub/com.visualstudio.code.oss/issues/6#issuecomment-380152999."""
]

OARS_DATA = {
    'social-chat': 'intense',
    'social-info': 'intense',
    'social-audio': 'intense',
    'social-contacts': 'intense',
}

def configure_file(product, input_data):
    replacements = {
        'NAME': product['applicationName'],
        'NAME_SHORT': product['nameShort'],
        'NAME_LONG': product['nameLong'],
        'EXEC': product['applicationName'],
        'ICON': product['applicationName'],
        'URLPROTOCOL': product['urlProtocol'],
        'LICENSE': product['licenseName'],
    }
    output = input_data
    for key, value in replacements.items():
        placeholder = f'@@{key}@@'
        output = output.replace(placeholder, value)
    return output

def install_file(product, src_file, dst_file):
    with open(src_file, 'r') as i:
        os.makedirs(os.path.dirname(dst_file), exist_ok=True)
        with open(dst_file, 'w') as o:
            o.write(configure_file(product, i.read()))

def update_appdata(srcdir, appdata_path):
    appdata = minidom.parse(appdata_path)
    component = appdata.documentElement

    with open(os.path.join(srcdir, 'package.json'), 'r') as p:
        package = json.load(p)

    version = package['version']
    git_timestamp = subprocess.run(['git', 'show', '-s', '--format=%ct'],
                                  cwd=srcdir, check=True, text=True,
                                  stdout=subprocess.PIPE).stdout
    date = datetime.datetime.fromtimestamp(int(git_timestamp)).date().isoformat()

    releases = appdata.createElement('releases')
    release = appdata.createElement('release')
    release.setAttribute('version', version)
    release.setAttribute('date', date)
    releases.appendChild(release)
    component.appendChild(releases)

    content_rating = appdata.createElement('content_rating')
    content_rating.setAttribute('type', 'oars-1.1')
    for attr, value in OARS_DATA.items():
        content_attribute = appdata.createElement('content_attribute')
        content_attribute.setAttribute('id', attr)
        content_attribute.appendChild(appdata.createTextNode(value))
        content_rating.appendChild(content_attribute)
    component.appendChild(content_rating)

    description = component.getElementsByTagName('description')[0]
    for descr in EXTRA_DESCRIPTION:
        paragraph = appdata.createElement('p')
        paragraph.appendChild(appdata.createTextNode(descr))
        description.appendChild(paragraph)

    with open(appdata_path, 'w') as o:
        o.write(appdata.toxml(encoding='UTF-8').decode())

def install_desktop_data(srcdir, datadir):
    with open(os.path.join(srcdir, 'product.json'), 'r') as p:
        product = json.load(p)
    appname = product['applicationName']

    for s in [16, 24, 32, 48, 64, 128, 192, 256, 512]:
        size = f'{s}x{s}'
        dest_file = os.path.join(datadir, 'icons', 'hicolor', size, 'apps', f'{appname}.png')
        os.makedirs(os.path.dirname(dest_file), exist_ok=True)
        subprocess.run([
            'magick', 'convert', os.path.join(srcdir, 'resources', 'linux', 'code.png'),
            '-resize', size, dest_file
        ], check=True)

    install_file(product,
                 os.path.join(srcdir, 'resources', 'linux', 'code.desktop'),
                 os.path.join(datadir, 'applications', f'{appname}.desktop'))
    install_file(product,
                 os.path.join(srcdir, 'resources', 'linux', 'code-url-handler.desktop'),
                 os.path.join(datadir, 'applications', f'{FLATPAK_ID}-url-handler.desktop'))
    appdata_path = os.path.join(datadir, 'appdata', f'{appname}.appdata.xml')
    install_file(product,
                 os.path.join(srcdir, 'resources', 'linux', 'code.appdata.xml'),
                 appdata_path)
    update_appdata(srcdir, appdata_path)

def main():
    srcdir = sys.argv[1]
    datadir = sys.argv[2]
    install_desktop_data(srcdir, datadir)

if __name__ == '__main__':
    main()