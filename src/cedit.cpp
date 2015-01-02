/* Core Editor - By Koen van Vliet <8by8mail@gmail.com>
 * World editor for Hero Core ti83+ port.
 */

#include <iostream>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"
#include "cedit.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;
const int GRID = 16;

const int WORLD_WIDTH = 9;
const int WORLD_HEIGHT = 8;
const int ROOM_WIDTH = 11;
const int ROOM_HEIGHT = 8;

const int SET_WIDTH = 8;
const int SET_HEIGHT = 8;

SDL_Surface *screen = NULL;
SDL_Surface *tileset = NULL;
SDL_Surface *background = NULL;

SDL_Rect tile_clip( int ix ){
	SDL_Rect tilec;

	tilec.x = ( ix % SET_WIDTH ) * GRID;
	tilec.y = ( ix / SET_WIDTH ) * GRID;
	tilec.w = GRID;
	tilec.h = GRID;

	return tilec;
}

void apply_surface(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip = NULL)
{
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface(source, clip, destination, &offset);
}

bool Block::get_grid_xy( int cx, int cy, Coord *t )
{
	int xrel;
	int yrel;

	xrel = ( cx - x ) / GRID;
	yrel = ( cy - y ) / GRID;

	if( cx >= x && cx < w * GRID + x && cy >= y && cy < h * GRID + y )
	{
		t->x = xrel;
		t->y = yrel;
		t->i = yrel * w + xrel;
		return true;
	}
	return false;
}

bool Block::tile_place( int cx, int cy, int ix )
{
	Coord t;
	int xrel;
	int yrel;
	SDL_Rect tilec = ::tile_clip( ix );

	get_grid_xy( cx, cy, &t );

	::apply_surface( t.x * GRID + x , t.y * GRID + y, tileset, screen, &tilec );

	if( SDL_Flip( screen ) == -1 )
	{
		return false;
	}
	return true;
}

int main( int argc, char *args[] )
{
	/* Initialize application
	 * - Process parameters and flags
	 * - Initialize SDL
	 * - Load assets
	 */

	/* Initialize SDL */
	if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}

	screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE);
	
	if( screen == NULL )
	{
		return 1;
	}

	SDL_WM_SetCaption( "Core Editor - By Koen van Vliet", NULL);

	/* Load assets */
	tileset = IMG_Load( "tileset.bmp" );
	background = IMG_Load( "background.bmp" );

	if( tileset == NULL || background == NULL )
	{
		std::cout << "Error loading image files.\n";
		return 1;
	}

	/* Editor
	 * - Initialize editor
	 * - Handle events
	 */
	{
		bool mdl = false, mdr = false;
		bool quit = false;
		char tilemap[ WORLD_WIDTH * WORLD_HEIGHT ][ ROOM_WIDTH * ROOM_HEIGHT ] = {0};
		Block b_room[3][3];
		Block b_picker;
		Block b_map;
		int x, y;
		SDL_Event event;
		int tile;

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
					x * ROOM_WIDTH * GRID,
					y * ROOM_HEIGHT * GRID,
					ROOM_WIDTH,
					ROOM_HEIGHT
				};

				apply_surface(b_room[x][y].x, b_room[x][y].y , background, screen );
			}
		}

		b_picker = {
			SCREEN_WIDTH - ( SET_WIDTH * GRID ),
			0,
			SET_WIDTH,
			SET_HEIGHT
		};

		b_map = {
			SCREEN_WIDTH - ( WORLD_WIDTH * GRID ),
			( SET_HEIGHT + 1 ) * GRID,
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
				
				if( event.type == SDL_MOUSEBUTTONDOWN )
				{
					/* Select tile */
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						Coord t;
						mdl = true;

						if( b_picker.get_grid_xy( event.button.x, event.button.y, &t ) )
						{
							tile = t.i;
						}
					}
					else if( event.button.button == SDL_BUTTON_RIGHT )
					{
						mdr = true;
					}
				}
				else if( event.type == SDL_MOUSEBUTTONUP )
				{
					switch( event.button.button == SDL_BUTTON_LEFT )
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
				if( ( event.type == SDL_MOUSEMOTION && mdl ) || ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT ) )
				{
					Coord txy;
					int rx, ry;
					rx = ( event.button.x - b_room[0][0].x ) / ( ROOM_WIDTH * GRID );
					ry = ( event.button.y - b_room[0][0].y ) / ( ROOM_HEIGHT * GRID );
					if (event.button.x >= 0 && rx < 3 && event.button.y >= 0 && ry < 3)
					{
						if ( b_room[rx][ry].tile_place( event.button.x, event.button.y, tile ) )
						{
							std::cout << "Tile placed in room[" << rx << ", " << ry << "]\n";
						}
					}
				}
			}
		}
	}
	SDL_FreeSurface( tileset );
	SDL_Quit();
	return 0;
}