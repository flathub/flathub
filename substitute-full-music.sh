set -e -u -x

mkdir full-music
cd full-music
mv ../full-Music.ocg Music.ocg
../c4group Music.ocg -x
cd Music.ocg
mv * ../../planet/Music.ocg/
