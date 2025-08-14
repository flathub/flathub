#!/usr/bin/env python3
"""
Simple runner for the simplified sshpilot package under new/
"""

import sys
import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PARENT = os.path.dirname(CURRENT_DIR)

# Ensure simplified package is importable
sys.path.insert(0, CURRENT_DIR)
sys.path.insert(0, PARENT)

from sshpilot.main import main

if __name__ == '__main__':
    main()


