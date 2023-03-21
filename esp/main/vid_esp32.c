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
#include "../../../core/quakedef.h"
#include "../../../core/render/original/d_local.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include <driver/spi_master.h>

#include "common.h"

extern void lcd_init(uint16_t xres, uint16_t yres);

unsigned short  d_8to16table[256];

viddef_t vid;

uint8_t *vid_2;

// esp>idf.py build && idf.py size-components && idf.py flash -p com4
// idf.py build && idf.py size && idf.py flash -p com4
// xtensa-esp32-elf-size -A -d build/quake.elf
// idf.py monitor
// xtensa-esp32-elf-addr2line -pfiaC -e build/quake.elf <address>

/*
esp has:
328 k DRAM
192 k IRAM
 16 k RTC RAM


1:
I (770) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (776) heap_init: At 3FFD9CC8 len 00006338 (24 KiB): DRAM
I (782) heap_init: At 3FFE0440 len 0001FBC0 (126 KiB): D/IRAM
I (788) heap_init: At 40078000 len 00008000 (32 KiB): IRAM
I (794) heap_init: At 4009600C len 00009FF4 (39 KiB): IRAM   (Can be used for static defines)

I (770) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (776) heap_init: At 3FFD9350 len 00006CB0 (27 KiB): DRAM
I (782) heap_init: At 3FFE0440 len 0001FBC0 (126 KiB): D/IRAM
I (789) heap_init: At 40078000 len 00008000 (32 KiB): IRAM
I (795) heap_init: At 40096184 len 00009E7C (39 KiB): IRAM
*/

/*
now:
vid  <dynamic>: 30720 (30 k)
zbuf <static> : 61440 (60 k)
90 k

 76800 ( 75 k)
153600 (150 k)
225k

*/

void VID_SetPalette (unsigned char *palette)
{
    for (uint32_t i = 0; i < 256; i++) {
    	uint8_t r = (*palette++ >>  3) & 31;
    	uint8_t g = (*palette++ >>  2) & 63;
    	uint8_t b = (*palette++ >>  3) & 31;
    	uint16_t tmp = r << 11 | g << 5 | b;
		// TODO: Make it 8-bit at all times, this is a pain in the ass...
    	d_8to16table[i] = (tmp << 8 | tmp >> 8);
    }
}

void VID_Init (unsigned char *palette)
{
	static short zbuffer[BASEWIDTH*BASEHEIGHT];
	// static byte dispbuffer[BASEWIDTH*BASEHEIGHT];

	printf("=== Video Init ===\n");
	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = heap_caps_malloc(BASEWIDTH*BASEHEIGHT, MALLOC_CAP_8BIT);
	// vid.buffer = vid.conbuffer = (pixel_t*)dispbuffer;
	// d_pzbuffer = (uint16_t*)((uint32_t)heap_caps_malloc((BASEWIDTH*BASEHEIGHT*2)+1, MALLOC_CAP_8BIT) & ~1);
	d_pzbuffer = zbuffer;

	if (!vid.buffer || !d_pzbuffer)
		Sys_Error("Could not allocate video buffer\n");

    // vid_2 = malloc(BASEWIDTH*BASEHEIGHT);
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;

	// D_InitCaches (heap_caps_malloc(SURFCACHE_SIZE, MALLOC_CAP_INTERNAL), SURFCACHE_SIZE);

    VID_SetPalette(palette);

	// lcd_init(DISPLAYWIDTH, DISPLAYHEIGHT);
}

extern void lcdStartDMA(void);

void VID_Update (vrect_t *rects)
{
    // Status bar is only updated as need be. As such we must keep two copies to prevent the annoying back and forth between old and new updates
    // memcpy(&vid_2[DISPLAYWIDTH*(DISPLAYHEIGHT-48)], &vid.buffer[DISPLAYWIDTH*(DISPLAYHEIGHT-48)], DISPLAYWIDTH*48/*DISPLAYHEIGHT*/);
/*
    // Switch buffers
    void *tmp = vid.buffer;
    vid.buffer = vid.conbuffer = (void*)vid_2;
    vid_2 = (uint8_t*)tmp;

    lcdStartDMA();
*/
    // heap_caps_dump_all();
}

void VID_Shutdown (void) {}
void VID_ShiftPalette (unsigned char *palette) { VID_SetPalette(palette); }
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height) {}
void D_EndDirectRect (int x, int y, int width, int height) {}
