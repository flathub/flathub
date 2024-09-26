
CFLAGS=$(shell /usr/bin/python3-config --cflags)
LDFLAGS=$(shell /usr/bin/python3-config --ldflags)

all: stickyhours

stickyhours: main.c
	gcc -o stickyhours $^ $(CFLAGS) $(LDFLAGS) -lpython3.12

clean:
	rm -f stickyhours

install:
	cp stickyhours /app/bin/stickyhours

.PHONY: all clean install
