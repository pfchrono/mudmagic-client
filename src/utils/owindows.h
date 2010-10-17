/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2006 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* owindows.h:                                                             *
*                2006 Tomas Mecir  ( kmuddy@kmuddy.net )                  *
*                                                                         *
***************************************************************************/
/**************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/    
#ifndef OWINDOWS_H
#define OWINDOWS_H

typedef struct _OWindowList OWINDOWLIST;

#include "configuration.h"
#include <gtk/gtk.h>

/* one owindow */
typedef struct
{
  int left, top, width, height;
  gchar *name, *title;

  // some widgets
  GtkWindow *window;
  GtkTextView *textview;
} OWINDOW;

/* owindow list */
struct _OWindowList
{
  GList *list;
  gboolean loading;
  SESSION_STATE *sess;
  gchar *active;
};

// OWINDOWLIST manipulation
OWINDOWLIST * owindowlist_new (SESSION_STATE *s);
void owindowlist_destroy(OWINDOWLIST * gl);

/** does this owindow exist in the list ? */ 
gboolean owindowlist_exists(OWINDOWLIST *ol, gchar * name);
OWINDOW * owindowlist_get_owindow(OWINDOWLIST *ol, gchar * name);
void owindowlist_set_owindow(OWINDOWLIST *ol, gchar *name, gchar *title,
    int left, int top, int width, int height);
void owindowlist_remove_owindow (OWINDOWLIST *ol, gchar *name);
void owindowlist_set_active (OWINDOWLIST *ol, gchar *name);
OWINDOW *owindowlist_get_active (OWINDOWLIST *ol);
GtkTextView *owindowlist_active_textview (OWINDOWLIST *ol);

/** load owindows */ 
void  owindowlist_load(OWINDOWLIST *ol, FILE * f);
/** save owindows */ 
void  owindowlist_save(OWINDOWLIST *ol, FILE * f);

OWINDOW *owindow_new (gchar *name, gchar *title, int left, int top, int width, int height);
void owindow_destroy (OWINDOW *o);
void owindow_set (OWINDOW *o, gchar *name, gchar *title, int left, int top, int width, int height);

#endif // OWINDOWS_H

