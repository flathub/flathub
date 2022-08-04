#!/usr/bin/env python3

import pathlib
import subprocess


LOCAL_DIR = pathlib.Path("calcleaner/data/locales")


def build_locales():
    for po_file in pathlib.Path("locales").glob("*.po"):
        output_file = (
            LOCAL_DIR
            / po_file.name[: -len(po_file.suffix)]
            / "LC_MESSAGES"
            / "org.flozz.calcleaner.mo"
        )
        print(output_file.as_posix())
        output_file.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [
                "msgfmt",
                po_file.as_posix(),
                "-o",
                output_file.as_posix(),
            ]
        )


if __name__ == "__main__":
    build_locales()
