#!/usr/bin/env python3
"""Reject a source imagecodecs build that silently drops a Linux wheel extension.

The extension inventory is from imagecodecs v2026.8.16 setup.py, applying its
customize_build_cibuildwheel Linux exclusions. Imports prove availability, not
numerical parity; full RAW/TIFF/ICC acceptance is a separate release gate.
"""
import importlib
import json

EXPECTED_EXTENSIONS = ('shared', 'imcd', 'aec', 'apng', 'avif', 'bcn', 'bitshuffle', 'blosc', 'blosc2', 'bmp', 'brotli', 'bz2', 'ccitt', 'cfitsio', 'cms', 'czi', 'deflate', 'exr', 'gif', 'h5checksum', 'htj2k', 'isal', 'jpeg2k', 'jpeg8', 'jpegls', 'jpegsof3', 'jpegxl', 'jpegxr', 'lerc', 'ljpeg', 'lz4', 'lzf', 'lzfse', 'lzham', 'lzma', 'lzo', 'meshopt', 'pcodec', 'pglz', 'pixarlog', 'png', 'qoi', 'quantize', 'rgbe', 'snappy', 'sperr', 'spng', 'sz3', 'szip', 'pcx', 'tga', 'tiff', 'ultrahdr', 'wavpack', 'webp', 'zfp', 'zlib', 'zlibng', 'zopfli', 'zstd')


def check(import_module=importlib.import_module):
    failures = {}
    for name in EXPECTED_EXTENSIONS:
        try:
            import_module('imagecodecs._' + name)
        except (ImportError, OSError) as error:
            failures[name] = str(error)
    if failures:
        raise RuntimeError('Missing required source-built codecs: ' + json.dumps(failures, sort_keys=True))
    return list(EXPECTED_EXTENSIONS)


def main():
    available = check()
    import imagecodecs
    print(json.dumps({'codec_modules_imported': available, 'versions': imagecodecs.version(),
                      'numerical_parity_verified': False}, indent=2))


if __name__ == '__main__':
    main()
