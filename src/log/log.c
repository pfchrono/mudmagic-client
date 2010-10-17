/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* log.c:                                                                  *
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
#include "log.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

FILE* log_open_logfile( gchar* slot ) { // game title
	gchar *t, *filename ;
	FILE *ret;
	gchar buff[255];
	g_return_val_if_fail( slot != NULL, NULL );

	g_print("[log_open_logfile] %s\n", slot);
	if ( !g_file_test( slot, G_FILE_TEST_IS_DIR) ) { 
		// create the directory if not exists
#ifdef HAVE_WINDOWS
		if ( mkdir( slot ) == -1 ) { 
#else
		if ( mkdir( slot, 0777 ) == -1 ) { 
#endif
		
			perror("creating directory");
			g_free( t );
			return NULL;
		}
	}
	filename = g_build_path( G_DIR_SEPARATOR_S, slot, "log.txt", NULL);
	
	ret = fopen( filename, "a");
	if (ret) {
		time_t t;
		time ( &t ); 
		strftime( buff, 255, "\n%Y/%m/%d %H:%M:%S >> START LOGGING\n", 
			localtime( &t ));
		fprintf( ret, buff );
	}
	g_free( filename );
	
	return ret;	
}

void log_printf (FILE* log, const gchar* fmt, ...)
{
  va_list ap;

  if (log == NULL || fmt == NULL);
  
  va_start (ap, fmt);

  vfprintf (log, fmt, ap);

  va_end (ap);
}

void log_vprintf (FILE* log, const gchar* fmt, va_list ap)
{
//  va_start (ap, fmt);

  vfprintf (log, fmt, ap);

  va_end (ap);
}


void log_write_in_logfile( FILE* file, gchar *data, gsize size ) {
	gint n;
	g_return_if_fail( file != NULL && data != NULL );
	n = fwrite( data, 1, size, file );
	if ( n != size )
		g_print("[log_write_in_logfile] %d bytes from %d written\n", n, size );
		
}

void log_close_logfile( FILE* file ) {
	time_t t;
	gchar buff[255];
	g_return_if_fail( file != NULL );
	time ( &t ); 
	strftime( 
		buff, 255, "\n%Y/%m/%d %H:%M:%S >> STOP LOGGING\n", localtime( &t )
	);
	fprintf( file, buff );
	fclose( file );
}

void log_remove_logfile( gchar* slot ) {
	gchar *t =  NULL;
	t = g_build_path ( G_DIR_SEPARATOR_S, slot, "log.txt", NULL );
	g_print("[log_remove_logfile] %s\n", t );
	if ( unlink(t) ) { 
		g_print("[log_remove_logfile] can't remove %s\n", t);
	}
}
