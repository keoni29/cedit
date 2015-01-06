/* Core Editor - By Koen van Vliet <8by8mail@gmail.com>
 * World editor for Hero Core ti83+ port.
 *
 * Known bugs:
 * - Click mouse without releasing the mouse button, then alt-tab out. Now the editor 
 *	 is stuck in the mouse-down state until the mouse is clicked in the editor window again.
 */

/* Summary:
 * 1: Include files
 * 2: Functions for loading images and drawing graphics to the screen
 * 3: Editor block object functions
 * 4: Main
 * 4,1: Process command-line options
 * 4,2: Initialize graphics subsystem, editor and load images
 * 4,3: Load buffer from autosave file
 * 4,4: Draw various static graphical elements on screen 
 * 4,5: Handle events
 *		- Navigation
 *		- Placing tiles
 *		- Removing tiles
 *		- Quit on window close
 * 4,5: Quit graphics subsystem
 * 4,6: Save buffer to autosave file
 * 4,7: Export to 8xv
 */

/* 1: Include files */
#include <algorithm>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "cedit.h"

#define _FILE_VERSION 1

const SDL_Color c_white = { 0xFF, 0xFF, 0xFF };
const SDL_Color c_black = { 0x00, 0x00, 0x00 };

// Yet to fix this global
int GRID;

/* 2: Functions for loading images and drawing graphics to the screen */
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

void draw_rectangle( int x, int y, int w, int h, SDL_Color c, SDL_Surface *screen )
{
	SDL_Rect r[4];
	int i;
	r[0] = { 
		x * GRID,
		y * GRID,
		w * GRID,
		1
	};
	r[1] = {
		x * GRID,
		( y + h ) * GRID - 1 ,
		w * GRID,
		1
	};
	r[2] = {
		x * GRID,
		y * GRID,
		1,
		h * GRID
	};
	r[3] = {
		( x + w ) * GRID - 1,
		y * GRID,
		1,
		h * GRID
	};
	for( i = 0; i < sizeof(r) / sizeof(*r); i++ )
	{
		SDL_FillRect( screen, &r[i], SDL_MapRGB( screen->format, c.r, c.g, c.b ) );
	}
}

void draw_tile( int x, int y, int ix, SDL_Surface *tileset, SDL_Surface *screen )
{
	SDL_Rect tilec;

	tilec.x = ( ix % (tileset->w / GRID ) ) * GRID;
	tilec.y = ( ix / (tileset->w / GRID ) ) * GRID;
	tilec.w = GRID - 1;
	tilec.h = GRID - 1;

	apply_surface( x * GRID, y * GRID, tileset, screen, &tilec );
}

/* 3: Editor block object functions */
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

bool Block::tile_place( int cx, int cy, int ix, SDL_Surface *tileset )
{
	Coord t;

	if ( get_rel_xy( cx, cy, &t ) )
	{
		if( *( buff + t.y * w + t.x ) != ix )
		{	
			*( buff + t.y * w + t.x ) = ix;
			::draw_tile( t.x + x, t.y + y, ix, tileset, screen );
			if( SDL_Flip( screen ) == -1 )
			{
				return false;
			}
			return true;
		}
	}
	return false;
}

bool Block::draw( int viewx, int viewy, SDL_Surface *tileset, bool border = false )
{
	int xx, yy;
	char tile;
	SDL_Rect backgc;
	SDL_Rect backgic;
	if( border )
	{
		backgc = { x * GRID - 1, y * GRID - 1, w * GRID + 1, h * GRID + 1 };
		backgic = {	x * GRID, y * GRID, w * GRID - 1, h * GRID - 1 };
		SDL_FillRect( screen, &backgc, SDL_MapRGB( screen->format, 0xFF, 0xFF, 0xFF ) );
		SDL_FillRect( screen, &backgic, SDL_MapRGB( screen->format, 0x00, 0x00, 0x00 ) );
	}
	for( yy = 0; yy < h; yy++ )
	{
		for( xx = 0; xx < w; xx++ )
		{
			tile = *(buff + ( w * yy + xx ) );
			::draw_tile( xx + x, yy + y, tile, tileset, screen );
		}
	}
	return true;
}

/* 4: Main */
int main( int argc, char *args[] )
{
	Block b_picker;
	Block b_map;

	bool mdl = false, mdr = false;
	bool redraw;
	bool quit = false;

	char *buffer = NULL;
	char *tilemap = NULL;
	char *worldmap = NULL;

	char *inFileName = NULL;
	char *outFileName = NULL;
	char *exportFileName = NULL;
	char *varName = NULL;
	char *tilesetFileName = NULL;

	const int SCREEN_BPP = 32;

	int buffer_size;
	int cx, cy;
	int lcx, lcy;
	int rx, ry;
	int tile = 1;
	int view_x = 0, view_y = 0;
	int x, y;

	GRID = 16;
	int VERSION = _FILE_VERSION;
	int ROOM_WIDTH = 0;
	int ROOM_HEIGHT = 0;
	int VIEW_WIDTH = 0;
	int VIEW_HEIGHT = 0;
	int WORLD_WIDTH = 0;
	int WORLD_HEIGHT = 0;

	int SET_WIDTH;
	int SET_HEIGHT;
	int SCREEN_WIDTH;
	int SCREEN_HEIGHT;

	int header_size = 200;

	SDL_Event event;
	SDL_Rect arrow_d;
	SDL_Rect arrow_l;
	SDL_Rect arrow_u;
	SDL_Rect arrow_r;
	SDL_Rect worldc;
	SDL_Rect viewc;
	SDL_Surface *arrows = NULL;
	SDL_Surface *tileset = NULL;
	SDL_Surface *tileset_mapscreen = NULL;
	SDL_Surface *screen = NULL;

/*
	7 size parameters,
	-g=??    (2-255)
	-r=??x?? (1-255)
	-w=??x?? (1-255)
	-v=??x?? (1-255)
	4 name parameters
	-i=?? (max 32 long)
	-o=?? (max 32 long)
	-e=?? (max 32 long)
	-v=?? (max 8 long)
*/
	
	/* 4,1: Process command-line options */
	char option_char;
	while( ( option_char = getopt( argc, args, "i:o:a:e:t:f:g:r:R:w:W:v:V:?h" ) ) != -1 )
	{
		switch( option_char )
		{
			case 'i': inFileName = optarg; break;
			case 'o': outFileName = optarg; break;
			case 'a': varName = optarg; break;
			case 'e': exportFileName = optarg; break;
			case 't': tilesetFileName = optarg; break;

			case 'g': GRID = atoi( optarg ); break;
			case 'r': ROOM_WIDTH = atoi( optarg ); break;
			case 'R': ROOM_HEIGHT = atoi( optarg ); break;
			case 'w': WORLD_WIDTH = atoi( optarg ); break;
			case 'W': WORLD_HEIGHT = atoi( optarg ); break;
			case 'v': VIEW_WIDTH = atoi( optarg ); break;
			case 'V': VIEW_HEIGHT = atoi( optarg ); break;

			case 'h':
			case '?': std::cout << "Usage: " << args[0] << "[OPTIONS]" << ".\n"
								<< "-i filename\t\tLoad file\n"
								<< "-o filename\t\tSave file\n"
								<< "-e filename\t\tExport name\n"
								<< "-a name\tAppvar name\n"
								<< "-t filename\t\tLoad tileset from image\n"
								<< "\n"
								<< "-g grid_size\t(2-255)\tGrid size in pixels\n"
								<< "-r room_width\t(1-255)\tSet room width\n"
								<< "-R room_height\t(1-255)\tSet room height\n"
								<< "-w world_width\t(1-255)\tSet world width\n"
								<< "-W world_height\t(1-255)\tSet world height\n"
								<< "-v view_width\t(1-255)\tSet view width\n"
								<< "-V view_height\t(1-255)\tSet view height\n";
								return 1;
		}
	}

	if( inFileName == NULL )
	{
		if( ROOM_WIDTH == 0 )
		{
			ROOM_WIDTH = 11;
			ROOM_HEIGHT = 8;
			std::cout << "Room width not specified. Using defaults:" << ROOM_WIDTH << " x " << ROOM_HEIGHT << ".\n";
		}

		if( WORLD_WIDTH == 0 )
		{
			WORLD_WIDTH = 9;
			WORLD_HEIGHT = 8;
			std::cout << "World width not specified. Using default world size:" << WORLD_WIDTH << " x " << WORLD_HEIGHT << ".\n";
		}
	}
	
	/* 4,2: Initialize graphics subsystem, editor and load images */

	if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}

	if( tilesetFileName == NULL )
	{
		tilesetFileName = (char *)"tileset.bmp";
	}
	tileset = SDL_LoadBMP( tilesetFileName );
	if( tileset == NULL )
	{
		std::cout << "Error loading tileset.\n";
		return 1;
	}
	SET_WIDTH = tileset->w / GRID;
	SET_HEIGHT = tileset->h / GRID;
	SDL_FreeSurface( tileset );

	if( SET_WIDTH * SET_HEIGHT > 256 )
	{
		std::cout << "Warning: Too many tiles. Try using a smaller image or a smaller grid size!\n";
	}

	/* 4,3: Load buffer from file */
	std::ifstream inFile( inFileName, std::ifstream::out | std::ifstream::binary );
	if( inFileName != NULL )
	{
		buffer = (char *)calloc( header_size, sizeof(char) );
		if( buffer == NULL )
		{
			std::cout << "Error: Cound not allocate memory.\n";
			return 1;
		}

		if (inFile != NULL)
		{
	 		inFile.read( buffer, header_size );
		}
		VERSION = *( buffer );
		GRID = *( buffer + 1 );
		ROOM_WIDTH = *( buffer + 2 );
		ROOM_HEIGHT = *( buffer + 3 );
		WORLD_WIDTH = *( buffer + header_size - 2 );
		WORLD_HEIGHT = *( buffer + header_size - 1 );
		free(buffer);
	}
	
	if( VIEW_WIDTH == 0 )
	{
		VIEW_WIDTH = std::min( WORLD_WIDTH, 3 );
		VIEW_HEIGHT = std::min( WORLD_HEIGHT, 3 );
		std::cout << "View width not specified. Using default view size:" << VIEW_WIDTH << " x " << VIEW_HEIGHT << ".\n";
	}

	// Reserve memory for the world editor and the map screen
	Block b_room[VIEW_WIDTH][VIEW_HEIGHT];

	buffer_size = ROOM_WIDTH * ROOM_HEIGHT * WORLD_WIDTH * WORLD_HEIGHT;
	buffer = (char *)calloc( buffer_size + header_size, sizeof(char) );
	worldmap = (char *)calloc( WORLD_WIDTH * WORLD_HEIGHT, sizeof(char) );
	tilemap = buffer + header_size;

	if( inFile != NULL )
	{
		inFile.read( tilemap, buffer_size );
		inFile.close();
	}

	VIEW_WIDTH = std::min( WORLD_WIDTH, VIEW_WIDTH );
	VIEW_HEIGHT = std::min( WORLD_HEIGHT, VIEW_HEIGHT );

	std::cout << "GRID_SIZE = " << GRID << " * " << GRID << " pixels\n";
	std::cout << "ROOM_WIDTH = " << ROOM_WIDTH << " tiles\n";
	std::cout << "ROOM_HEIGHT = " << ROOM_HEIGHT << " tiles\n";
	std::cout << "WORLD_WIDTH = " << WORLD_WIDTH << " rooms\n";
	std::cout << "WORLD_HEIGHT = " << WORLD_HEIGHT << " rooms\n";
	std::cout << "VIEW_WIDTH = " << VIEW_WIDTH << " rooms\n";
	std::cout << "VIEW_HEIGHT = " << VIEW_HEIGHT << " rooms\n";

	SCREEN_WIDTH = ROOM_WIDTH * VIEW_WIDTH + std::max( SET_WIDTH + 2, WORLD_WIDTH ) + 3;
	SCREEN_HEIGHT = std::max( ROOM_HEIGHT * VIEW_HEIGHT, SET_HEIGHT + 1 + WORLD_HEIGHT ) + 2;

	if( buffer == NULL || tilemap == NULL || worldmap == NULL )
	{
		std::cout << "Error: Could not allocate memory.\n";
		return 1;
	}

	SDL_WM_SetIcon( IMG_Load("icon.bmp"), NULL );

	screen = SDL_SetVideoMode( SCREEN_WIDTH * GRID, SCREEN_HEIGHT * GRID, SCREEN_BPP, SDL_SWSURFACE);
	
	if( screen == NULL )
	{
		return 1;
	}

	/* Load assets */
	tileset = load_image( tilesetFileName );
	arrows = load_image( "arrows.bmp" );
	tileset_mapscreen = load_image( "tileset_mapscreen.png" );
	if( tileset == NULL || arrows == NULL || tileset_mapscreen == NULL )
	{
		std::cout << "Error loading image files.\n";
		return 1;
	}

	SDL_WM_SetCaption( "Core Editor - By Koen van Vliet", NULL);

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
				ROOM_HEIGHT,
				screen
			};
		}
	}

	b_picker = {
		NULL,
		SCREEN_WIDTH - std::max( SET_WIDTH + 2, WORLD_WIDTH ) - 1,
		1,
		SET_WIDTH,
		SET_HEIGHT,
		screen
	};

	b_map = {
		worldmap,
		SCREEN_WIDTH - std::max( SET_WIDTH + 2, WORLD_WIDTH ) - 1,
		( SET_HEIGHT + 2 ),
		WORLD_WIDTH,
		WORLD_HEIGHT,
		screen
	};

	worldc = {
		b_map.x * GRID - 1,
		b_map.y * GRID - 1,
		WORLD_WIDTH * GRID + 1,
		WORLD_HEIGHT * GRID + 1
	};

	viewc = {
		( b_map.x + view_x ) * GRID - 1,
		( b_map.y + view_y ) * GRID - 1,
		VIEW_WIDTH * GRID + 1,
		VIEW_HEIGHT * GRID + 1
	};

	/* 4,4: Draw various static graphical elements on screen */
	arrow_u = { 0, 0, 32, 14 };
	arrow_d = { 0, 33, 32, 14 };
	arrow_l = { 0, 14, 16, 20 };
	arrow_r = { 16, 14, 16, 20 };

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

	redraw = true;
	while( quit == false )
	{
		/* Redraw world editor and map screen */
		if ( redraw )
		{
			redraw = false;

			// Draw tile picker
			apply_surface(b_picker.x * GRID, b_picker.y * GRID, tileset, screen );
			draw_rectangle( b_picker.x + tile % SET_WIDTH, b_picker.y + tile / SET_WIDTH, 1, 1, c_white, screen );

			// Draw world editor
			for( y = 0; y < VIEW_HEIGHT; y++ )
			{
				for( x = 0; x < VIEW_WIDTH; x++ )
				{
					b_room[x][y].buff = tilemap + ROOM_WIDTH * ROOM_HEIGHT * ( WORLD_WIDTH * ( view_y + y ) + view_x + x );
					b_room[x][y].draw( view_x, view_y, tileset, true );
				}
			}

			viewc.x = ( b_map.x + view_x ) * GRID - 1;
			viewc.y = ( b_map.y + view_y ) * GRID - 1;

			SDL_FillRect( screen, &worldc, SDL_MapRGB( screen->format, 0x00, 0x00, 0x00 ) );
			SDL_FillRect( screen, &viewc, SDL_MapRGB( screen->format, 0xFF, 0xFF, 0xFF ) ); 
			
			b_map.draw( 0, 0, tileset_mapscreen );

			if( SDL_Flip( screen ) == -1 )
			{
				std::cout << "Error updating screen!\n";
				return 1;
			}
		}

		/* 4,5 Handle events and perform actions such as placing tiles
		 * changing the view coordinates etc.
		 */
		while( SDL_PollEvent( &event ) )
		{
			cx = event.button.x / GRID;
			cy = event.button.y / GRID;
			if( event.type == SDL_MOUSEBUTTONDOWN )
			{
				if( event.button.button == SDL_BUTTON_LEFT )
				{
					mdl = true;
					Coord t;

					// Pick tiles from set
					if( b_picker.get_rel_xy( cx, cy, &t ) )
					{
						redraw = true;
						tile = t.i;
						draw_tile( b_picker.x + SET_WIDTH + 1, b_picker.y, t.i, tileset, screen );
						if( SDL_Flip( screen ) == -1 )
						{
							std::cout << "Error updating screen!\n";
							return 1;
						}
						std::cout << "Picked tile with id " << tile << ".\n";
					}

					/* Scrolling trough the world when clicked 1 tile outside of the room editor */
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
			/* Clicking and dragging */
			if( ( event.type == SDL_MOUSEMOTION && ( mdl || mdr ) && ( cx != lcx || cy != lcy  ) ) || ( event.type == SDL_MOUSEBUTTONDOWN ) )
			{
				Coord txy;
				int tid, offs;
				Coord t;

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
					if ( b_room[rx][ry].tile_place( cx, cy, tid, tileset ) )
					{
						std::cout << "Placed Tile! \n";
					}
				}

				if( b_map.get_rel_xy( cx, cy, &t ) )
				{
					if( event.button.button == SDL_BUTTON_LEFT )
					{
						redraw = true;
						view_x = std::min( std::max( t.x - ( VIEW_WIDTH / 2 ), 0 ), WORLD_WIDTH - VIEW_WIDTH );
						view_y = std::min( std::max( t.y - ( VIEW_HEIGHT / 2 ), 0 ), WORLD_HEIGHT - VIEW_HEIGHT );
					}
				}

				lcx = cx;
				lcy = cy;
			}
		}
	}

	/* 4,5: Quit graphics subsystem */
	SDL_FreeSurface( tileset );
	SDL_FreeSurface( arrows );
	SDL_FreeSurface( tileset_mapscreen );
	SDL_Quit();

	/* 4,6: Save buffer to file */
	if( outFileName == NULL || sizeof( outFileName ) <= 1 )
	{
		outFileName = (char *)"autosave.core";
	}

	if( varName == NULL || sizeof( varName ) <= 1 )
	{
		outFileName = (char *)"autosave.core";
	}
	*( buffer ) = VERSION;
	*( buffer + 1 ) = (char)GRID;
	*( buffer + 2 ) = (char)ROOM_WIDTH;
	*( buffer + 3 ) = (char)ROOM_HEIGHT;
	*( buffer + header_size - 2 ) = (char)WORLD_WIDTH;
	*( buffer + header_size - 1 ) = (char)WORLD_HEIGHT;
	std::ofstream outFile( "autosave.core", std::ofstream::out | std::ofstream::binary );
	outFile.write( buffer, buffer_size + header_size );
	outFile.close();

	/* 4,7: Export to 8xv */
	if( exportFileName != NULL )
	{
		std::ofstream exportFile( "out.cedit", std::ofstream::out | std::ofstream::binary );
		exportFile.write( tilemap - 2 , buffer_size + 2 );
		exportFile.close();
		char* args_to8xv[] = {
			(char *)"to8xv",
			outFileName,
			exportFileName,
			varName,
		};
		if( execv( "to8xv", args_to8xv ) == -1 )
		{
			std::cout << "Warning: Could not convert to appvar.";
			return 1;
		}
	}
	free(buffer);
	return 0;
}