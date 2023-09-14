
CFLAGS=$(shell /app/bin/python3-config --cflags)
LDFLAGS=$(shell /app/bin/python3-config --ldflags)

all: simplethanks

simplethanks: main.c
	gcc -o simplethanks $^ $(CFLAGS) $(LDFLAGS) -lpython3.11

clean:
	rm -f simplethanks

install:
	cp simplethanks /app/bin/simplethanks

.PHONY: all clean install
