/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* utils.h:                                                                *
*                2004 Calvin Ellis  ( kyndig@mudmagic.com )               *
*                2005 Mart Raudsepp ( leio@users.sf.net )                 *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef UTILS_H
#define UTILS_H 1
#include <glib.h>

gboolean utils_mkdir(gchar * name);
gint utils_atoi(const gchar * buff, gsize len);
gsize utils_strip_ansi_codes(gchar * data, gsize len);
void utils_LF2CRLF(gchar ** data, gsize * len);
void utils_replace(gchar * data, gsize len, gchar A, gchar B);
void utils_dump_data(gchar * buff, gsize len);
gchar *strip_color(gchar * text);
gboolean try_to_execute_url(const char * template, const char * url);
// playing sound file. Returns zero if succeful.
int utils_play_file (char * name);

const char*
utils_check_subpath (const gchar* basepath, const gchar* path);

gchar* utils_get_home_dir (void);
void utils_clear_gerrors (GList** gerrors);
void utils_clear_errors (GList** errors);

gchar* utils_join_strs (GList* list, const gchar* delimiter);
gchar* utils_join_gerrors (GList* list, const gchar* delimiter);

#if !GLIB_CHECK_VERSION(2,8,0)
gboolean
g_file_set_contents (const gchar *filename,
		     const gchar *contents,
		     gssize	     length,
		     GError	   **error);
#endif


#endif				// UTILS_H
