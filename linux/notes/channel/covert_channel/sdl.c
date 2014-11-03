#include "SDL.h"
#include <assert.h>
#include <math.h>
#include "config.h"

/* Event information is placed in here */
SDL_Event event;

/* This will be used as our "handle" to the screen surface */
SDL_Surface *scr;

unsigned char *txdata;
unsigned char *rxdata;

static int screen_init(int w, int h)
{
	static int init=0;

	if(init)
		return;
	
    SDL_Init(SDL_INIT_VIDEO);

    /* Get a 640x480, 24-bit software screen surface */
    scr = SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE);
    assert(scr);
	init = 1;
}

unsigned char* get_frame_ptr(void)
{
	unsigned char *ptr;
	static int frame_nr=0;

	if(transmitter)
		ptr = &txdata[frame_nr];
	else
		ptr = &rxdata[frame_nr*DATA_PACKET_SIZE];
	frame_nr++;
	if(frame_nr == 1024){
		frame_nr=0;
		exit(1);
		fprintf(stderr,"!");
	}
	return ptr;
}

void display_init(void)
{
	int fd,x;
	void *ptr;
	char name[64];

	if(!ascii)
		screen_init(PIXEL_WIDTH, PIXEL_HEIGHT);

	if(transmitter)
		sprintf(name,"dattx");
	else
		sprintf(name,"datrx");

	if(playback){
		if ((fd = shm_open(name, O_RDWR,S_IRWXU|S_IRWXG|S_IRWXO)) < 0)
			 DIE("could not open play back file");
	}
	else{

		if ((fd = shm_open(name, O_CREAT|O_RDWR,
					S_IRWXU|S_IRWXG|S_IRWXO)) > 0) {
			//640x480 is 38Kb per frame
			if (ftruncate(fd, DATA_PACKET_SIZE * 1024) != 0)
				DIE("could not truncate shared file\n");
		}
		else
			DIE("Open channel");
	}

	ptr = mmap(NULL,DATA_PACKET_SIZE * 1024,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(ptr == MAP_FAILED)
		DIE("mmap");


	if(transmitter){
		txdata = ptr;
		if(!playback){
			for(x=0;x<1024;x++){
				/* Data source is bitmap */
				memcpy(&txdata[x*DATA_PACKET_SIZE],Untitled_bits,DATA_PACKET_SIZE);
			}
		}
	}
	else{
		rxdata = ptr;
		if(!playback){
			memset(rxdata,0,DATA_PACKET_SIZE * 1024);
		}
	}
}

static void setpixel(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
	Uint32 *pixmem32;
	Uint32 colour;  
	colour = SDL_MapRGB( screen->format, r, g, b );
			   
	pixmem32 = (Uint32*) screen->pixels  + y + x;
	*pixmem32 = colour;
}

int dump_frame(unsigned char *data)
{
    int x, y, t, idx, ytimesw;

	if(ascii)
		return;

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
