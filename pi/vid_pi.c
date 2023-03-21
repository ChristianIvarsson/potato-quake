/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// vid_null.c -- null video driver to aid porting efforts

#include "../core/quakedef.h"

// #include "../core/render/orig/render.h"


// max_hunk 3789
// max_hunk 3763
// max_hunk 3762


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>


static uint16_t *fbp = 0;
static int       fbfd = -1;
static size_t    fbScrSize = 0;

viddef_t vid; // global video state

#define BASEWIDTH 320
#define BASEHEIGHT 240

byte vid_buffer[BASEWIDTH * BASEHEIGHT];
short zbuffer[BASEWIDTH * BASEHEIGHT];

#ifndef NOSURFCACHE
byte surfcache[640 * 1024];
#endif

unsigned short d_8to16table[256];
// unsigned d_8to24table[256];

void VID_Update(vrect_t *rects) {
	for (int i = 0; i < (BASEWIDTH * BASEHEIGHT); i++)
		fbp[i] = d_8to16table[ vid_buffer[i] ];
}

void VID_SetPalette(unsigned char *palette) {
    for (uint32_t i = 0; i < 256; i++) {
    	uint8_t r = (*palette++ >>  3) & 31;
    	uint8_t g = (*palette++ >>  2) & 63;
    	uint8_t b = (*palette++ >>  3) & 31;
    	d_8to16table[i] = (r << 11) | (g << 5) | b;
    }
}

void VID_ShiftPalette(unsigned char *palette) {
	VID_SetPalette(palette);
}

void VID_Init(unsigned char *palette) {
    struct fb_var_screeninfo vinfo, finfo;

	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vid_buffer;
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;

	d_pzbuffer = zbuffer;

#ifndef NOSURFCACHE
	D_InitCaches(surfcache, sizeof(surfcache));
#endif

	VID_SetPalette(palette);

    fbfd = open("/dev/fb1", O_RDWR);
    if (fbfd == -1) {
        perror("opening /dev/fb0");
        exit(-1);
    }

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
        exit(-2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
        exit(-3);
    }

    // Figure out the size of the screen in bytes
    fbScrSize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);

    // Map the device to memory
    fbp = (uint16_t *)mmap(0, fbScrSize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit(-4);
    }
}

void VID_Shutdown(void) {
    munmap(fbp, fbScrSize);
    close(fbfd);
}

void D_BeginDirectRect(int x, int y, byte *pbitmap, int width, int height) {}
void D_EndDirectRect(int x, int y, int width, int height) {}
