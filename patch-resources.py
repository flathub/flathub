#!/usr/bin/python3


import re, sys


replacements = (
    # Uncomment to force-enable debug logging
    # (re.compile(re.escape(b'if (app.argv.debugMode)')), b'if (true)'),
    (re.compile(re.escape(b'${os.homedir()}/.local/share')), b'${process.env.UNITY_DATADIR}'),
    (re.compile(re.escape(b'path.parse(folder).root')), b'path.parse(folder).dir'),
    (re.compile(b'getDiskRootPath\(folder\)\s+{.*?return.*?}', re.DOTALL),
        b'''getDiskRootPath(f) {
    let p = f;
    while (true) {
      p = path.parse(p).dir;
      if (fs.existsSync(p)) return p;
    }
  }'''),
)


with open(sys.argv[1], 'r+b') as fp:
    buf = bytearray(fp.read(2048))

    # Use a 1024-byte sliding window of the 2048-byte buffer to avoid potentially splitting
    # the strings we're looking for over a buffer edge boundary.
    # It's a bit inefficient but fast enough for this use case.
    # We mostly just don't want to try to sort through the entire 60MB+ file in memory.

    while True:
        for pattern, replacement in replacements:
            for match in pattern.finditer(buf):
                assert len(match.group()) >= len(replacement), (len(match.group()),
                                                                len(replacement), match.group(),
                                                                replacement)

                replacement = replacement.ljust(len(match.group()))
                # print(f'[{replacement.decode("utf-8")}]')

                pos = fp.tell()
                fp.seek(pos - (2048 - match.start()))
                fp.write(replacement)
                fp.seek(pos)

                buf[match.start():match.end()] = replacement

                break

        chunk = fp.read(1024)
        if not chunk:
            break

        del buf[:1024]
        buf.extend(chunk)
