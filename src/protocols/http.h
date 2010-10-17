/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* http.h:                                                                 *
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
#ifndef HTTP_H
#define HTTP_H 1

#include <glib/gtypes.h>

typedef struct _HttpHelper HttpHelper;

typedef void     (*HH_StartDownload_t) (HttpHelper*);
typedef void     (*HH_EndDownload_t) (HttpHelper*);
typedef void     (*HH_UpdateDownload_t) (HttpHelper*, gsize current, gsize total);
typedef gboolean (*HH_GetStatus_t) (HttpHelper*);

struct _HttpHelper
{
    gpointer user_data_1;
    gpointer user_data_2;

    HH_StartDownload_t  start_cb;
    HH_EndDownload_t    end_cb;
    HH_UpdateDownload_t update_cb;
    HH_GetStatus_t      status_cb;
};

gint http_download (const gchar *url, int fd, HttpHelper* hh); 
gint http_header_get_status (const gchar *header );

#endif // HTTP_H
