#include "SDL.h"
#include <assert.h>
#include <math.h>
#include "config.h"

/* Event information is placed in here */
SDL_Event event;

/* This will be used as our "handle" to the screen surface */
SDL_Surface *scr;

unsigned char data[DATA_PACKET_SIZE];

unsigned char* get_frame_ptr(void)
{
	return data;
}

static int screen_init(int w, int h)
{
	static int init=0;

	if(init)
		return;
	if(ascii)
		return;
    SDL_Init(SDL_INIT_VIDEO);

    /* Get a 640x480, 24-bit software screen surface */
    scr = SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE);
    assert(scr);
	init = 1;
}

static void rx_init(void)
{
	screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);
}

static void tx_init(void)
{	
	screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);
	/* Data source is bitmap */
	memcpy(data,Untitled_bits,DATA_PACKET_SIZE);
	screen_dump(Untitled_bits);
}

void display_init(void)
{
	if(transmitter)
		tx_init();
	else
		rx_init();

}

static void setpixel(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
	Uint32 *pixmem32;
	Uint32 colour;  
	colour = SDL_MapRGB( screen->format, r, g, b );
			   
	pixmem32 = (Uint32*) screen->pixels  + y + x;
	*pixmem32 = colour;
}

int screen_dump(unsigned char *data)
{
    int x, y, t, idx, ytimesw;

    /* Ensures we have exclusive access to the pixels */
    SDL_LockSurface(scr);
	/* Clear the raster */
	for(y = 0; y < scr->h; y++){
		for(x = 0; x < scr->w; x++){
			ytimesw = y*scr->pitch/4;
			setpixel(scr, x, ytimesw, 0,0,0);
		}
	}

	for(y = 0; y < scr->h; y++)
		for(x = 0; x < scr->w; x=x+8){
			idx = (x+(y*(scr->w)))/8;
			ytimesw = y*scr->pitch/4;
			if ((data[idx] >>0) &0x1)
				setpixel(scr, x+0, ytimesw, 0xff,0,0);
			if ((data[idx] >>1) &0x1)
				setpixel(scr, x+1, ytimesw, 0xff,0,0);
			if ((data[idx] >>2) &0x1)
				setpixel(scr, x+2, ytimesw, 0xff,0,0);
			if ((data[idx] >>3) &0x1)
				setpixel(scr, x+3, ytimesw, 0xff,0,0);
			if ((data[idx] >>4) &0x1)
				setpixel(scr, x+4, ytimesw, 0xff,0,0);
			if ((data[idx] >>5) &0x1)
				setpixel(scr, x+5, ytimesw, 0xff,0,0);
			if ((data[idx] >>6) &0x1)
				setpixel(scr, x+6, ytimesw, 0xff,0,0);
			if ((data[idx] >>7) &0x1)
				setpixel(scr, x+7, ytimesw, 0xff,0,0);
/*
			fprintf(stderr,"%d %d %d %d %d %d %d %d %d %d %d\n",x,y,idx, 
				((data[idx] >>0) &0x1)*255,
				((data[idx] >>1) &0x1)*255,
				((data[idx] >>2) &0x1)*255,
				((data[idx] >>3) &0x1)*255,
				((data[idx] >>4) &0x1)*255,
				((data[idx] >>5) &0x1)*255,
				((data[idx] >>6) &0x1)*255,
				((data[idx] >>7) &0x1)*255);
*/

	}
    SDL_UnlockSurface(scr);

    /* Copies the `scr' surface to the _actual_ screen */
    SDL_UpdateRect(scr, 0, 0, 0, 0);

#if 0
    /* Now we wait for an event to arrive */
    while(SDL_WaitEvent(&event))
    {
    	/* Any of these event types will end the program */
    	if (event.type == SDL_QUIT
    	 || event.type == SDL_KEYDOWN
    	 || event.type == SDL_KEYUP)
    		break;
    }
    SDL_Quit();
#endif

    return EXIT_SUCCESS;
}
