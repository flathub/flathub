#!/bin/python3

from subprocess import call
from optparse import OptionParser
import os

gpg_key = '43998AD0A6FC78823E7BBEFDFCBDD9236E1A33FF'


# Quick ’n’ dirty build mode, suitable for testing
def build_debug(branch, appid):
    call(['flatpak-builder',
          '--force-clean',
          '--ccache',
          'appdir-%s' % branch,
          '%s/%s.json' % (branch, appid)])


# Full-blown repo build, along with GPG and deltas
def build_release(branch, appid):
    call(['flatpak-builder',
          '--force-clean',
          '--ccache',
          '--gpg-sign=%s' % gpg_key,
          '--gpg-homedir=gpg',
          '--repo=repo',
          'appdir-%s' % branch,
          '%s/%s.json' % (branch, appid)])

    call(['flatpak',
          'build-update-repo',
          '--generate-static-deltas',
          '--gpg-sign=%s' % gpg_key,
          '--gpg-homedir=gpg',
          'repo'])


def main():
    # Feed the options manager
    usage = "usage: %prog [options] APPID"
    parser = OptionParser(usage=usage)
    parser.add_option('-r', '--release', action='store_true',
                      help="export build to repository")
    parser.add_option('-b', '--branch', default='stable',
                      help=("which branch to use, manifests are expected"
                            " to be stored in BRANCH/APPID.json format"
                            " [default: %default]"))

    (options, args) = parser.parse_args()

    # Check if APPID is supplied
    args_len = len(args)
    if args_len < 1:
        parser.error("APPID must be specified")

    # Complain if more APPIDs are supplied
    elif args_len > 1:
        parser.error("building more than one app is unsupported")

    # Clean .flatpak-builder to avoid running out of disk space
    if not os.path.isdir('./.flatpak-builder'):
        print(("WARNING: the .flatpak-builder direcory was not found,"
               " are you sure you are launching the script from"
               " the correct location?"))

    elif os.path.isdir('./.flatpak-builder/build'):
        print("Emptying build dir")
        call(['rm', '-rf', './.flatpak-builder/build'])

    # Build according to chosen mode
    if not options.release:
        build_debug(options.branch, args[0])
    else:
        build_release(options.branch, args[0])

main()
