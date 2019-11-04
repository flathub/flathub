# -*- Mode: Python -*-
# GObject-Introspection - a framework for introspecting GObject libraries
# Copyright (C) 2008  Johan Dahlin
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import errno
import re
import os
import subprocess
import platform
import shutil
import time


_debugflags = None


def have_debug_flag(flag):
    """Check for whether a specific debugging feature is enabled.
Well-known flags:
 * start: Drop into debugger just after processing arguments
 * exception: Drop into debugger on fatalexception
 * warning: Drop into debugger on warning
 * posttrans: Drop into debugger just before introspectable pass
"""
    global _debugflags
    if _debugflags is None:
        _debugflags = os.environ.get('GI_SCANNER_DEBUG', '').split(',')
        if '' in _debugflags:
            _debugflags.remove('')
    return flag in _debugflags


def break_on_debug_flag(flag):
    if have_debug_flag(flag):
        import pdb
        pdb.set_trace()


# Copied from h2defs.py
_upperstr_pat1 = re.compile(r'([^A-Z])([A-Z])')
_upperstr_pat2 = re.compile(r'([A-Z][A-Z])([A-Z][0-9a-z])')
_upperstr_pat3 = re.compile(r'^([A-Z])([A-Z])')


def to_underscores(name):
    """Converts a typename to the equivalent underscores name.
    This is used to form the type conversion macros and enum/flag
    name variables.
    In particular, and differently from to_underscores_noprefix(),
    this function treats the first character differently if it is
    uppercase and followed by another uppercase letter."""
    name = _upperstr_pat1.sub(r'\1_\2', name)
    name = _upperstr_pat2.sub(r'\1_\2', name)
    name = _upperstr_pat3.sub(r'\1_\2', name, count=1)
    return name


def to_underscores_noprefix(name):
    """Like to_underscores, but designed for "unprefixed" names.
    to_underscores("DBusFoo") => dbus_foo, not d_bus_foo."""
    name = _upperstr_pat1.sub(r'\1_\2', name)
    name = _upperstr_pat2.sub(r'\1_\2', name)
    return name


_libtool_pat = re.compile("dlname='([A-z0-9\\.\\-\\+]+)'\n")


def _extract_dlname_field(la_file):
    with open(la_file) as f:
        data = f.read()
    m = _libtool_pat.search(data)
    if m:
        return m.groups()[0]
    else:
        return None


_libtool_libdir_pat = re.compile("libdir='([^']+)'")


def _extract_libdir_field(la_file):
    with open(la_file) as f:
        data = f.read()
    m = _libtool_libdir_pat.search(data)
    if m:
        return m.groups()[0]
    else:
        return None


# Returns the name that we would pass to dlopen() the library
# corresponding to this .la file
def extract_libtool_shlib(la_file):
    dlname = _extract_dlname_field(la_file)
    if dlname is None:
        return None

    # Darwin uses absolute paths where possible; since the libtool files never
    # contain absolute paths, use the libdir field
    if platform.system() == 'Darwin':
        dlbasename = os.path.basename(dlname)
        libdir = _extract_libdir_field(la_file)
        if libdir is None:
            return dlbasename
        return libdir + '/' + dlbasename
    # From the comments in extract_libtool(), older libtools had
    # a path rather than the raw dlname
    return os.path.basename(dlname)


def extract_libtool(la_file):
    dlname = _extract_dlname_field(la_file)
    if dlname is None:
        raise ValueError("%s has no dlname. Not a shared library?" % la_file)
    libname = os.path.join(os.path.dirname(la_file),
                           '.libs', dlname)
    # FIXME: This hackish, but I'm not sure how to do this
    #        in a way which is compatible with both libtool 2.2
    #        and pre-2.2. Johan 2008-10-21
    libname = libname.replace('.libs/.libs', '.libs').replace('.libs\\.libs', '.libs')
    return libname


# Returns arguments for invoking libtool, if applicable, otherwise None
def get_libtool_command(options):
    libtool_infection = not options.nolibtool
    if not libtool_infection:
        return None

    libtool_path = options.libtool_path
    if libtool_path:
        # Automake by default sets:
        # LIBTOOL = $(SHELL) $(top_builddir)/libtool
        # To be strictly correct we would have to parse shell.  For now
        # we simply split().
        return libtool_path.split(' ')

    libtool_cmd = 'libtool'
    if platform.system() == 'Darwin':
        # libtool on OS X is a completely different program written by Apple
        libtool_cmd = 'glibtool'
    try:
        subprocess.check_call([libtool_cmd, '--version'],
                              stdout=open(os.devnull, 'w'))
    except (subprocess.CalledProcessError, OSError):
        # If libtool's not installed, assume we don't need it
        return None

    return [libtool_cmd]


def files_are_identical(path1, path2):
    with open(path1, 'rb') as f1, open(path2, 'rb') as f2:
        buf1 = f1.read(8192)
        buf2 = f2.read(8192)
        while buf1 == buf2 and buf1 != b'':
            buf1 = f1.read(8192)
            buf2 = f2.read(8192)
        return buf1 == buf2


def cflag_real_include_path(cflag):
    if not cflag.startswith("-I"):
        return cflag

    return "-I" + os.path.realpath(cflag[2:])


def host_os():
    return os.environ.get("GI_HOST_OS", os.name)


def which(program):
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    def is_nt_exe(fpath):
        return not fpath.lower().endswith('.exe') and \
            os.path.isfile(fpath + '.exe') and \
            os.access(fpath + '.exe', os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
        if os.name == 'nt' and is_nt_exe(program):
            return program + '.exe'
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
            if os.name == 'nt' and is_nt_exe(exe_file):
                return exe_file + '.exe'

    return None


def makedirs(name, mode=0o777, exist_ok=False):
    """Super-mkdir; create a leaf directory and all intermediate ones.  Works like
    mkdir, except that any intermediate path segment (not just the rightmost)
    will be created if it does not exist. If the target directory already
    exists, raise an OSError if exist_ok is False. Otherwise no exception is
    raised.  This is recursive.

    Note: This function has been imported from Python 3.4 sources and adapted to work
    with Python 2.X because get_user_cache_dir() uses the exist_ok parameter. It can
    be removed again when Python 2.X support is dropped.
    """
    head, tail = os.path.split(name)
    if not tail:
        head, tail = os.path.split(head)
    if head and tail and not os.path.exists(head):
        try:
            makedirs(head, mode, exist_ok)
        except (IOError, OSError) as e:
            # be happy if someone already created the path
            if e.errno != errno.EEXIST:
                raise
        cdir = os.path.curdir
        if isinstance(tail, bytes):
            cdir = os.path.curdir.encode("ascii")
        if tail == cdir:           # xxx/newdir/. exists if xxx/newdir exists
            return
    try:
        os.mkdir(name, mode)
    except (IOError, OSError) as e:
        if not exist_ok or e.errno != errno.EEXIST or not os.path.isdir(name):
            raise


def get_user_cache_dir(dir=None):
    '''
    This is a Python reimplemention of `g_get_user_cache_dir()` because we don't want to
    rely on the python-xdg package and we can't depend on GLib via introspection.
    If any changes are made to that function they'll need to be copied here.
    '''

    xdg_cache_home = os.environ.get('XDG_CACHE_HOME')
    if xdg_cache_home is not None:
        if dir is not None:
            xdg_cache_home = os.path.join(xdg_cache_home, dir)
        try:
            makedirs(xdg_cache_home, mode=0o755, exist_ok=True)
        except EnvironmentError:
            # Let's fall back to ~/.cache below
            pass
        else:
            return xdg_cache_home

    homedir = os.path.expanduser('~')
    if homedir is not None:
        cachedir = os.path.join(homedir, '.cache')
        if dir is not None:
            cachedir = os.path.join(cachedir, dir)
        try:
            makedirs(cachedir, mode=0o755, exist_ok=True)
        except EnvironmentError:
            return None
        else:
            return cachedir

    return None


def get_system_data_dirs():
    '''
    This is a Python reimplemention of `g_get_system_data_dirs()` because we don't want to
    rely on the python-xdg package and we can't depend on GLib via introspection.
    If any changes are made to that function they'll need to be copied here.
    '''
    xdg_data_dirs = [x for x in os.environ.get('XDG_DATA_DIRS', '').split(os.pathsep)]
    if not xdg_data_dirs and os.name != 'nt':
        xdg_data_dirs.append('/usr/local/share')
        xdg_data_dirs.append('/usr/share')

    return xdg_data_dirs


def rmtree(*args, **kwargs):
    '''
    A variant of shutil.rmtree() which waits and tries again in case one of
    the files in the directory tree can't be deleted.

    This can be the case if a file is still in use by some process,
    for example a Virus scanner on Windows scanning .exe files when they are
    created.
    '''

    tries = 3
    for i in range(1, tries + 1):
        try:
            shutil.rmtree(*args, **kwargs)
        except OSError:
            if i == tries:
                raise
            time.sleep(i)
            continue
        else:
            return
