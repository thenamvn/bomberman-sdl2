# Makefile cho dự án Bomberman

all:
	g++ -std=c++17 -I SDL2/include -I SDL2_ttf/include -I SDL2_mixer/include -L SDL2/lib -L SDL2_ttf/lib -L SDL2_mixer/lib -o main main.cpp -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

clean:
	del main.exe