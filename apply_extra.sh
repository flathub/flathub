#!/usr/bin/bash

bsdtar -Oxf writersolo.deb 'data.tar.xz'|bsdtar -xf - --exclude='usr'

rm writersolo.deb

# The actual named binary is really funky and weird. This step will rename it so anyone else reading the wrapper or the desktop file can understand it much better.
mv 'opt/WriterSolo/${productName}-${version}-${arch}.${ext}' opt/WriterSolo/writersolo-desktop



#DONE!
