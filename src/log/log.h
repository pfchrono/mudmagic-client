/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* log.h:                                                                  *
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
#ifndef LOG_H
#define LOG_H

#include <glib.h>
#include <stdio.h>

FILE* log_open_logfile( gchar* name );
void log_write_in_logfile( FILE *file, gchar *data, gsize size );
void log_printf (FILE* log, const gchar* fmt, ...);
void log_vprintf (FILE* log, const gchar* fmt, va_list args);
void log_close_logfile( FILE* file );
void log_remove_logfile( gchar* name );

#endif // LOG_H
