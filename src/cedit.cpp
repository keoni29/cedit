/* Core Editor - By Koen van Vliet <8by8mail@gmail.com>
 * World editor for Hero Core ti83+ port.
 */

#include <iostream>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

const int W_WIDTH = 800;
const int W_HEIGHT = 480;
const int W_BPP = 32;
const int GRID = 16;

const int W_SET = 8;
const int H_SET = 8;

SDL_Surface *screen = NULL;
SDL_Surface *tileset = NULL;

typedef struct
{ 
	int x;
	int y;
} offs;

typedef struct
{
	int x;
	int y;
	int id;
} tile;

void apply_surface(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip = NULL)
{
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface(source, clip, destination, &offset);
}

bool tile_place( int x, int y, tile *t )
{
	SDL_Rect tilec;
	tilec.x = t->x * GRID;
	tilec.y = t->y * GRID;
	tilec.w = GRID;
	tilec.h = GRID;
	apply_surface( x * GRID, y * GRID, tileset, screen, &tilec );
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
	char *fset = (char *)"tileset.bmp";

	/* Process parameters and flags */
	if( argc < 2 )
	{
		std::cout << "No set specified. Trying to use default tileset.\n";
	}
	else
	{
		fset = args[1];
	}

	/* Initialize SDL */
	if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}
	screen = SDL_SetVideoMode( W_WIDTH, W_HEIGHT, W_BPP, SDL_SWSURFACE);
	if( screen == NULL )
	{
		return 1;
	}
	SDL_WM_SetCaption( "Core Editor - By Koen van Vliet", NULL);

	/* Load assets */
	tileset = IMG_Load( "tileset.bmp" );
	if( tileset == NULL )
	{
		std::cout << "Error loading file: " << fset << "\n";
		return 1;
	}

	/* Editor
	 * - Initialize editor
	 * - Handle events
	 */
	{
		bool mdl = false, mdr = false;
		bool quit = false;
		int x, y;
		offs o_tp, o_we, o_wp, o_men;
		SDL_Event event;
		tile t;

		/* Initialize editor
		 * - Set all editor positions
		 * - Select tile
		 * - Clip tile from set
		 * - Show tileset on screen
		 * - Update screen
		 */
		o_we.x = 0;
		o_we.y = 0;
		o_tp.x = W_WIDTH - ( 8 * GRID );
		o_tp.y = 0;

		t.x = 0;
		t.y = 0;
		t.id = 0;

		apply_surface(o_tp.x, o_tp.y, tileset, screen );

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
						int txx, tyy;
						mdl = true;
						txx = ( event.button.x - o_tp.x ) / GRID;
						tyy = ( event.button.y - o_tp.y ) / GRID;
						if( txx >= 0 && txx < W_SET && tyy >=0 && tyy < H_SET )
						{
							t.x = txx;
							t.y = tyy;
							t.id = tyy * W_SET + txx;
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
					x = ( event.button.x - o_we.x ) / GRID ;
					y = ( event.button.y - o_we.y ) / GRID ;
					std::cout << "x = " << x << ", y = " << y << ".\n";
					if( x >= 0 && x < 33 && y >= 0 && y < 24 )
					{
						tile_place(x, y, &t);
					}
				}
			}
		}
	}
	SDL_FreeSurface( tileset );
	SDL_Quit();
	return 0;
}