# -*- coding: utf-8 -*-

import argparse
import sys
from bitsafe.connection import Connection
from bitsafe.constants import CLI_VERSION
class BitSafeCLI:
    def __init__(self):
        #main args
        self.argparcer = argparse.ArgumentParser(
            prog="bsafe",
            description="password manager - BitSafe CLI",
            epilog="Development by: Francivaldo Costa",
            usage="%(prog)s [options]"
        )
        self.argparcer.version = CLI_VERSION
        self.argparcer.add_argument(
            "-v", "--version",
            action="version")
        Connection(self.argparcer)
        parser_args = self.argparcer.parse_args()
        if parser_args:
            try:
                if parser_args.func:
                    parser_args.func(parser_args)
            except Exception as e:
                self.argparcer.print_help()
                sys.exit(1)
