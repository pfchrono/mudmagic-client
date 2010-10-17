/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2006 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* owindows.c:                                                             *
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
#include "owindows.h"
    
#include <mudmagic.h>
#include <interface.h>

#include <stdlib.h>
#include <string.h>

OWINDOWLIST *owindowlist_new (SESSION_STATE *s)
{
  OWINDOWLIST *ol = g_new0 (OWINDOWLIST, 1);
  ol->list = 0;
  ol->sess = s;
  return ol;
}

void owindowlist_destroy(OWINDOWLIST *ol)
{
  GList* it;

  for (it = g_list_first (ol->list); it; it = g_list_next (it))
  {
    owindow_destroy ((OWINDOW*) it->data);
  }
  if (ol->active) g_free (ol->active);
  g_list_free (ol->list);
  g_free (ol);
}

gboolean owindowlist_exists(OWINDOWLIST *ol, gchar *name)
{
  return (owindowlist_get_owindow (ol, name) != 0) ? TRUE : FALSE;
}

OWINDOW * owindowlist_get_owindow(OWINDOWLIST *ol, gchar *name)
{
  GList* it;

  for (it = g_list_first (ol->list); it; it = g_list_next (it))
  {
    OWINDOW *o = (OWINDOW *) it->data;
    if (!strcmp (name, o->name))
      return o;
  }
  return 0;
}

void owindowlist_set_owindow(OWINDOWLIST *ol, gchar *name, gchar *title,
    int left, int top, int width, int height)
{
  OWINDOW *o = owindowlist_get_owindow (ol, name);
  if (!o) {
    o = owindow_new (name, title, left, top, width, height);
    ol->list = g_list_append (ol->list, o);
  } else
    owindow_set (o, name, title, left, top, width, height);
}

void owindowlist_remove_owindow (OWINDOWLIST *ol, gchar *name)
{
  OWINDOW *o = owindowlist_get_owindow (ol, name);
  if (!o) return;
  ol->list = g_list_remove (ol->list, o);
  owindow_destroy (o);
}

void owindowlist_set_active (OWINDOWLIST *ol, gchar *name)
{
  if (ol->active) g_free (ol->active);
  if (name) ol->active = g_strdup (name);
}

OWINDOW *owindowlist_get_active (OWINDOWLIST *ol)
{
  if (!ol->active) return 0;
  return owindowlist_get_owindow (ol, ol->active);
}

GtkTextView *owindowlist_active_textview (OWINDOWLIST *ol)
{
  OWINDOW *ow = owindowlist_get_active (ol);
  if (!ow) return 0;
  
  // show it, so that the user doesn't lose text if he closed the window
  gtk_widget_show (GTK_WIDGET (ow->window));
  
  return ow->textview;
}

void owindowlist_load(OWINDOWLIST *ol, FILE *f)
{
  char buff[1025], buff2[1025], buff3[1025];
  while (!feof(f)) 
  {
    if ((fgets(buff, 1024, f) != NULL) && (fgets(buff2, 1024, f) != NULL)
         && (fgets(buff3, 1024, f) != NULL))
    {
      int len = strlen(buff);
      int len2 = strlen(buff2);
      int len3 = strlen(buff3);
      int left, top, width, height;
      if (len && len2 && len3)
      {
        // get rid of trailing newlines, if any
        if (buff[len - 1] == '\n') 
        {
          buff[len - 1] = '\0';
          --len;
        }
        if (buff2[len2 - 1] == '\n') {
          buff2[len2 - 1] = '\0';
          --len2;
        }
        if (buff3[len3 - 1] == '\n') {
          buff3[len3 - 1] = '\0';
          --len3;
        }
        // create a owindow, add it to the list
        sscanf (buff3, "%d%d%d%d", &left, &top, &width, &height);
        owindowlist_set_owindow (ol, buff, buff2, left, top, width, height);
      }
    }
  }
}

void save_ow_entry (gpointer *data, gpointer *userdata) 
{
  OWINDOW* o = ((OWINDOW *) data);
  FILE *f = (FILE *) userdata;
  fprintf(f, "%s\n", o->name);
  fprintf(f, "%s\n", o->title);
  fprintf(f, "%d %d %d %d\n", o->left, o->top, o->width, o->height);
}

void owindowlist_save(OWINDOWLIST *ol, FILE *f)
{
  g_list_foreach (ol->list, (GFunc) save_ow_entry, f);
}

OWINDOW *owindow_new (gchar *name, gchar *title, int left, int top, int width, int height)
{
  OWINDOW *ow = g_new0(OWINDOW, 1);
  owindow_set (ow, name, title, left, top, width, height);
  return ow;
}

void owindow_set (OWINDOW *ow, gchar *name, gchar *title, int left, int top, int width, int height)
{
  if (ow->name) g_free (ow->name);
  if (ow->title) g_free (ow->title);
  ow->name = g_strdup (name);
  ow->title = g_strdup (title);
  ow->left = left;
  ow->top = top;
  ow->width = width;
  ow->height = height;

  ow->window = 0;
  ow->textview = 0;

  // actually create/resize the real thing
  ow->window = interface_create_object_by_name("owindow");
  g_return_if_fail(ow->window != NULL);
  gtk_window_set_title(GTK_WINDOW (ow->window), ow->title);
  
  ow->textview = GTK_TEXT_VIEW (interface_get_widget (ow->window, "output"));
  
  // resize the thing
  gtk_decorated_window_move_resize_window (ow->window,
      left, top, width, height);
  gtk_window_set_resizable (ow->window, TRUE);
  gtk_widget_show (GTK_WIDGET (ow->window));
}

void owindow_destroy(OWINDOW *o)
{
  if (!o) return;
  
  // destroy the window
  gtk_widget_destroy (o->window);

  if (o->name)
    g_free(o->name);
  if (o->title)
    g_free(o->title);
  g_free (o);

}

