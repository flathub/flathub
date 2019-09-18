#!/usr/bin/python3

# patch-resources will patch the Substance Launcher resources file to fix some bugs that are necessary for
# it to function as a Flatpak.

# The resources file is, as other Electron apps, an asar, so the replacement string *must* be the
# same length as the original. In the below replacements sequence, if the replacement string is
# shorter than the match, it will be padded with spaces.


import re, sys


replacements = (
    # Make Substance install to the "extra" dir.
    (re.compile(re.escape(br"'/opt/Allegorithmic/Substance_Designer',")), b"'/app/extra/Substance_Designer',"),
    (re.compile(re.escape(br"'/opt/Allegorithmic/Substance_Painter',")), b"'/app/extra/Substance_Painter',"),
    (re.compile(re.escape(br"'/opt/Allegorithmic/Substance_Alchemist',")), b"'/app/extra/Substance_Alchemist',"),
    (re.compile(re.escape(br"defaultInstallPath = '/opt/Allegorithmic';")), b"defaultInstallPath = '/app/extra';"),
)


with open(sys.argv[1], 'r+b') as fp:
    buf = bytearray(fp.read(2048))
    used_replacements = set()

    # Use a 1024-byte sliding window of the 2048-byte buffer to avoid potentially splitting
    # the strings we're looking for over a buffer edge boundary.
    # It's a bit inefficient but fast enough for this use case.
    # We mostly just don't want to try to sort through the entire 60MB+ file in memory.

    while True:
        for i, (pattern, replacement) in enumerate(replacements):
            for match in pattern.finditer(buf):
                assert len(match.group()) >= len(replacement), (len(match.group()),
                                                                len(replacement), match.group(),
                                                                replacement)

                replacement = replacement.ljust(len(match.group()))

                pos = fp.tell()
                fp.seek(pos - (2048 - match.start()))
                fp.write(replacement)
                fp.seek(pos)

                buf[match.start():match.end()] = replacement
                used_replacements.add(i)

                break

        chunk = fp.read(1024)
        if not chunk:
            break

        del buf[:1024]
        buf.extend(chunk)

    if len(used_replacements) != len(replacements):
        leftover_replacements = set(range(len(replacements))) - used_replacements
        print('Leftover replacements:', file=sys.stderr)
        for leftover_index in leftover_replacements:
            pattern = replacements[leftover_index][0].pattern.decode()
            print('', pattern, file=sys.stderr)
        sys.exit(1)
