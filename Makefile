all:
	g++ -o bin/cedit src/cedit.cpp -lSDL -lSDL_image -lSDL_ttf -std=c++11 -Wno-narrowing
