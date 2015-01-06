// cedit.h
#ifndef EDITOR_

#define EDITOR_
typedef struct
{
	int x;
	int y;
	int i;
} Coord;

class Block
{
public:
	char *buff;
	int x;
	int y;
	int w;
	int h;

	bool get_rel_xy( int cx, int cy, Coord *t );
	bool tile_place( int cx, int cy, int ix, SDL_Surface *tileset );
	bool draw( int viewx, int viewy, SDL_Surface *tileset, bool border );

	SDL_Surface *screen;
};

class Editor
{
public:
	int GRID;
	int ROOM_WIDTH;
	int ROOM_HEIGHT;
	int SCREEN_WIDTH;
	int SCREEN_HEIGHT;
	int SET_WIDTH;
	int SET_HEIGHT;
	int VIEW_WIDTH;
	int VIEW_HEIGHT;
	int WORLD_WIDTH;
	int WORLD_HEIGHT;
	SDL_Surface *screen;
};

#endif