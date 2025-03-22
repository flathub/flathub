DEBUG:=1
PREFIX:=/app

all: libfix-app-location.so

clean:
	rm -f fix-app-location.o libfix-app-location.so
	$(foreach s, $(icon_resolutions), rm -f $(s)_icon.png;)

fix-app-location.o: fix-app-location.c
	gcc -Wall -DDEBUG=$(DEBUG) -fPIC -c -o fix-app-location.o fix-app-location.c

libfix-app-location.so: fix-app-location.o
	gcc -shared -fPIC -Wl,-soname -Wl,libfix-app-location.so -o libfix-app-location.so fix-app-location.o

install: libfix-app-location.so
	install -m755 -Dt $(PREFIX)/lib libfix-app-location.so
