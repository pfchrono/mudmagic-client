/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* http.c:                                                                 *
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
#include <stdio.h>
#include <glib.h>
#include <mudmagic.h>
#include <network.h>

#ifdef HAVE_WINDOWS
#include <unistd.h>
#endif

#include "http.h"
#define HTTPBUFFLEN	2048

// return TRUE if in  there is a valid URL and fill host & port fields
// if len is -1  buff must be a NULL terminated string
gboolean http_parse_url(const gchar *buff, gsize len, gchar **host, gint *port) {
	gint i, pos ;
	gboolean on;
	// sanity check
	g_return_val_if_fail( buff != NULL && host != NULL && port != NULL, FALSE );
	*host = NULL ; *port = 0;
	if ( len == -1 ) len = strlen( buff );
	// url must be at least 7 bytes long
	g_return_val_if_fail( len > 7 , FALSE );
	// url must start with http://
	g_return_val_if_fail( g_str_has_prefix( buff, "http://" ), FALSE );
	// now what is until ":" or "/" or "$" is the host ($ means end of string)
	pos = 7 ; i = 7; on = FALSE ;
	while ( i < len  ) {
		//printf ( "%c\n", buff[i] );
		if ( on ) {  // we are after ":"
			if ( buff[i] == '/' ) {
				*port = utils_atoi( buff + pos, i - pos );
				break;
			}
			else
				g_return_val_if_fail( g_ascii_isdigit( buff[i] ), FALSE );
		} else {
			switch (buff[i]) {
				case ':' :{
					on = TRUE;
				}
				case '/' :{
					*host = g_strndup( buff + pos, i - pos );
					// check if we have a valid host
					g_return_val_if_fail( i > pos, FALSE );
					// get out from loop if port is not specified
					if ( !on ) i = len ;
					pos = i + 1;
				} break;
				default  :{
					g_return_val_if_fail(
						g_ascii_isalnum( buff[i] )||
						buff[i]=='-' || buff[i]=='.',
						FALSE
					);
				}
			}
		}
		i++;
	}
	if ( *host == NULL ) { // means we hit end of buffer without find ':' || '/'
		*host = g_strndup( buff + pos, i - pos );
	}
	if ( *port == 0 ) {
		if ( on ) // is port specified  ?
			*port = utils_atoi( buff + pos, i - pos );
		else
			*port = 80 ;
	}
	return TRUE;
}


gint http_header_get_status( const gchar *header ){
	gchar **strings;
	gint ret;
	strings = g_strsplit( header, " ", 3 );
	ret = utils_atoi( strings[1], -1 ) ;
	g_strfreev( strings );
	return ret;
}
// url is a null terminate string http://host[:port][/]
// file is the fullpath where the url is saved


gint http_download (const gchar *url, int f, HttpHelper* hh)
{
    gchar *host;
    gint port, fd, nb, i, k, state;
    gchar *request = NULL;
    gchar buff[HTTPBUFFLEN];
    gchar head[HTTPBUFFLEN];
    gboolean header;
    gboolean ret = FALSE;

    gsize current =  0; 		// how much we have downloaded
    gsize total = -1;			// content lenght
    gchar *s;
    /*
	    state means ...
	    0 - normal
	    1 - after \r
	    2 - after \r\n
	    3 - after \r\n\r
	    4 - after \r\n\r\n
    */
    g_return_val_if_fail( hh != NULL, CONNECT_BAD_PARAM);
    g_return_val_if_fail( url != NULL, CONNECT_BAD_URL );
    g_return_val_if_fail( http_parse_url( url, -1, &host, &port ), CONNECT_BAD_URL );

    mdebug (DBG_NETWORK, 0, "HTTP download '%s'\n", url);

	// FIXME: proxy http downloading unfixed yet
    fd = network_connection_open( host, port );
    if( fd < 0 )
    {
	return fd;
    }
    else
    {
	request = g_strconcat( "GET ", url, " HTTP/1.0\r\n\r\n", NULL );
	network_data_send( fd, request, -1 );
	g_free( request );

	header = TRUE; k = 0 ;
	state = 0;
	while ( ( nb = network_data_recv( fd, buff, HTTPBUFFLEN )) > 0 )
	{
	    if ( header )
	    { // we are in header ?
		for ( i = 0 ; i < nb ; i++ )
		{
		    //printf("state=%d i=%d char=%c code=%d\n", state, i, buff[i], buff[i]);
		    // save the response header in head
		    if ( k < HTTPBUFFLEN - 1 ) head[k++] = buff[i];
		    switch (state)
		    {
			case 0 :
			case 2 : {
				if (buff[i] == '\r') state ++;
				else state = 0;
				 } break;
			case 1 :
			case 3 : {
				if (buff[i] == '\n') state ++;
				else state = 0;

				if ( state == 4 )
				{ // check the header
				    header = FALSE;
				    head[k] = '\0' ; // we can cos k < HTTPBUFFLEN
				    if ( http_header_get_status(head) / 100 == 2 )
				    {
					// write what is left in buffer in file
					mdebug(DBG_NETWORK, 0, "HTTP: open file for writing");
					// get content lenght from header
					s = strstr( head, "Content-Length:" );
					if ( s != NULL )
						total = utils_atoi( s + 16, -1 );

					// create a new download window
					(*hh->start_cb) (hh);
					//win = interface_download_new("Download", NULL);

					write(f, buff + i + 1, nb - i - 1);
					current = nb-i-1;
					(*hh->update_cb) (hh, current, total);
					//interface_download_update( win, current, total);
					ret = CONNECT_OK;
				    }
				    else
				    {
					mdebug (DBG_NETWORK, 0, "HTTP: return code != 2xx");
				    }

				    i = nb; // finish the loop
				}
			} break;
		    } // switch
		}
	    }
	    else
	    {
	        if ( (*hh->status_cb) (hh))
		//if ( interface_download_iscanceled(win) )
		{
		    network_connection_close (fd);
		    ret = CONNECT_CANCELED;
		    break;
		}
		if ( f != -1 )
		{
		    if ( write (f, buff, nb) != nb )
		    {
			mdebug (DBG_NETWORK, 0, "HTTP download: writing problem\n");
		        ret = CONNECT_BAD_FILE;
			break;
		    }
		    else
		    {
			current += nb;
			(*hh->update_cb) (hh, current, total);
			//interface_download_update( win, current, total );
		    }
		}
	    }
	}
    }
    g_free( host );
    (*hh->end_cb) (hh);
    //if ( win != NULL )
	//interface_download_free( win );
    return ret;
}
