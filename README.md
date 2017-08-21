# Obtaining the source archive

hg clone http://www.octave.org/hg/octave
cd octave
./bootstrap
./configure
make
make dist
cp octave-4.3.0+.tar.gz ..
