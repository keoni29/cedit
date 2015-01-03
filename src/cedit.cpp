/* Core Editor - By Koen van Vliet <8by8mail@gmail.com>
 * World editor for Hero Core ti83+ port.
 */
#include "cedit.h"

const int GRID = 16;

const int WORLD_WIDTH = 9;
const int WORLD_HEIGHT = 8;
const int ROOM_WIDTH = 11;
const int ROOM_HEIGHT = 8;

const int SET_WIDTH = 16;
const int SET_HEIGHT = 4;

int VIEW_WIDTH = 3;
int VIEW_HEIGHT = 3;

int SCREEN_WIDTH;
int SCREEN_HEIGHT;
const int SCREEN_BPP = 32;

SDL_Surface *screen = NULL;

SDL_Rect tile_clip( int ix, int w = 1 )
{
	SDL_Rect tilec;

	tilec.x = ( ix % SET_WIDTH ) * GRID;
	tilec.y = ( ix / SET_WIDTH ) * GRID;
	tilec.w = GRID - w;
	tilec.h = GRID - w;

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

void draw_tile( int x, int y, int ix, SDL_Surface *tileset )
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

bool Block::tile_place( int cx, int cy, int ix, SDL_Surface *tileset )
{
	Coord t;

	if ( get_rel_xy( cx, cy, &t ) )
	{
		*( buff + t.y * w + t.x ) = ix;
		::draw_tile( t.x + x, t.y + y, ix, tileset );
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

bool Block::draw( int viewx, int viewy, SDL_Surface *tileset )
{
	int xx, yy;
	char tile;
	for( yy = 0; yy < h; yy++ )
	{
		for( xx = 0; xx < w; xx++ )
		{
			tile = *(buff + ( w * yy + xx ) );
			::draw_tile( xx + x, yy + y, tile, tileset );
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
	char *buffer = NULL;
	char *tilemap = NULL;
	char *worldmap = NULL;
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
	SDL_Surface *tileset = NULL;
	SDL_Surface *tileset_mapscreen = NULL;
	SDL_Rect arrow_u = { 0, 0, 32, 14 };
	SDL_Rect arrow_d = { 0, 33, 32, 14 };
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

	SCREEN_WIDTH = ROOM_WIDTH * VIEW_WIDTH + std::max( SET_WIDTH + 2, WORLD_WIDTH ) + 3;
	SCREEN_HEIGHT = ROOM_HEIGHT * VIEW_HEIGHT + 2;

	buffer_size = ROOM_WIDTH * ROOM_HEIGHT * WORLD_WIDTH * WORLD_HEIGHT;
	buffer = (char *)calloc( buffer_size + 2, sizeof(char) );
	tilemap = buffer + 2;
	*( buffer ) = (char)SCREEN_WIDTH;
	*( buffer + 1 ) = (char)SCREEN_HEIGHT;
	worldmap = (char *)calloc( WORLD_WIDTH * WORLD_HEIGHT, sizeof(char) );

	if( tilemap == NULL || worldmap == NULL )
	{
		std::cout << "Error: Could not allocate memory\n";
		return 1;
	}

	/* Initialize SDL */
	if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}

	SDL_WM_SetIcon( IMG_Load("icon.bmp"), NULL );

	screen = SDL_SetVideoMode( SCREEN_WIDTH * GRID, SCREEN_HEIGHT * GRID, SCREEN_BPP, SDL_SWSURFACE);
	
	if( screen == NULL )
	{
		return 1;
	}

	/* Load assets */
	tileset = load_image( "tileset.bmp" );
	background = load_image( "background.bmp" );
	arrows = load_image( "arrows.bmp" );
	tileset_mapscreen = load_image( "tileset_mapscreen.png" );
	if( tileset == NULL || background == NULL || arrows == NULL || tileset_mapscreen == NULL )
	{
		std::cout << "Error loading image files.\n";
		return 1;
	}

	/* Load buffer from file file */
	std::ifstream inFile( "autosave.core", std::ifstream::out | std::ifstream::binary );
	if (inFile != NULL)
	{
 		inFile.read( tilemap, buffer_size );
 		inFile.close();
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
				ROOM_HEIGHT
			};
		}
	}


	b_picker = {
		NULL,
		SCREEN_WIDTH - std::max( SET_WIDTH + 2, WORLD_WIDTH ) - 1,
		1,
		SET_WIDTH,
		SET_HEIGHT
	};

	b_map = {
		worldmap,
		SCREEN_WIDTH - std::max( SET_WIDTH + 2, WORLD_WIDTH ) - 1,
		( SET_HEIGHT + 2 ),
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
			SDL_Rect worldc = {
				b_map.x * GRID - 1,
				b_map.y * GRID - 1,
				WORLD_WIDTH * GRID + 1,
				WORLD_HEIGHT * GRID + 1
			};
			SDL_Rect viewc = {
				( b_map.x + view_x ) * GRID - 1,
				( b_map.y + view_y ) * GRID - 1,
				VIEW_WIDTH * GRID + 1,
				VIEW_HEIGHT * GRID + 1
			};
			redraw = false;
			for( y = 0; y < VIEW_HEIGHT; y++ )
			{
				for( x = 0; x < VIEW_WIDTH; x++ )
				{
					b_room[x][y].buff = tilemap + ROOM_WIDTH * ROOM_HEIGHT * ( WORLD_WIDTH * ( view_y + y ) + view_x + x );
					apply_surface(b_room[x][y].x * GRID, b_room[x][y].y * GRID, background, screen );
					b_room[x][y].draw( view_x, view_y, tileset );
				}
			}
			SDL_FillRect( screen, &worldc, SDL_MapRGB( screen->format, 0x00, 0x00, 0x00 ) );
			SDL_FillRect( screen, &viewc, SDL_MapRGB( screen->format, 0xFF, 0xFF, 0xFF ) ); 
			b_map.draw( 0, 0, tileset_mapscreen );
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
						// Succesfully placed tile
					}
				}

				if( b_picker.get_rel_xy( cx, cy, &t ) )
					{
						tile = t.i;
						draw_tile( b_picker.x + SET_WIDTH + 1, b_picker.y, t.i, tileset );
						if( SDL_Flip( screen ) == -1 )
						{
							std::cout << "Error updating screen!\n";
							return 1;
						}
						std::cout << "Picked tile with id " << tile << ".\n";
					}
				if( b_map.get_rel_xy( cx, cy, &t ) ){
					redraw = true;
					view_x = std::min( std::max( t.x - 1, 0 ), WORLD_WIDTH - VIEW_WIDTH );
					view_y = std::min( std::max( t.y - 1, 0 ), WORLD_HEIGHT - VIEW_HEIGHT );
				}

				lcx = cx;
				lcy = cy;
			}
		}
	}

	SDL_FreeSurface( tileset );
	SDL_Quit();

	/* Save buffer to file */
	std::ofstream outFile( "autosave.core", std::ofstream::out | std::ofstream::binary );
	outFile.write( tilemap, buffer_size );
	outFile.close();
	std::ofstream exportFile( "out.cedit", std::ofstream::out | std::ofstream::binary );
	exportFile.write( buffer, buffer_size + 2 );
	exportFile.close();
	char* args_to8xv[] = {
		(char *)"to8xv",
		(char *)"out.cedit",
		(char *)"Untitled_Hero_Core_Tilemap.8xv",
		(char *)"HCMT"
	};
	if( execv( "to8xv", args_to8xv ) == -1 )
	{
		std::cout << "Warning: Could not convert to appvar.";
		return 1;
	}
	free(buffer);
	return 0;
}