/* Core Editor - By Koen van Vliet <8by8mail@gmail.com>
 * World editor for Hero Core ti83+ port.
 */

#include <iostream>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

const int W_WIDTH = 640;
const int W_HEIGHT = 480;
const int W_BPP = 32;

SDL_Surface *screen = NULL;

void apply_surface(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip = NULL){
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface(source, clip, destination, &offset);
}

int main( int argc, char *args[] )
{
	bool quit = false;
	SDL_Event event;
	SDL_Surface *tilemap = NULL;

	/*if( argc < 2 )
	{
		std::cout << "Not enough parameters\n";
		return 1;
	}*/

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

	tilemap = IMG_Load( "tileset.bmp" );
	if( tilemap == NULL )
	{
		std::cout << "Error loading tileset.bmp\n";
		return 1;
	}

	// Ok, now that has been taken care of I can start working on the world editor...
	apply_surface( 0, 0, tilemap, screen );

	if(SDL_Flip(screen) == -1)
	{
		std::cout << "Error updating screen!\n";
		return 1;
	}

	while(quit == false)
	{
		while(SDL_PollEvent( &event ))
		{	
			if(event.type == SDL_QUIT)
			{
				quit = true;
			}
		}
	}

	SDL_FreeSurface( tilemap );
	SDL_Quit();
	return 0;
}