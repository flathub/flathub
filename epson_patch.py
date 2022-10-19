#!/usr/bin/python

#
# Patch the binary in place: it want to dlopen stuff in /usr
#
with open('/app/lib/x86_64-linux-gnu/epsonscan2/non-free-exec/es2intif', 'r+b') as exe:
    exe.seek(0x20380 + 4)
    exe.write(b'app')
    exe.seek(0x20400 + 1)
    exe.write(b'app')
