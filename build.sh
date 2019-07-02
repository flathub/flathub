#!/bin/sh

set -e

/app/sbcl/usr/local/bin/sbcl --load /app/tools/sbcl-init.lisp --load startup.lisp

(
    cd infoparser
    /app/sbcl/usr/local/bin/sbcl --load /app/tools/sbcl-init.lisp \
                                 --non-interactive \
                                 --disable-debugger \
                                 --eval '(ql:quickload "infoparser")' \
                                 --eval '(sb-ext:save-lisp-and-die "maxima-parser.bin" :toplevel #'"'"'infoparser::resolve-example-code-external :executable t)'

    PATH=/app/mupdf/bin:$PATH
    /app/sbcl/usr/local/bin/sbcl --load /app/tools/sbcl-init.lisp \
                                 --non-interactive \
                                 --disable-debugger \
                                 --eval '(ql:quickload "infoparser")' \
                                 --eval '(infoparser:generate-doc-directory :skip-example nil :skip-figures nil)'
)
