/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* module.c:                                                               *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                   *
***************************************************************************/
/**************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <mudmagic.h>
#include <interface.h>
#include "module.h"

#include "modules.h"


/*
This manages modules, parts of the client that can be enabled/disabled
on demand. They are fairly isolated from the rest, and used for bigger
stand-alone functionality, such as the mapper.
*/

void module_init( GList **list) {
  *list = NULL;

  // Here we construct the list of available modules:
  MODULE_ENTRY *entry;

  entry = g_new0( MODULE_ENTRY, 1);
  entry->name = g_strdup ("automapper");
  entry->description = g_strdup ("Automapper module for MudMagic.");
  entry->functions = 0;
  *list = g_list_append( *list, entry );
  
  entry = g_new0( MODULE_ENTRY, 1);
  entry->name = g_strdup ("database");
  entry->description = g_strdup ("With this module the user can store informations about various entities from game ( weapons, monsters, etc  )");
  entry->functions = 0;
  *list = g_list_append( *list, entry );
  
  entry = g_new0( MODULE_ENTRY, 1);
  entry->name = g_strdup ("notes");
  entry->description = g_strdup ("With this module the user can write/store/remove notes.");
  entry->functions = 0;
  *list = g_list_append( *list, entry );
  
  entry = g_new0( MODULE_ENTRY, 1);
  entry->name = g_strdup ("recorder");
  entry->description = g_strdup ("May be used for record sequences of user commands and save them as macro/alias/trigger.");
  entry->functions = 0;
  *list = g_list_append( *list, entry );
}


void module_end( GList *list) { // unload all the modules
  GList* l;
  MODULE_ENTRY* entry;
  l = list;
  while  ( l ) {
    entry = (MODULE_ENTRY*)l->data;
    module_unload (entry);
    g_free( entry->name );
    g_free( entry->description );
    g_free( entry->functions );
    g_free( l->data );
    l = g_list_next ( l );
  }
  g_list_free( list );
}


MODULE_ENTRY* module_get_by_name( GList *list, gchar *name ) {
  GList *l;
  l = list;
  while ( l ) {
    if ( !strcmp( name, ((MODULE_ENTRY*)l->data)->name ) )
      return (MODULE_ENTRY*)l->data;
    l = g_list_next( l );
  }
  return NULL;
}


gboolean module_load( MODULE_ENTRY* entry ) {
  if (!entry) return FALSE;
  
  // TODO: this code somewhat sucks, would be better if each module
  // could register its functions itself ...
  if (!strcmp (entry->name, "automapper")) {
    entry->functions = g_new0( MODULE_FUNCTIONS, 1 );
    entry->functions->load = module_automapper_load;
    entry->functions->unload = module_automapper_unload;
    entry->functions->menu_modify = (module_widgetfunc)
        module_automapper_menu_modify;
    entry->functions->menu_reset = (module_widgetfunc)
        module_automapper_menu_reset;
    entry->functions->session_open = (module_sessionfunc)
        module_automapper_session_open;
    entry->functions->session_close = (module_sessionfunc)
        module_automapper_session_close;
    entry->functions->data_out = (module_datafunc)
        module_automapper_data_out;
  }
  else if (!strcmp (entry->name, "database")) {
    entry->functions = g_new0( MODULE_FUNCTIONS, 1 );
    entry->functions->load = module_database_load;
    entry->functions->unload = module_database_unload;
    entry->functions->toolbar_modify = (module_widgetfunc)
        module_database_toolbar_modify;
    entry->functions->toolbar_reset = (module_widgetfunc)
        module_database_toolbar_reset;
  }
  else if (!strcmp (entry->name, "notes")) {
    entry->functions = g_new0( MODULE_FUNCTIONS, 1 );
    entry->functions->load = module_notes_load;
    entry->functions->unload = module_notes_unload;
    entry->functions->toolbar_modify = (module_widgetfunc)
        module_notes_toolbar_modify;
    entry->functions->toolbar_reset = (module_widgetfunc)
        module_notes_toolbar_reset;
  }
  else if (!strcmp (entry->name, "recorder")) {
    entry->functions = g_new0( MODULE_FUNCTIONS, 1 );
    entry->functions->load = module_recorder_load;
    entry->functions->unload = module_recorder_unload;
    entry->functions->toolbar_modify = (module_widgetfunc)
        module_recorder_toolbar_modify;
    entry->functions->toolbar_reset = (module_widgetfunc)
        module_recorder_toolbar_reset;
    entry->functions->session_open = (module_sessionfunc)
        module_recorder_session_open;
    entry->functions->session_close = (module_sessionfunc)
        module_recorder_session_close;
    entry->functions->session_changed = (module_sessionfunc)
        module_recorder_session_changed;
    entry->functions->data_out = (module_datafunc)
        module_recorder_data_out;
  }
  else return FALSE;
  
  return TRUE;
}

gboolean module_unload( MODULE_ENTRY* entry )
{
  if (!entry) return FALSE;

  if ( entry->functions && entry->functions->unload )
    entry->functions->unload();

  g_free( entry->functions );
  entry->functions = NULL;

  return TRUE;
}

// These functions call various functions from the modules.
// This means that the rest of the code knows nothng about modules, they simply
// call these functions, which take care of the rest.

void module_call_all_data_in( GList *list, gpointer session, gchar** data, gsize *size) {
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if ( entry->functions )
      if ( entry->functions->data_in )
        entry->functions->data_in( session, data, size );
    l = g_list_next(l);
  }
}

void module_call_all_data_out(GList *list, gpointer session, gchar** data, gsize *size) {
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if ( entry->functions )
      if ( entry->functions->data_out )
        entry->functions->data_out( session, data, size );
    l = g_list_next(l);
  }
}

void module_call_all_menu_modify( GList *list, gpointer menubar) {
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if ( entry->functions )
      if ( entry->functions->menu_modify )
        entry->functions->menu_modify( menubar );
    l = g_list_next(l);
  }
}

void module_call_all_menu_reset( GList *list, gpointer menubar) {
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if ( entry->functions )
      if ( entry->functions->menu_reset )
        entry->functions->menu_reset( menubar );
    l = g_list_next(l);
  }
}

void module_call_all_toolbar_modify( GList *list, gpointer toolbar) {
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if ( entry->functions )
      if ( entry->functions->toolbar_modify )
        entry->functions->toolbar_modify( toolbar );
    l = g_list_next(l);
  }
}

void module_call_all_toolbar_reset( GList *list, gpointer toolbar) {
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if ( entry->functions )
      if ( entry->functions->toolbar_reset )
        entry->functions->toolbar_reset( toolbar );
    l = g_list_next(l);
  }
}

void module_call_all_session_open( GList *list, gpointer menubar) {
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if ( entry->functions )
      if ( entry->functions->session_open )
        entry->functions->session_open( menubar );
    l = g_list_next(l);
  }
}

void module_call_all_session_close( GList *list, gpointer menubar) {
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if ( entry->functions )
      if ( entry->functions->session_close )
        entry->functions->session_close( menubar );
    l = g_list_next(l);
  }
}

void module_call_all_session_changed (GList *list, gpointer session)
{
  GList *l;
  MODULE_ENTRY *entry;
  l = list;
  while ( l ) {
    entry = ( MODULE_ENTRY*) l->data;
    if (entry->functions
      && entry->functions->session_changed )
        entry->functions->session_changed (session);
    l = g_list_next(l);
  }
}

gchar** module_create_names_list (GList* list, gsize* len)
{
  GList*        it;
  gchar**       ret;
  const gchar*  name;
  gsize         ait = 0;

  if (list == NULL) return NULL;
  g_assert (len);
  
  if (g_list_length (list) == 0)
  {
    *len = 0;
    return NULL;
  }

  ret = g_new0 (gchar*, g_list_length (list) + 1);

  for (it = g_list_first (list); it; it = g_list_next (it))
    if (((MODULE_ENTRY*) it->data)->functions)
    {
      name = ((MODULE_ENTRY*) it->data)->name;
      ret[ait++] = g_strdup (name);
    }
  *len = g_list_length (list);

  return ret;
}

