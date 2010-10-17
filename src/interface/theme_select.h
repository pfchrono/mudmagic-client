/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* callbacks.h:                                                            *
*                2004 Calvin Ellis  ( kyndig@mudmagic.com )               *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef THEME_SELECT_H
#define THEME_SELECT_H 1
#include <gtk/gtk.h>
#include <mudmagic.h>

#ifdef HAVE_WINDOWS  
#include <windows.h>
#endif

GList * build_theme_list (void);
void populate_with_themes(GtkWidget* w);
gchar *get_current_theme();
void themelist_selection_changed_cb(GtkTreeSelection* selection, gpointer data);
gchar * get_selected_theme();
gchar * get_current_font();
void apply_theme(gchar *theme_name, gchar *font, gboolean is_preview );
void init_theme();

#endif //THEME_SELECT_H
