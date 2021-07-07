#!/usr/bin/env python3

import argparse
import getpass
import http.cookiejar
import json
import os
import sys
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET

NOTIFICATIONS_URL = 'https://www.xilinx.com/direct/swhelp/notifications.xml'
AUTHENTICATOR_URL = 'https://www.xilinx.com/bin/public/webinstall/sso/authenticator'
DOWNLOAD_LINK_URL = 'https://xilinx.entitlenow.com/wi/v1/downloadlink'


def get_downloads(filter_fn):
    with urllib.request.urlopen(NOTIFICATIONS_URL) as response:
        raw_xml = response.read()

    root = ET.fromstring(raw_xml)

    notifications = root.iter('notification')
    notifications_version = filter(filter_fn, notifications)
    files = [(notification.get('version'), file) for notification in notifications_version for file in notification.findall('fileToDownlaod')]

    download_info = [
        {
            "version": version,
            "key": file.get('refKey'),
            "size": file.get('size'),
            "md5": file.find('md5Checksum').text,
            "platform": file.find('platform').text
        } for version, file in files]
    return download_info


def get_newsoftware():
    return get_downloads(lambda n: n.get('type') == 'NEW_SW_NOTIFICATION_E')


def get_version(version):
    return get_downloads(lambda n: version in [ver.text for ver in n.iter('appliesToVersion')])


def get_auth_token(username, password):
    params = {'xilinxUserId': username, 'password': password, "encrypted": "true"}
    url = AUTHENTICATOR_URL + "?" + urllib.parse.urlencode(params)

    cj = http.cookiejar.CookieJar()
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))
    opener.open(url, data=b'')
    token_cookie = next(filter(lambda c: c.name == "token", cj))

    return token_cookie.value


def get_download_link(token, filename, platform, version):
    request_content = {
        "files": [filename],
        "metadata": {
            "add_ons": [],
            "devices": [],
            "edition": "",
            "install_type": "Install",
            "number_parallel": 4,
            "platform": platform,
            "product_version": version,
            "products": ""
        },
        "token": token
    }

    request = urllib.request.Request(
        DOWNLOAD_LINK_URL,
        data=json.dumps(request_content).encode('utf-8'),
        headers={'Content-Type': 'application/json'}
    )

    with urllib.request.urlopen(request) as response:
        raw_json = response.read()

    download_link = json.loads(raw_json)["downloads"]["urls"][0]["download_link"]
    return urllib.parse.unquote(download_link)


def download_file_with_progress(url, output_file, version):
    def print_progress(transferred_blocks, block_size, total_size):
        transferred_size = transferred_blocks * block_size
        percentage = min(99, int(transferred_size / total_size * 100))
        transferred_mib = transferred_size / (1024 * 1024)
        total_mib = total_size / (1024 * 1024)

        try:
            print(f"#Downloading Vivado {version} ({transferred_mib:.1f} MiB / {total_mib:.1f} MiB, {percentage} %)\n{percentage}", flush=True)
        except BrokenPipeError:     # The user has cancelled the download, everything is fine
            devnull = os.open(os.devnull, os.O_WRONLY)
            os.dup2(devnull, sys.stdout.fileno())
            sys.exit(0)

    urllib.request.urlretrieve(url, output_file, print_progress)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Find the latest downloadable version of the Xilinx Vivado Design Suite."
                                                 "If a username and password is provided, also get the download link.")
    parser.add_argument("username", nargs='?')
    parser.add_argument("output_file", nargs='?')
    args = parser.parse_args()

    download_info = next(filter(lambda s: s["platform"] == "LIN64", get_newsoftware()))

    if args.username:
        password = getpass.getpass() if sys.stdin.isatty() else sys.stdin.readline().rstrip()
        token = get_auth_token(args.username, password)
        download_link = get_download_link(token, download_info["key"] + ".bin", download_info["platform"], download_info["version"])

        if args.output_file:
            download_file_with_progress(download_link, args.output_file, download_info["version"])
        else:
            print(download_link)
    else:
        print(download_info["version"])
