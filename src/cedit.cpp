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

typedef struct
{ 
	int x;
	int y;
} offs;

void apply_surface(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip = NULL){
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface(source, clip, destination, &offset);
}

int main( int argc, char *args[] )
{
	bool mdl = false, mdr = false;
	bool quit = false;
	char *fset = (char *)"tileset.bmp";
	int x, y, tx, ty;
	offs o_tp, o_we, o_wp, o_men;
	SDL_Event event;
	SDL_Rect tilec;
	SDL_Surface *tileset = NULL;

	if( argc < 2 )
	{
		std::cout << "No set specified. Trying to use default tileset.\n";
	}
	else
	{
		fset = args[1];
	}

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

	tileset = IMG_Load( "tileset.bmp" );
	if( tileset == NULL )
	{
		std::cout << "Error loading file: " << fset << "\n";
		return 1;
	}

	// Initialize editor
	tx = 1;
	ty = 0;

	tilec.x = tx * GRID;
	tilec.y = ty * GRID;
	tilec.w = GRID;
	tilec.h = GRID;

	o_we.x = 0;
	o_we.y = 0;
	o_tp.x = W_WIDTH - ( 8 * GRID );
	o_tp.y = 0;

	// Ok, now that has been taken care of I can start working on the world editor...
	apply_surface(o_tp.x, o_tp.y, tileset, screen );

	if( SDL_Flip( screen ) == -1 )
	{
		std::cout << "Error updating screen!\n";
		return 1;
	}

	while( quit == false )
	{
		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_MOUSEMOTION )
			{
				if( mdl )
				{
					x = ( event.button.x - o_we.x ) / GRID ;
					y = ( event.button.y - o_we.y ) / GRID ;
					std::cout << "x = " << x << ", y = " << y << ".\n";
					if( x >= 0 && x < 33 && y >= 0 && y < 24 )
					{
						apply_surface( x * GRID, y * GRID, tileset, screen, &tilec );
					}
					if( SDL_Flip( screen ) == -1 )
					{
						std::cout << "Error updating screen!\n";
						return 1;
					}
				}
			}
			else if( event.type == SDL_MOUSEBUTTONDOWN )
			{
				if( event.button.button == SDL_BUTTON_LEFT )
				{
					mdl = true;
					tx = ( event.button.x - o_tp.x ) / GRID;
					ty = ( event.button.y - o_tp.y ) / GRID;
					if( tx >= 0 && tx < W_SET && ty >=0 && ty < H_SET )
					{
						tilec.x = tx * GRID;
						tilec.y = ty * GRID;
					}
				}
				else if( event.button.button == SDL_BUTTON_RIGHT )
				{
					mdr = true;
				}
			}
			else if( event.type == SDL_MOUSEBUTTONUP )
			{
				if( event.button.button == SDL_BUTTON_LEFT )
				{
					mdl = false;
				}
				else if( event.button.button == SDL_BUTTON_RIGHT )
				{
					mdr = false;
				}
			}
			else if( event.type == SDL_QUIT )
			{
				quit = true;
			}
		}
	}

	SDL_FreeSurface( tileset );
	SDL_Quit();
	return 0;
}