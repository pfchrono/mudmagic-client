/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* telnet.h:                                                               *
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
#ifndef __TELNET_H_
#define __TELNET_H_
#include <glib.h>
#include <zlib.h>

//#include "mudmagic.h"

#define	IAC		255
#define	DONT	254
#define	DO		253
#define	WONT	252
#define	WILL	251
#define	SB		250
#define	SE		240

#define TELOPT_ECHO			1
#define TELOPT_END_OF_RECORD            25
#define	TELOPT_NAWS			31
#define TELOPT_COMPRESS 	85
#define TELOPT_COMPRESS2 	86
#define TELOPT_TTYPE 		24  
#define TELQUAL_IS 		0 

#define ESC 27

#define BUFFSIZE 2048
enum {
	TN_STATE_NORMAL,
	TN_STATE_ANSI,		// after ESC
	TN_STATE_IAC,		// after IAC 
	TN_STATE_WILL,		// after IAC WILL
	TN_STATE_WONT,		// after IAC WONT
	TN_STATE_DO,		// after IAC DO
	TN_STATE_DONT,		// after IAC DONT
	TN_STATE_SB,		// after IAC SB         
	TN_STATE_SB_IAC		// after IAC SB ... IAC         state SE
};

struct MXPINFO;

typedef struct _TelnetState TelnetState;
typedef TelnetState TELNET_STATE;

struct _TelnetState
{
	guchar rbuff[BUFFSIZE];	// this buffer will store data before decompression
	guchar dbuff[BUFFSIZE];	// this buffer will store data before processing
	guchar ubuff[BUFFSIZE];	// for broked ansi command / subnegotiation data
	gsize rlen, dlen, ulen;	// the number of bytes from every buffer
	gsize dpos;		// the processed data indicator

	gint state;
	gint fd;

	// compress 
	z_stream *zstream;
	gint mccp_ver;

	// naws
	gboolean naws;
	// echo 
	gboolean echo;

	// msp structure
	gpointer msp;

	//mxp structure
	struct MXPINFO *mxp;

	// a link to parent session
	gpointer link;

};

TELNET_STATE *telnet_new();
void telnet_free(TELNET_STATE * telnet);
void telnet_reset(TELNET_STATE * telnet);
void telnet_send_window_size(TELNET_STATE * tn, gint w, gint h);
void telnet_process(TELNET_STATE * tn, gchar ** buff, gsize * len);

#endif				// __TELNET_H
