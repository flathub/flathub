DEBUG=0

all: libfix-config-location.so
CC = i686-unknown-linux-gnu-gcc

clean:
	rm -f libfix-config-location.o libfix-config-location.so

fix-config-location.o: fix-config-location.c directories.h Makefile
	$(CC) -Wall -DDEBUG=$(DEBUG) -fPIC -c -o fix-config-location.o fix-config-location.c

libfix-config-location.so: fix-config-location.o
	$(CC) -shared -fPIC -Wl,-soname -Wl,libfix-config-location.so -o libfix-config-location.so fix-config-location.o

install: libfix-config-location.so
	install -m755 -Dt /app/lib32 libfix-config-location.so
