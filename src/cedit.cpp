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

int VIEW_WIDTH = 3;
int VIEW_HEIGHT = 4;

int SCREEN_WIDTH;
int SCREEN_HEIGHT;
const int SCREEN_BPP = 32;

SDL_Surface *screen = NULL;
SDL_Surface *tileset = NULL;

SDL_Rect tile_clip( int ix )
{
	SDL_Rect tilec;

	tilec.x = ( ix % SET_WIDTH ) * GRID;
	tilec.y = ( ix / SET_WIDTH ) * GRID;
	tilec.w = GRID - 1;
	tilec.h = GRID - 1;

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
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface(source, clip, destination, &offset);
}

void draw_tile( int x, int y, int ix )
{
	SDL_Rect tilec = ::tile_clip( ix );
	apply_surface( x * GRID, y * GRID, tileset, screen, &tilec );
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

bool Block::tile_place( int cx, int cy, int ix)
{
	Coord t;

	if ( get_rel_xy( cx, cy, &t ) )
	{
		*( buff + t.y * ROOM_WIDTH + t.x ) = ix;
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

bool Block::draw( int viewx, int viewy)
{
	int xx, yy;
	char tile;
	for( yy = 0; yy < ROOM_HEIGHT; yy++ )
	{
		for( xx = 0; xx < ROOM_WIDTH; xx++ )
		{
			tile = *(buff + ( ROOM_WIDTH * yy + xx ) );
			::draw_tile( xx + x, yy + y, tile );
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

	bool mdl = false, mdr = false;
	bool redraw;
	bool quit = false;
	char *tilemap = NULL;
	int rx, ry;
	int buffer_size;
	Block b_room[VIEW_WIDTH][VIEW_HEIGHT];
	Block b_picker;
	Block b_map;
	int x, y;
	SDL_Event event;
	int tile = 1;
	int cx, cy;
	int lcx, lcy;
	int view_x = 0, view_y = 0;

	SDL_Surface *background = NULL;
	SDL_Surface *arrows = NULL;
	SDL_Rect arrow_u = { 0, 0, 32, 14 };
	SDL_Rect arrow_d = { 0, 34, 32, 14 };
	SDL_Rect arrow_l = { 0, 14, 16, 20 };
	SDL_Rect arrow_r = { 16, 14, 16, 20 };

	if( VIEW_WIDTH > WORLD_WIDTH )
	{
		std::cout << "Warning: View width is larger than world width. Resizing view.\n";
		VIEW_WIDTH = WORLD_WIDTH;
	}
	if( VIEW_HEIGHT > WORLD_HEIGHT )
	{
		std::cout << "Warning: View height is larger than world height. Resizing view.\n";
		VIEW_HEIGHT = WORLD_HEIGHT;
	}

	SCREEN_WIDTH = ROOM_WIDTH * VIEW_WIDTH + SET_WIDTH + 2;
	SCREEN_HEIGHT = ROOM_HEIGHT * VIEW_HEIGHT + 2;

	buffer_size = ROOM_WIDTH * ROOM_HEIGHT * WORLD_WIDTH * WORLD_HEIGHT;
	tilemap = (char *)calloc( buffer_size, sizeof(char) );

	if( tilemap == NULL)
	{
		std::cout << "Error: Could not allocate memory\n";
		return 1;
	}

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
	arrows = load_image( "arrows.bmp" );
	if( tileset == NULL || background == NULL || arrows == NULL )
	{
		std::cout << "Error loading image files.\n";
		return 1;
	}

	/* Load buffer from file file */
	std::ifstream inFile( "data.bin", std::ifstream::out | std::ifstream::binary );
	if (inFile != NULL)
	{
 		inFile.read( tilemap, buffer_size );
	}


	/* Editor
	 * - Initialize editor
	 * - Handle events
	 */

	/* Initialize editor
	 * - Create all editor blocks
	 * - Select tile
	 * - Clip tile from set
	 * - Show tileset on screen
	 * - Update screen
	 */
	for( y = 0; y < VIEW_HEIGHT; y++ )
	{
		for( x = 0; x < VIEW_WIDTH; x++ )
		{
			b_room[x][y] = {
				NULL,
				x * ROOM_WIDTH + 1,
				y * ROOM_HEIGHT + 1,
				ROOM_WIDTH,
				ROOM_HEIGHT
			};
		}
	}


	b_picker = {
		NULL,
		SCREEN_WIDTH - ( SET_WIDTH ),
		1,
		SET_WIDTH,
		SET_HEIGHT
	};

	b_map = {
		NULL,
		SCREEN_WIDTH - WORLD_WIDTH,
		( SET_HEIGHT + 1 ),
		WORLD_WIDTH,
		WORLD_HEIGHT
	};

	apply_surface(b_picker.x * GRID, b_picker.y * GRID, tileset, screen );
	apply_surface(
		( ( ROOM_WIDTH * VIEW_WIDTH + b_room[0][0].x + 1 ) * GRID ) / 2 - ( arrow_u.w / 2 ),
		( b_room[0][0].y - 1 ) * GRID,
		arrows,
		screen,
		&arrow_u
	);

	apply_surface(
		( ( ROOM_WIDTH * VIEW_WIDTH + b_room[0][0].x + 1 ) * GRID ) / 2 - ( arrow_d.w / 2 ),
		( VIEW_HEIGHT * ROOM_HEIGHT + b_room[0][0].y ) * GRID,
		arrows,
		screen,
		&arrow_d
	);

	apply_surface(
		( b_room[0][0].x - 1 ) * GRID,
		( ( ROOM_HEIGHT * VIEW_HEIGHT + b_room[0][0].y + 1 ) * GRID ) / 2 - ( arrow_l.h / 2 ),
		arrows,
		screen,
		&arrow_l
	);

	apply_surface(
		( ROOM_WIDTH * VIEW_WIDTH + b_room[0][0].x ) * GRID,
		( ( ROOM_HEIGHT * VIEW_HEIGHT + b_room[0][0].y + 1 ) * GRID ) / 2 - ( arrow_r.h / 2 ),
		arrows,
		screen,
		&arrow_r
	);


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
	redraw = true;
	while( quit == false )
	{
		if ( redraw )
		{
			redraw = false;
			for( y = 0; y < VIEW_HEIGHT; y++ )
			{
				for( x = 0; x < VIEW_WIDTH; x++ )
				{
					b_room[x][y].buff = tilemap + ROOM_WIDTH * ROOM_HEIGHT * ( WORLD_WIDTH * ( view_y + y ) + view_x + x );
					apply_surface(b_room[x][y].x * GRID, b_room[x][y].y * GRID, background, screen );
					b_room[x][y].draw( view_x, view_y );
				}
			}
			if( SDL_Flip( screen ) == -1 )
			{
				std::cout << "Error updating screen!\n";
				return 1;
			}
		}
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
					if( cx == 0 )
					{
						if( view_x ){
							redraw = true;
							view_x --;
						}
					}
					else if( cx == ( ROOM_WIDTH * VIEW_WIDTH + b_room[0][0].x ) )
					{
						if( view_x < WORLD_WIDTH - VIEW_WIDTH )
						{
							redraw = true;
							view_x ++;
						}
					}

					if ( cy == 0 )
					{
						if( view_y )
						{
							redraw = true;
							view_y --;
						}
					}
					else if( cy == ( ROOM_HEIGHT * VIEW_HEIGHT + b_room[0][0].y ) )
					{
						if( view_y < WORLD_HEIGHT - VIEW_HEIGHT )
						{
							redraw = true;
							view_y ++;
						}
					}
					
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
				if (rx >= 0 && rx < VIEW_WIDTH && ry >= 0 && ry < VIEW_HEIGHT)
				{
					if ( b_room[rx][ry].tile_place( cx, cy, tid ) )
					{
						std::cout << "Tile placed in room[" << rx << ", " << ry << "]\n";
					}
				}
				lcx = cx;
				lcy = cy;
			}
		}
	}

	SDL_FreeSurface( tileset );
	SDL_Quit();

	/* Save buffer to file */
	std::ofstream outFile( "data.bin", std::ofstream::out | std::ofstream::binary );
	outFile.write( tilemap, buffer_size );

	free(tilemap);
	return 0;
}