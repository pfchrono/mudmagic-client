/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2006 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* gauges.c:                                                            *
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
#include "gauges.h"
    
#include <mudmagic.h>
#include <interface.h>

#include <stdlib.h>
#include <string.h>

GAUGELIST * gaugelist_new (SESSION_STATE *s)
{
  GAUGELIST *gl = g_new0 (GAUGELIST, 1);
  gl->list = 0;
  gl->sess = s;
  gl->loading = FALSE;
  return gl;
}

void gaugelist_destroy(GAUGELIST *gl)
{
  GList* it;

  for (it = g_list_first (gl->list); it; it = g_list_next (it))
  {
    gauge_destroy ((GAUGE*) it->data);
  }
  g_list_free (gl->list);
  g_free (gl);
}

gboolean gaugelist_exists(GAUGELIST *gl, gchar * name)
{
  GList* it;

  for (it = g_list_first (gl->list); it; it = g_list_next (it))
  {
    GAUGE *g = (GAUGE *) it->data;
    if (!strcmp (name, g->variable))
      return TRUE;
  }
  return FALSE;
}

GAUGE * gaugelist_get_gauge(GAUGELIST *gl, gchar * name)
{
  GList* it;

  for (it = g_list_first (gl->list); it; it = g_list_next (it))
  {
    GAUGE *g = (GAUGE *) it->data;
    if (!strcmp (name, g->variable))
      return g;
  }
  return 0;
}

gboolean gaugebar_expose (GtkWidget *widget, GdkEventExpose *event,
    gpointer data)
{
  GList* it;
  GdkGC *gc;
  GdkColor black = {0, 0, 0, 0};
  PangoLayout *layout;

  GtkDrawingArea *gaugebar = GTK_DRAWING_AREA (widget);
  g_return_val_if_fail (gaugebar != NULL, FALSE);
  
  GAUGELIST *gl = (GAUGELIST *) g_object_get_data (GTK_OBJECT (gaugebar),
      "gaugelist");

  g_return_val_if_fail (gl != NULL, FALSE);

  int width = widget->allocation.width;
  int height = widget->allocation.height;
  
  gc = gdk_gc_new (widget->window);

  // clear the widget
  GdkColor bgc = gtk_widget_get_style (
      GTK_WIDGET (gl->sess->tab))->bg[GTK_STATE_NORMAL];
  gdk_gc_set_rgb_bg_color (gc, &bgc);
  gdk_gc_set_rgb_fg_color (gc, &bgc);
  gdk_gc_set_fill (gc, GDK_SOLID);
  gdk_draw_rectangle (widget->window, gc, TRUE, 0, 0, width+1, height+1);

  // paint the new widget ...
  int X = 2;
  for (it = g_list_first (gl->list); it; it = g_list_next (it))
  {
    GAUGE *g = (GAUGE *) it->data;

    // paint the text
    int xd;
    layout = gtk_widget_create_pango_layout (GTK_WIDGET (gaugebar),
                                             g->variable);
    gdk_gc_set_rgb_fg_color (gc, &black);
    gdk_draw_layout (widget->window, gc, X, 2, layout);
    pango_layout_get_pixel_size (layout, &xd, NULL);
    X += xd + 2;
    
    // paint the gauge
    int val = g->cur * 100 / (g->max ? g->max : 100);
    if (val > 100) val = 100;
    gdk_gc_set_line_attributes (gc, 1, GDK_LINE_SOLID, GDK_CAP_NOT_LAST,
                                GDK_JOIN_MITER);
    gdk_gc_set_rgb_fg_color (gc, &black);
    gdk_draw_rectangle (widget->window, gc, FALSE, X, 2, 102, 20);
    if (val) {
      gdk_gc_set_rgb_fg_color (gc, &g->color);
      gdk_draw_rectangle (widget->window, gc, TRUE, X+1, 3, val+1, 19);
    }
    g_object_unref (layout);
    X += 110;
  }

}

void update_gaugelist (GAUGELIST *gl)
{
  g_return_if_fail (gl->sess->tab != NULL);
  GtkWidget *gaugebar = interface_get_widget (gl->sess->tab, "gaugebar");
  g_return_if_fail (gaugebar != NULL);
  
  // no gauges -> goodbye ...
  if (!gl->list) {
    gtk_widget_hide (gaugebar);
    return;
  }
  
  // we have some gauges - must show the widget
  gtk_widget_show (gaugebar);
  
  // set gaugelist pointer
  g_object_set_data (GTK_OBJECT (gaugebar), "gaugelist", gl);
  
  gtk_widget_queue_draw (gaugebar);
}

void gaugelist_handle_variable_change (GAUGELIST *gl, gchar *variable)
{
  VARLIST *vl = (VARLIST *) gl->sess->variables;
  GList* it;
  gboolean change = FALSE;

  for (it = g_list_first (gl->list); it; it = g_list_next (it))
  {
    GAUGE *g = (GAUGE *) it->data;
    if (!strcmp (variable, g->variable)) {
      g->cur = varlist_get_int_value (vl, variable);
      change = TRUE;
    }
    if (!strcmp (variable, g->maxvariable))
    {
      g->max = varlist_get_int_value (vl, variable);
      change = TRUE;
    }
  }
  update_gaugelist (gl);
}

void gaugelist_set_gauge(GAUGELIST *gl, gchar *variable, gchar *maxvariable,
                         GdkColor color)
{
  VARLIST *vl = (VARLIST *) gl->sess->variables;
  GAUGE *g = gaugelist_get_gauge (gl, variable);
  if (!g) {
    g = gauge_new ();
    gl->list = g_list_append (gl->list, g);
  }
  gauge_set_var (g, variable);
  gauge_set_maxvar (g, maxvariable);
  g->color = color;
  g->cur = varlist_get_int_value (vl, variable);
  g->max = varlist_get_int_value (vl, maxvariable);
  update_gaugelist (gl);
}

void gaugelist_remove_gauge (GAUGELIST *gl, gchar * variable)
{
  GAUGE *g = gaugelist_get_gauge (gl, variable);
  if (!g) return;
  gl->list = g_list_remove (gl->list, g);
  gauge_destroy (g);
  update_gaugelist (gl);
}

void gaugelist_load(GAUGELIST *gl, FILE *f)
{
  gl->loading = TRUE;
  char buff[1025], buff2[1025], buff3[1025];
  while (!feof(f)) 
  {
    if ((fgets(buff, 1024, f) != NULL) && (fgets(buff2, 1024, f) != NULL)
         && (fgets(buff3, 1024, f) != NULL))
    {
      int len = strlen(buff);
      int len2 = strlen(buff2);
      int len3 = strlen(buff3);
      GdkColor color;
      color.pixel = 0;
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
        // create a gauge, add it to the list
        sscanf (buff3, "%d%d%d", &(color.red), &(color.green), &(color.blue));
        gaugelist_set_gauge (gl, buff, buff2, color);
      }
    }
  }
  gl->loading = FALSE;
}

void save_gl_entry (gpointer *data, gpointer *userdata) 
{
  GAUGE* g = ((GAUGE *) data);
  FILE *f = (FILE *) userdata;
  fprintf(f, "%s\n", g->variable);
  fprintf(f, "%s\n", g->maxvariable);
  fprintf(f, "%d %d %d\n", g->color.red, g->color.green, g->color.blue);
}

void gaugelist_save(GAUGELIST *gl, FILE *f)
{
  g_list_foreach (gl->list, (GFunc) save_gl_entry, f);
}

GAUGE *gauge_new()
{
  GAUGE *gauge = g_new0(GAUGE, 1);
  return gauge;
}

void gauge_destroy(GAUGE *g)
{
  if (!g) return;
  if (g->variable)
    g_free(g->variable);
  if (g->maxvariable)
    g_free(g->maxvariable);
  g_free (g);
}

void gauge_set_var (GAUGE *g, gchar *var)
{
  if (!g) return;
  if (g->variable)
    g_free(g->variable);
  g->variable = g_strdup (var);
}

void gauge_set_maxvar (GAUGE *g, gchar *var)
{
  if (g->maxvariable)
    g_free(g->maxvariable);
  g->maxvariable = g_strdup (var);
}
