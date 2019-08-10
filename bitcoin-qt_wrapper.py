#!/usr/bin/python3
import os
import sys

BITCOIN_QT_PATH = "/app/bin/bitcoin-qt"
XDG_DATA_HOME = os.environ["XDG_DATA_HOME"]
ARGS = ["-datadir={}".format(XDG_DATA_HOME)]


def main(bitcoin_binary=BITCOIN_QT_PATH):
    os.execve(
        bitcoin_binary, [bitcoin_binary] + ARGS + sys.argv[1:], os.environ
    )


if __name__ == "__main__":
    main()
