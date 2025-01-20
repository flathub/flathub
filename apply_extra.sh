
#!/usr/bin/sh

set -e

bsdtar -Oxf ulaa.deb 'data.tar*' |
  bsdtar -xf - \
    --strip-components=4 \
    --exclude='./opt/zoho/ulaa/nacl*'
rm ulaa.deb
