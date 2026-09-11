"""Keep the Linux wheel codec set; never silently use imagecodecs' default lite build.

The omissions match customize_build_cibuildwheel in imagecodecs 2026.8.16.
The native recipes supply the Linux wheel codec set. Missing libraries must
still fail compilation; no SDK-driven fallback is allowed to remove codecs.
"""
from pathlib import Path


def customize_build(extensions, options):
    for name in ('heif', 'jetraw', 'jpegxs', 'mozjpeg', 'brunsli', 'openzl', 'wic'):
        extensions.pop(name, None)
    extensions['jpeg8']['sources'] = []  # Linux wheels use libjpeg-turbo 3.
    extensions['lzham']['libraries'] = ['lzhamdll']
    extensions['lzham']['include_dirs'].append('/app/include/lzham')
    extensions['zfp']['extra_compile_args'].append('-fopenmp')
    extensions['zfp']['extra_link_args'].append('-fopenmp')
    options['library_dirs'].extend(['/app/lib', '/usr/lib'])
    for base in (Path('/app/include'), Path('/usr/include')):
        options['include_dirs'].append(str(base))
        for child in ('openjpeg-2.5', 'OpenEXR', 'Imath', 'jxrlib', 'zopfli',
                      'lzokay', 'lzokay-c', 'SZ3c', 'isa-l'):
            options['include_dirs'].append(str(base / child))
    # In particular, retain jpegxl even if its headers are missing. Upstream's
    # wheel helper silently removes it; this candidate must fail instead.
