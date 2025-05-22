pydbus
======
.. image:: https://travis-ci.org/LEW21/pydbus.svg?branch=master
    :target: https://travis-ci.org/LEW21/pydbus
.. image:: https://badge.fury.io/py/pydbus.svg
    :target: https://badge.fury.io/py/pydbus

Pythonic DBus library.

Changelog: https://github.com/LEW21/pydbus/releases

Requirements
------------
* Python 2.7+ - but works best on 3.4+ (help system is nicer there)
* PyGI_ (not packaged on pypi, you need to install it from your distribution's repository - it's usually called python-gi, python-gobject or pygobject)
* GLib_ 2.46+ and girepository_ 1.46+ (Ubuntu 16.04+) - for object publication support

.. _PyGI: https://wiki.gnome.org/Projects/PyGObject
.. _GLib: https://developer.gnome.org/glib/
.. _girepository: https://wiki.gnome.org/Projects/GObjectIntrospection

Examples
--------

Send a desktop notification
~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. code-block:: python

	from pydbus import SessionBus

	bus = SessionBus()
	notifications = bus.get('.Notifications')

	notifications.Notify('test', 0, 'dialog-information', "Hello World!", "pydbus works :)", [], {}, 5000)

List systemd units
~~~~~~~~~~~~~~~~~~
.. code-block:: python

	from pydbus import SystemBus

	bus = SystemBus()
	systemd = bus.get(".systemd1")

	for unit in systemd.ListUnits():
	    print(unit)

Start or stop systemd unit
~~~~~~~~~~~~~~~~~~
.. code-block:: python

	from pydbus import SystemBus

	bus = SystemBus()
	systemd = bus.get(".systemd1")

	job1 = systemd.StopUnit("ssh.service", "fail")
	job2 = systemd.StartUnit("ssh.service", "fail")

Watch for new systemd jobs
~~~~~~~~~~~~~~~~~~~~~~~~~~
.. code-block:: python

	from pydbus import SystemBus
	from gi.repository import GLib

	bus = SystemBus()
	systemd = bus.get(".systemd1")

	systemd.JobNew.connect(print)
	GLib.MainLoop().run()

	# or

	systemd.onJobNew = print
	GLib.MainLoop().run()

View object's API
~~~~~~~~~~~~~~~~~
.. code-block:: python

	from pydbus import SessionBus

	bus = SessionBus()
	notifications = bus.get('.Notifications')

	help(notifications)

More examples & documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Tutorial_ contains more examples and docs.

.. _Tutorial: https://github.com/LEW21/pydbus/blob/master/doc/tutorial.rst

Copyright Information
---------------------

Copyright (C) 2014, 2015, 2016 Linus Lewandowski <linus@lew21.net>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


