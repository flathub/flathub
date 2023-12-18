#!/usr/bin/bash

bsdtar -Oxf writersolo.deb 'data.tar.xz'|bsdtar -xf - --exclude='usr'

rm writersolo.deb

# For unknown reasons, the actual executable binary is named wrong. Got to fix that.
mv 'opt/WriterSolo/${productName}-${version}-${arch}.${ext}' opt/WriterSolo/writersolo-desktop



#DONE!
