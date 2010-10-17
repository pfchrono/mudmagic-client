/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2006 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* statusvars.c:                                                            *
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
#include "statusvars.h"
    
#include <mudmagic.h>
#include "interface.h"

#include <stdlib.h>
#include <string.h>

SVLIST * svlist_new (SESSION_STATE *s)
{
  SVLIST *svl = g_new0 (SVLIST, 1);
  svl->list = 0;
  svl->sess = s;
  svl->loading = FALSE;
  return svl;
}

void svlist_destroy(SVLIST *svl)
{
  GList* it;

  for (it = g_list_first (svl->list); it; it = g_list_next (it))
  {
    statusvar_destroy ((STATUSVAR*) it->data);
  }
  g_list_free (svl->list);
  g_free (svl);
}

gboolean svlist_exists(SVLIST *svl, gchar * name)
{
  GList* it;

  for (it = g_list_first (svl->list); it; it = g_list_next (it))
  {
    STATUSVAR *g = (STATUSVAR *) it->data;
    if (!strcmp (name, g->variable))
      return TRUE;
  }
  return FALSE;
}

STATUSVAR * svlist_get_statusvar(SVLIST *svl, gchar * name)
{
  GList* it;

  for (it = g_list_first (svl->list); it; it = g_list_next (it))
  {
    STATUSVAR *g = (STATUSVAR *) it->data;
    if (!strcmp (name, g->variable))
      return g;
  }
  return 0;
}

void update_svlist (SVLIST *svl)
{
  GList* it;
  GtkLabel *statusvars;
  
  if (svl->loading) return;
  
  statusvars = (GtkLabel *) interface_get_widget (svl->sess->tab, "statusvars");
  
  // no statusvars -> goodbye ...
  if (!svl->list) {
    gtk_widget_hide (GTK_WIDGET (statusvars));
    return;
  }
  
  // construct the label ...
  GString *str = g_string_new ("");
  for (it = g_list_first (svl->list); it; it = g_list_next (it))
  {
    STATUSVAR *sv = (STATUSVAR *) it->data;
    if (sv->percentage)
      g_string_append_printf (str, "%s %d%%  ", sv->variable,
                                    sv->max ? sv->cur * 100 / sv->max : sv->cur);
    else
      g_string_append_printf (str, "%s %d/%d  ", sv->variable,
                                    sv->cur, sv->max);
  }
  // we have some status variables - must show the widget
  gtk_widget_show (GTK_WIDGET (statusvars));
  gtk_label_set_text (statusvars, str->str);
  g_string_free (str, TRUE);
}

void svlist_set_statusvar(SVLIST *svl, gchar *variable, gchar *maxvariable,
                         gboolean percentage)
{
  VARLIST *vl = (VARLIST *) svl->sess->variables;
  STATUSVAR *g = svlist_get_statusvar (svl, variable);
  if (!g) {
    g = statusvar_new ();
    svl->list = g_list_append (svl->list, g);
  }
  statusvar_set_var (g, variable);
  statusvar_set_maxvar (g, maxvariable);
  g->percentage = percentage;
  g->cur = varlist_get_int_value (vl, variable);
  g->max = varlist_get_int_value (vl, maxvariable);
  update_svlist (svl);
}

void svlist_handle_variable_change (SVLIST *svl, gchar *variable)
{
  VARLIST *vl = (VARLIST *) svl->sess->variables;
  GList* it;

  for (it = g_list_first (svl->list); it; it = g_list_next (it))
  {
    STATUSVAR *g = (STATUSVAR *) it->data;
    if (!strcmp (variable, g->variable))
      g->cur = varlist_get_int_value (vl, variable);
    if (!strcmp (variable, g->maxvariable))
      g->max = varlist_get_int_value (vl, variable);
  }
  update_svlist (svl);
}

void svlist_remove_statusvar (SVLIST *svl, gchar * variable)
{
  STATUSVAR *sv = svlist_get_statusvar (svl, variable);
  if (!sv) return;
  svl->list = g_list_remove (svl->list, sv);
  statusvar_destroy (sv);
  update_svlist (svl);
}

void svlist_load(SVLIST *svl, FILE *f)
{
  char buff[1025], buff2[1025], buff3[1025];
  svl->loading = TRUE;
  while (!feof(f)) 
  {
    if ((fgets(buff, 1024, f) != NULL) && (fgets(buff2, 1024, f) != NULL)
         && (fgets(buff3, 1024, f) != NULL))
    {
      int len = strlen(buff);
      int len2 = strlen(buff2);
      int len3 = strlen(buff3);
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
        // create a statusvar, add it to the list
        svlist_set_statusvar (svl, buff, buff2, (atoi (buff3)) ? TRUE : FALSE);
      }
    }
  }
  
  svl->loading = FALSE;
}

void save_svl_entry (gpointer *data, gpointer *userdata) 
{
  STATUSVAR* g = ((STATUSVAR *) data);
  FILE *f = (FILE *) userdata;
  fprintf(f, "%s\n", g->variable);
  fprintf(f, "%s\n", g->maxvariable);
  fprintf(f, "%d\n", g->percentage ? 1 : 0);
}

void svlist_save(SVLIST *svl, FILE *f)
{
  g_list_foreach (svl->list, (GFunc) save_svl_entry, f);
}

STATUSVAR *statusvar_new()
{
  STATUSVAR *statusvar = g_new0(STATUSVAR, 1);
  return statusvar;
}

void statusvar_destroy(STATUSVAR *sv)
{
  if (!sv) return;
  if (sv->variable)
    g_free(sv->variable);
  if (sv->maxvariable)
    g_free(sv->maxvariable);
  g_free (sv);
}

void statusvar_set_var (STATUSVAR *sv, gchar *var)
{
  if (!sv) return;
  if (sv->variable)
    g_free(sv->variable);
  sv->variable = g_strdup (var);
}

void statusvar_set_maxvar (STATUSVAR *sv, gchar *var)
{
  if (sv->maxvariable)
    g_free(sv->maxvariable);
  sv->maxvariable = g_strdup (var);
}
