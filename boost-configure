
./bootstrap.sh "$@"


cat <<EOF >Makefile
all:
	./b2 -j `nproc`

install:
	./b2 install
EOF