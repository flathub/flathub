LDLIBS := -lm -lSDL -lGL
CFLAGS := -g -Ofast -Wall -Wextra -I../../source -I../../source/qcommon -include standins.h

VPATH = ../../source/qcommon

ALL: visualizer
.PHONY: ALL

clean:
	rm *.o visualizer

visualizer: main.o standins.o libgarland.o binheap.o terrain.o image.o files.o  ../../source/libgame.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) 
