/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* module.h:                                                               *
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
#ifndef MODULE_H
#define MODULE_H 1

#include <glib.h>
#include <gmodule.h>
#include <stdio.h>
#include <mudaccel.h>


typedef void (*module_func) (void);
typedef void (*module_datafunc) (gpointer session, gchar **data, gsize *size);
typedef void (*module_widgetfunc)  (gpointer *widget); // arg it's a GTK widget
typedef void (*module_sessionfunc) (gpointer session); 

typedef struct {
  // called when module in loaded
  module_func load;
  // called when module in unloaded
  module_func unload;
  // called when module is loaded or or a new window is created
  module_widgetfunc menu_modify;
  // called when module is unloaded
  module_widgetfunc menu_reset;
  // called when module is loaded or or a new window is created
  module_widgetfunc toolbar_modify;
  // called when module is unloaded
  module_widgetfunc toolbar_reset;
  // called when a session is created or loaded
  module_sessionfunc session_open;
  // called when a session is created or loaded
  module_sessionfunc session_close;
  // called before before arrived from server data is showed 
  module_datafunc data_in;
  // called before data is send to server
  module_datafunc data_out;
        // called when a session has been changed
        module_sessionfunc session_changed;
} MODULE_FUNCTIONS;

typedef struct {
  gchar *name;
  gchar *description;
  MODULE_FUNCTIONS *functions;
  // loaded at startup ?
  gboolean used;
} MODULE_ENTRY;


void module_init( GList **list );
void module_end( GList *list );

MODULE_ENTRY* module_get_by_name( GList *list, gchar *name );
gboolean module_load(MODULE_ENTRY *entry );
gboolean module_unload(MODULE_ENTRY *entry );
// return list of enabled modules
gchar** module_create_names_list (GList* list, gsize* len);

// call data_in method from every loaded module for data received
void module_call_all_data_in( GList *list,  gpointer session, gchar** data, gsize *size); 
// call data_out method from every loaded module for data that will be send
void module_call_all_data_out( GList *list, gpointer session, gchar** data, gsize *size);
// called when a new window is created
void module_call_all_menu_modify( GList *list, gpointer menubar); 
// called when a new window is closed 
void module_call_all_menu_reset( GList *list, gpointer menubar); 
// called when a new window is created
void module_call_all_toolbar_modify( GList *list, gpointer toolbar); 
// called when a new window is closed 
void module_call_all_toolbar_reset( GList *list, gpointer toolbar); 
// called when a new session is created
void module_call_all_session_open( GList *list, gpointer session); 
// called when a new session is closed 
void module_call_all_session_close( GList *list, gpointer session);
// celled when a session has been changed
void module_call_all_session_changed (GList* list, gpointer session);

#endif // MODULE_H
