#!/usr/bin/env python3

# Script to generate release appdata from davmail release notes

import re
import os
import sys
import copy
import html
import subprocess

def gather_releases(filename):
    # dictionary with release and text
    # (worry about date later)
    releases = []

    current_release = {}

    for line in open(filename).readlines():
        matches = re.search("\*\* DavMail (.*) released (.*)\*\*", line, re.IGNORECASE)

        if matches:
            # Got a new release
            releases.append(copy.deepcopy(current_release))
            current_release = {} # empty, better way?
            current_release['version'] = matches.group(1)
            current_release['text'] = ''
        else:
            current_release['text'] += line

    # push everything gathered so far
    releases.append(copy.deepcopy(current_release))

    # Remove empty placeholder at the beginning
    releases.pop(0)

    return releases

def run_process(args):
    process = subprocess.Popen(args, stdout=subprocess.PIPE)
    outstr, errstr = process.communicate()
    return process.returncode, outstr.decode('utf-8').strip()

def get_date(release_tag):
    command = "git log -n1 --date=short " + str(release_tag)
    (code, output) = run_process(command.split())

    date = "UNKNOWN"

    for line in output.split('\n'):
        matches = re.search("Date:\s+(.+)", line)
        if matches:
            date = matches.group(1)
            break

    return date

def populate_dates(releases):
    for release in releases:
        release['date'] = get_date(release['version'])

    return releases

def format_xml(releases):
    result = ''
    for release in releases:
        result += """    <release version="%s" date="%s">
      <description>
        <p>
%s
        </p>
      </description>
    </release>
""" % (release['version'], release['date'].strip(), html.escape(release['text'].strip()))

    return result

try:
    (script, davmail_git_repo) = sys.argv
except:
    print("Usage: <generate-appdata-releases.py <davmail git repo>")
    sys.exit(1)

releases = gather_releases(davmail_git_repo + os.sep + 'releasenotes.txt')
current_dir = os.getcwd()
os.chdir(davmail_git_repo)
releases = populate_dates(releases)
os.chdir(current_dir)

xml = format_xml(releases)

print(xml)
