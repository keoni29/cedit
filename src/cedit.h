// cedit.h
#ifndef EDITOR_

#define EDITOR_
typedef struct
{ 
	int x;
	int y;
} offs;

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
		bool tile_place( int cx, int cy, int ix);
		bool draw( int viewx, int viewy);
};
#endif