/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* modules.c:                                                              *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                   *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <gtk/gtk.h>
#include <mudmagic.h>
#include <module.h>
#include <interface.h>

//-------------------------------- modules callbacks ------------------------

extern CONFIGURATION *config;
enum {        //used by modules treeview
  COL_ENABLE,
  COL_NAME,
  COL_DESCRIPTION,
  N_COL
};


void on_modules_cell_toggle_callback(GtkCellRenderer * cell,
             gchar * path, gpointer model)
{
  GtkTreeIter iter;
  gboolean *on;
  gchar *name;
  MODULE_ENTRY *entry;
  GList *l;

  gtk_tree_model_get_iter_from_string(model, &iter, path);
  gtk_tree_model_get(model, &iter, COL_ENABLE, &on, COL_NAME, &name,
         -1);
  if (on) {
    entry = module_get_by_name(config->modules, name);
    GtkWidget *menubar;
    GtkWidget *toolbar;
    // reset menu for any existing window
    if (entry->functions) {
      l = config->windows;
      while (l) {
        menubar =
            interface_get_widget(GTK_WIDGET
               (l->data),
               "menubar_main");
        if (menubar)
          if (entry->functions->menu_reset)
            entry->functions->
                menu_reset((gpointer)
                     menubar);

        toolbar =
            interface_get_widget(GTK_WIDGET
               (l->data),
               "toolbar_main");
        if (toolbar)
          if (entry->functions->
              toolbar_reset)
            entry->functions->
                toolbar_reset((gpointer) toolbar);
        l = g_list_next(l);
      }

      // call session_open for all existing sessions
      if (entry->functions->session_close) {
        l = config->sessions;
        while (l) {
          entry->functions->session_close(l->
                  data);
          l = g_list_next(l);
        }
      }
    }
    if (module_unload(entry)) {
      gtk_list_store_set(model, &iter, COL_ENABLE, FALSE,
             -1);
      // change the menu 
    } else {
      char *message;
      message =
          g_strdup_printf
          (" Module \"%s\" can't be unloaded !", name);
      interface_display_message(message);
      g_free(message);
    }
  } else {
    if (module_load
        ((entry =
          module_get_by_name(config->modules, name)))) {
      GtkWidget *menubar;
      GtkWidget *toolbar;
      if (entry->functions) {
        // modify menu for any existing window
        l = config->windows;
        while (l) {
          menubar =
              interface_get_widget(GTK_WIDGET
                 (l->data),
                 "menubar_main");
          if (menubar)
            if (entry->functions->
                menu_modify)
              entry->functions->
                  menu_modify((gpointer) menubar);
          toolbar =
              interface_get_widget(GTK_WIDGET
                 (l->data),
                 "toolbar_main");
          if (toolbar)
            if (entry->functions->
                toolbar_modify)
              entry->functions->
                  toolbar_modify((gpointer) toolbar);
          l = g_list_next(l);
        }
        // call session_open for all existing sessions
        if (entry->functions->session_open) {
          l = config->sessions;
          while (l) {
            entry->functions->
                session_open(l->data);
            l = g_list_next(l);
          }
        }
      }
      gtk_list_store_set(model, &iter, COL_ENABLE, TRUE,
             -1);
    } else {
      char *message;
      message =
          g_strdup_printf
          (" Module \"%s\" can't be loaded !", name);
      interface_display_message(message);
      g_free(message);
    }
  }
  g_free(name);
}

void on_modules1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *wid;
  GtkWidget *tree_view;
  GtkWidget *text_view;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *store;
  GtkTreeSelection *select;
  GList *l;
  wid = interface_create_object_by_name("window_modules");

  store = gtk_list_store_new(N_COL,
           G_TYPE_BOOLEAN,
           G_TYPE_STRING,
           G_TYPE_STRING, G_TYPE_STRING);

  l = config->modules;
  while (l) {
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
           COL_ENABLE,
           ((MODULE_ENTRY *) l->data)->
           functions ? TRUE : FALSE, COL_NAME,
           ((MODULE_ENTRY *) l->data)->name,
           COL_DESCRIPTION,
           ((MODULE_ENTRY *) l->data)->description,
           -1);
    l = g_list_next(l);
  }

  tree_view = interface_get_widget(wid, "modules_treeview");
  text_view = interface_get_widget(wid, "modules_desc");
  gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view),
        GTK_TREE_MODEL(store));


  renderer = gtk_cell_renderer_toggle_new();
  g_object_set(renderer, "activatable", TRUE, NULL);
  g_signal_connect(renderer, "toggled",
       (GCallback) on_modules_cell_toggle_callback,
       store);

  column =
      gtk_tree_view_column_new_with_attributes("Enable", renderer,
                 "active", COL_ENABLE,
                 NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  renderer = gtk_cell_renderer_text_new();
  //g_object_set(renderer, "editable", TRUE, NULL );

  column =
      gtk_tree_view_column_new_with_attributes("Module", renderer,
                 "text",
                 COL_DESCRIPTION,
                 NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
  gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

  g_print("[on_modules1_activate]\n");
}

//---------------------------- end modules callbacks ------------------------
