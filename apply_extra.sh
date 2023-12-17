#!/usr/bin/bash


bsdtar -xf writersolo.deb
tar -xvf data.tar.xz

rm writersolo.deb
rm *.xz
rm *.gz
rm debian-binary

#Note, not removing the usr/ directory it comes with. At least, not right now.




#SUID chrome-sandbox for Electron 5+
chmod 4755 ./opt/WriterSolo/chrome-sandbox


#DONE!
