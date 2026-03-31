CXX = g++
CXXFLAGS = -I/app/include/SDL -I/app/include/freetype2
LDFLAGS = -L/app/lib -lSDL -lSDL_image -lSDL_mixer -lSDL_ttf -lSDL_gfx -lm

OBJS = main.o loadg.o DxLib.o

all: SyobonAction

SyobonAction: $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

clean:
	rm -f SyobonAction $(OBJS)