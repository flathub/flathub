#!/usr/bin/python3
import os
import sys

APP_DIR = "/app/share/flippy"

os.chdir(APP_DIR)
sys.path.insert(0, APP_DIR)

import flippy
flippy.main()
