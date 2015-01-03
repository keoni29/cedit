// cedit.h
#ifndef EDITOR_

#define EDITOR_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

typedef struct
{
	int x;
	int y;
	int i;
} Coord;

class Block{
public:
	char *buff;
	int x;
	int y;
	int w;
	int h;

	bool get_rel_xy( int cx, int cy, Coord *t );
	bool tile_place( int cx, int cy, int ix, SDL_Surface *tileset );
	bool draw( int viewx, int viewy, SDL_Surface *tileset );
};

#endif