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
// sys_null.h -- null system driver to aid porting efforts

#include "../../../core/quakedef.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#include <errno.h>
#include <time.h>


/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
static FILE    *sys_handles[MAX_HANDLES];

int             findhandle (void)
{
	int             i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int             pos;
	int             end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (const char *path, int *hndl)
{
	FILE    *f;
	int             i;
	
	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;
	
	return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE    *f;
	int             i;
	
	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;
	
	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, const void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int Sys_FileTime (char *path)
{
	FILE    *f;
	
	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}
	
	return -1;
}

void Sys_mkdir (char *path)
{
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{}

void Sys_Error (const char *error, ...)
{
	va_list argptr;
	printf ("Sys_Error: ");   
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	exit (1);
}

void Sys_Printf (const char *fmt, ...)
{
	va_list argptr;
	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
}

void Sys_Quit (void)
{
	exit (0);
}

double Sys_FloatTime (void)
{
	// A race condition happens when it tries to read the adjusted time.
	// Since we only have to know time since last capture, there's no need to be that exact.
	return (double)((uint64_t)esp_time_impl_get_time_since_boot()) / 1000000.0;
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

void Sys_SendKeyEvents (void)
{

}



void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}


//=============================================================================

#define PSRAM_ADDRESS (0x3F800000)
#define PSRAM_SIZE    (4 * 1024 * 1024)

// Had som weird issues with PSRAM
static void testWritePsram(void) {
	uint8_t *ptr = (uint8_t*)PSRAM_ADDRESS;
	printf("Now attempting write of psram..\n");

	for (int i = 0; i < (4 * 1024 * 1024); i++)
		ptr[i] = (uint8_t)i;
	for (int i = 0; i < (4 * 1024 * 1024); i++)
		if (ptr[i] != (uint8_t)i)
			Sys_Error("Data error in psram");

	for (int i = 0; i < (4 * 1024 * 1024); i++)
		ptr[i] = 0;
	for (int i = 0; i < (4 * 1024 * 1024); i++)
		if (ptr[i] != 0)
			Sys_Error("Data error in psram");

	printf("We still alive? -Think it passed...\n");
}

int quake_main (int argc, char **argv, int memsize)
{
	static quakeparms_t    parms;
	double		time, oldtime, newtime;


#ifdef TRACE_STACK
	double stackTime;
	COM_InitStackTrace(&stackTime);
#endif

	parms.basedir = ".";

	parms.memsize = PSRAM_SIZE;
	parms.membase = (void*)PSRAM_ADDRESS;

	testWritePsram();

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	Host_Init (&parms);

    oldtime = Sys_FloatTime () - 0.1;
#ifdef TRACE_STACK
	stackTime = oldtime;
#endif

    while (1)
    {
// find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;

/*
        if (cls.state == ca_dedicated)
        {   // play vcrfiles at max speed
            if (time < sys_ticrate.value && (vcrFile == -1 || recording) )
            {
				usleep(1);
                continue;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }
*/
        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        Host_Frame (time);

#ifdef TRACE_STACK
		if ((newtime - stackTime) >= 1)
		{
			stackTime = newtime;
			printf("Max Depth: %lu\n", COM_GetMaxStack());
		}
#endif

/*
// graphic debugging aids
        if (sys_linerefresh.value)
            Sys_LineRefresh ();*/
    }





	return 0;
}


