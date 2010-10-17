/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* msp.h:                                                                  *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef MSP_H
#define MSP_H 1

#include <glib.h>

#define TELOPT_MSP 90
#define MSPBUFFSIZE  1024 	// enough to store a filename or an url 
#define MSPMAXFILES  16  

typedef struct 	_msp_trigger 	MSP_TRIGGER;
typedef struct 	_msp_info 		MSP_INFO;
typedef enum 	_msp_type 		MSP_TYPE;


enum _msp_type {
	MSP_NULL,
	MSP_SOUND,
	MSP_MUSIC 
};

struct _msp_trigger {
	MSP_TYPE type;
	gchar *fname; 					// file name can be a patern 
	gchar *url;						// 
	gint volume; 					// 0 - 100
	gint priority;					// 0 - 100
	gint loop;						// -1 means infinite loop 
	gint cont;
	gchar *sound_type; 				// whatever is T parameter
	gint pid;						// linux only 
	gchar *filenames[MSPMAXFILES]; 	// files matching the patern pattern 
	gint n;							// filenames length
};

struct _msp_info {
	MSP_TRIGGER *sound;			// current sound
	MSP_TRIGGER *music;  		// current music  
	GThread *tsound;			// the thread which play current sound 
	GThread *tmusic;			// the thread which play current music
	gchar urls[MSPBUFFSIZE];	// the default url for sounds
	gchar urlm[MSPBUFFSIZE];	// the default url for music 
	MSP_TRIGGER *trigger;		// store last trigger before call msp_handle
	gpointer link;				// a pointer to session ... used for sound dir
	gchar buff[MSPBUFFSIZE];
	gsize len;
	gint state;
	
};

MSP_INFO* msp_new( gpointer link );
void msp_free( MSP_INFO *mps );
void msp_process( MSP_INFO *msp, gchar **buff, gsize *len );

#endif // MSP_H
