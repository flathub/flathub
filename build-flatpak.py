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
          '%s-%s.json' % (appid, branch)])


# Full-blown repo build, along with GPG, deltas and upload
def build_release(branch, no_push, appid):
    call(['flatpak-builder',
          '--force-clean',
          '--delete-build-dirs',
          '--ccache',
          '--gpg-sign=%s' % gpg_key,
          '--gpg-homedir=gpg',
          '--repo=repo',
          'appdir-%s' % branch,
          '%s-%s.json' % (appid, branch)])

    call(['flatpak',
          'build-update-repo',
          '--generate-static-deltas',
          '--gpg-sign=%s' % gpg_key,
          '--gpg-homedir=gpg',
          'repo'])

    if no_push:
        return

    os.chdir('repo/')

    if not os.path.isdir('./.git'):
        raise ValueError('not a git repository')

    call(['git',
          'add',
          '--all'])

    call(['git',
          'commit',
          '--message=Add new build'])

    call(['git',
          'lfs',
          'push',
          'origin',
          'master'])

    call(['git',
          'push'])


def run(branch, appid):
    call(['flatpak-builder',
          '--run',
          'appdir-%s' % branch,
          '%s-%s.json' % (appid, branch),
          'scribus'])


def main():
    # Feed the options manager
    usage = "usage: %prog [options] APPID"
    parser = OptionParser(usage=usage)
    parser.add_option('-R', '--run', action='store_true',
                      help="run the app from the app dir")
    parser.add_option('-r', '--release', action='store_true',
                      help="export build to repository")
    parser.add_option('-n', '--no-push', action='store_true',
                      help="don’t push the repo")
    parser.add_option('-b', '--branch', default='master',
                      help=("which branch to use, manifests are expected"
                            " to be stored in BRANCH/APPID.json format"
                            " [default: %default]"))

    (options, args) = parser.parse_args()

    if options.run:
        run(options.branch, args[0])
        return

    # Check if APPID is supplied
    args_len = len(args)
    if args_len < 1:
        parser.error("APPID must be specified")

    # Complain if more APPIDs are supplied
    elif args_len > 1:
        parser.error("building more than one app is unsupported")

    # Check if in correct directory
    if not os.path.isdir('./.flatpak-builder'):
        print(("WARNING: the .flatpak-builder direcory was not found,"
               " are you sure you are launching the script from"
               " the correct location?"))

    # Build according to chosen mode
    if not options.release:
        build_debug(options.branch, args[0])
    else:
        build_release(options.branch, options.no_push, args[0])


main()
