#!/usr/bin/env python3

import os
import xsettings

if __name__ == '__main__':
    try:
        settings = xsettings.get_xsettings()
        scale_factor = settings.get(b'Gdk/WindowScalingFactor')
        print('{0:.1f}'.format(scale_factor))
    except:
        print(os.environ.get('GDK_SCALE', '1'))
