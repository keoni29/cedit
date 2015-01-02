/* Core Editor - By Koen van Vliet <8by8mail@gmail.com>
 * World editor for Hero Core ti83+ port.
 */
#include <fstream>
#include <iostream>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"
#include <string>
#include "cedit.h"


const int GRID = 16;

const int WORLD_WIDTH = 9;
const int WORLD_HEIGHT = 8;
const int ROOM_WIDTH = 11;
const int ROOM_HEIGHT = 8;

const int SET_WIDTH = 8;
const int SET_HEIGHT = 8;

const int SCREEN_WIDTH = ROOM_WIDTH * 3 + SET_WIDTH + 1;
const int SCREEN_HEIGHT = ROOM_HEIGHT * 3;
const int SCREEN_BPP = 32;

SDL_Surface *screen = NULL;
SDL_Surface *tileset = NULL;
SDL_Surface *background = NULL;

SDL_Rect tile_clip( int ix )
{
	SDL_Rect tilec;

	tilec.x = ( ix % SET_WIDTH ) * GRID;
	tilec.y = ( ix / SET_WIDTH ) * GRID;
	tilec.w = GRID;
	tilec.h = GRID;

	return tilec;
}

SDL_Surface *load_image( std::string filename)
{
	SDL_Surface* loadedImage = NULL;
	SDL_Surface* optimizedImage = NULL;
	loadedImage = IMG_Load( filename.c_str() );
	if( loadedImage != NULL ){
		optimizedImage = SDL_DisplayFormat( loadedImage );
		SDL_FreeSurface( loadedImage );
	}
	return optimizedImage;
}

void apply_surface(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip = NULL)
{
	SDL_Rect offset;
	offset.x = x * GRID;
	offset.y = y * GRID;
	SDL_BlitSurface(source, clip, destination, &offset);
}

void draw_tile( int x, int y, int ix )
{
	SDL_Rect tilec = ::tile_clip( ix );
	apply_surface( x, y, tileset, screen, &tilec );
}

bool Block::get_rel_xy( int cx, int cy, Coord *t )
{
	int xrel;
	int yrel;

	xrel = ( cx - x );
	yrel = ( cy - y );

	if( xrel >= 0 && xrel < w && yrel >= 0 && yrel < h )
	{
		t->x = xrel;
		t->y = yrel;
		t->i = yrel * w + xrel;
		return true;
	}
	return false;
}

bool Block::tile_place( int cx, int cy, int ix, char *buff )
{
	Coord t;

	if ( get_rel_xy( cx, cy, &t ) )
	{
		buff += t.y * ROOM_WIDTH + t.x;
		*buff = ix;
		::draw_tile( t.x + x, t.y + y, ix );
		if( SDL_Flip( screen ) == -1 )
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool Block::draw( int viewx, int viewy, char *buff )
{
	int xx, yy;
	char tile;
	for( yy = 0; yy < ROOM_HEIGHT; yy++ )
	{
		for( xx = 0; xx < ROOM_WIDTH; xx++ )
		{
			tile = *(buff + ( ROOM_WIDTH * ROOM_HEIGHT * ( WORLD_WIDTH * ( viewy + ry ) + viewx + rx ) ) + ( ROOM_WIDTH * yy + xx ) );
			::draw_tile( ROOM_WIDTH * rx + xx, ROOM_HEIGHT * ry + yy, tile );
		}
	}
	return true;
}

int main( int argc, char *args[] )
{
	/* Initialize application
	 * - Allocate memory for world editor
	 * - Initialize SDL
	 * - Load assets
	 */
	int rx, ry;
	int buffer_size = ROOM_WIDTH * ROOM_HEIGHT * WORLD_WIDTH * WORLD_HEIGHT;
	char *tilemap = (char *)malloc( buffer_size );

	/* Initialize SDL */
	if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}

	screen = SDL_SetVideoMode( SCREEN_WIDTH * GRID, SCREEN_HEIGHT * GRID, SCREEN_BPP, SDL_SWSURFACE);
	
	if( screen == NULL )
	{
		return 1;
	}

	SDL_WM_SetCaption( "Core Editor - By Koen van Vliet", NULL);

	/* Load assets */
	tileset = load_image( "tileset.bmp" );
	background = load_image( "background.bmp" );

	if( tileset == NULL || background == NULL )
	{
		std::cout << "Error loading image files.\n";
		return 1;
	}

	/* Load world file */
	std::ifstream inFile( "data.bin", std::ifstream::out | std::ifstream::binary );

	if (inFile != NULL)
	{
 		inFile.read( tilemap, buffer_size );
	}


	/* Editor
	 * - Initialize editor
	 * - Handle events
	 */

	bool mdl = false, mdr = false;
	bool quit = false;
	Block b_room[3][3];
	Block b_picker;
	Block b_map;
	int x, y;
	SDL_Event event;
	int tile = 1;
	int cx, cy;
	int lcx, lcy;
	int room_x = 0, room_y = 0;

	/* Initialize editor
	 * - Create all editor blocks
	 * - Select tile
	 * - Clip tile from set
	 * - Show tileset on screen
	 * - Update screen
	 */
	for( y = 0; y < 3; y++ )
	{
		for( x = 0; x < 3; x++ )
		{
			b_room[x][y] = {
				x,
				y,
				x * ROOM_WIDTH,
				y * ROOM_HEIGHT,
				ROOM_WIDTH,
				ROOM_HEIGHT
			};

			apply_surface(b_room[x][y].x, b_room[x][y].y , background, screen );
			b_room[x][y].draw( room_x, room_y, tilemap );
		}
	}


	b_picker = {
		0,0,
		SCREEN_WIDTH - ( SET_WIDTH ),
		0,
		SET_WIDTH,
		SET_HEIGHT
	};

	b_map = {
		0,0,
		SCREEN_WIDTH - WORLD_WIDTH,
		( SET_HEIGHT + 1 ),
		WORLD_WIDTH,
		WORLD_HEIGHT
	};

	apply_surface(b_picker.x, b_picker.y, tileset, screen );

	if( SDL_Flip( screen ) == -1 )
	{
		std::cout << "Error updating screen!\n";
		return 1;
	}

	/* Handle events
	 * - Move mouse: Place tile if left mouse button is held down
	 * - Left click: in world editor:Place tile/ in tile picker:Select tile
	 * - Quit: Stop SDL and end application
	 */
	while( quit == false )
	{
		while( SDL_PollEvent( &event ) )
		{
			cx = event.button.x / GRID;
			cy = event.button.y / GRID;
			if( event.type == SDL_MOUSEBUTTONDOWN )
			{
				/* Select tile */
				if( event.button.button == SDL_BUTTON_LEFT )
				{
					Coord t;
					mdl = true;

					if( cx != lcx || cy != lcy ){
						if( b_picker.get_rel_xy( cx, cy, &t ) )
						{
							tile = t.i;
						}
					}
				}
				else if( event.button.button == SDL_BUTTON_RIGHT )
				{
					mdr = true;
				}
			}
			else if( event.type == SDL_MOUSEBUTTONUP )
			{
				switch( event.button.button )
				{
					case SDL_BUTTON_LEFT:
						mdl = false;
						break;
					case SDL_BUTTON_RIGHT:
						mdr = false;
						break;
				}
			}
			else if( event.type == SDL_QUIT )
			{
				quit = true;
			}
			/* Place tiles */
			if( ( event.type == SDL_MOUSEMOTION && ( mdl || mdr ) && ( cx != lcx || cy != lcy  ) ) || ( event.type == SDL_MOUSEBUTTONDOWN ) )
			{
				Coord txy;
				int tid, offs;

				if ( event.button.button == SDL_BUTTON_LEFT )
				{
					tid = tile;
				}
				else
				{
					tid = 0;
				}

				rx = ( cx - b_room[0][0].x ) / ROOM_WIDTH;
				ry = ( cy - b_room[0][0].y ) / ROOM_HEIGHT;
				if (rx >= 0 && rx < 3 && ry >= 0 && ry < 3)
				{
					offs = ROOM_WIDTH * ROOM_HEIGHT * ( WORLD_WIDTH * ( room_y + ry ) + room_x + rx );
					if ( b_room[rx][ry].tile_place( cx, cy, tid, tilemap + offs ) )
					{
						std::cout << "Tile placed in room[" << rx << ", " << ry << "] offs =" << offs << "\n";
					}
				}
				lcx = cx;
				lcy = cy;
			}
		}
	}

	SDL_FreeSurface( tileset );
	SDL_Quit();

	std::ofstream outFile( "data.bin", std::ofstream::out | std::ofstream::binary );
	outFile.write( tilemap, buffer_size );
	return 0;
}