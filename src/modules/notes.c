/***************************************************************************
 *  Mud Magic Client                                                       *
 *  Copyright (C) 2004 MudMagic.Com ( hosting@mudmagic.com )               *
 *                2004 Calvin Ellis ( kyndig@mudmagic.com  )               *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "notes.h"
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <string.h>
#include <mudmagic.h>
#include <interface.h>
#include <sqlite3.h>
#include <time.h>
#include <directories.h>

// there will be no exporting
#undef EXPORT
#define EXPORT

static gchar *module_path = NULL;


EXPORT void module_notes_load( gchar *filename ) {
  module_path = g_path_get_dirname( filename );
  mdebug (DBG_MODULE, 0, "Module notes is loaded.");
}

EXPORT void module_notes_unload(void) {
  if ( module_path ) {
    g_free( module_path );
    module_path = NULL;
  }
  mdebug (DBG_MODULE, 0, "module notes is unloaded");
}

EXPORT void on_notes_remove_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *win;

  sqlite3 *db = NULL;
  gchar *error=NULL;
  gint rc;

  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;

  gint id;
  gchar *query;

  win = gtk_widget_get_toplevel( GTK_WIDGET(button) );
  g_return_if_fail( win );
  db = g_object_get_data( G_OBJECT(win), "notes" );
  g_return_if_fail( db );
  treeview = g_object_get_data( G_OBJECT(win), "treeview_notes_list");
  g_return_if_fail( treeview );
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  g_return_if_fail( selection );
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get( model, &iter, 0, &id, -1);
    query = g_strdup_printf("delete from notes where id = %d", id );
    rc = sqlite3_exec( db, query, NULL, NULL, &error );
    if ( rc != SQLITE_OK ) {
      g_warning( "remove note from database: (%d)%s\n", rc, error );
      if (error != NULL ) {
        sqlite3_free(error);
        error = NULL ;
      }
    }
    gtk_list_store_remove( GTK_LIST_STORE(model), &iter);
  } else {
    //was interface_display_message() but can't link correctly
    mdebug (DBG_MODULE, 0, "No note selected!");
  }

}

EXPORT void on_notes_add_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *win;
  GtkWidget *wid;
  GtkTextBuffer *buffer;

  sqlite3 *db = NULL;
  gchar *error=NULL;
  gint rc;
  gchar *query;

  gint id;
  gchar *title;
  gchar date[32];
  time_t t;
  gchar *text;

  GtkTextIter start;
  GtkTextIter end;

  GtkWidget *treeview;
  GtkTreeIter iter;
  GtkListStore *store;

  win = gtk_widget_get_toplevel( GTK_WIDGET(button) );
  g_return_if_fail( win );
  db = g_object_get_data( G_OBJECT(win), "notes" );
  g_return_if_fail( db );

  wid = g_object_get_data( G_OBJECT(win), "entry_note_title" );
  title = g_strdup( gtk_entry_get_text( GTK_ENTRY(wid) ) );

  buffer = g_object_get_data( G_OBJECT(win), "textbuffer_note_text" );
  gtk_text_buffer_get_start_iter( buffer, &start );
  gtk_text_buffer_get_end_iter( buffer, &end );
  text = gtk_text_buffer_get_text ( buffer, &start, &end, FALSE );

  time( &t );
  strftime( date, 255, "%Y-%m-%d %H:%M:%S", localtime(&t) );

  query = g_strdup_printf(
    "insert into notes values( NULL, \"%s\", \"%s\", \"%s\" )",
    title, text, date
  );
  rc = sqlite3_exec( db, query, NULL, NULL, &error );
  g_free( query );
  if ( rc != SQLITE_OK ) {
    g_warning( "add note in database: (%d)%s\n", rc, error );
    if (error != NULL ) {
      sqlite3_free(error);
      error = NULL ;
    }
    return;
  }
  id = sqlite3_last_insert_rowid( db );

  treeview = g_object_get_data( G_OBJECT(win), "treeview_notes_list" );
  g_return_if_fail( treeview );
  store = GTK_LIST_STORE(gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) ));
  gtk_list_store_append ( store, &iter );
  gtk_list_store_set(
    store, &iter, 0, id, 1, title, 2, date, -1
  );
  g_free(title);
  g_free(text);
}

void on_notes_close_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *win;
  sqlite3 *db = NULL;
  win = gtk_widget_get_toplevel( GTK_WIDGET( button ));
  db = g_object_get_data( G_OBJECT(win), "notes" );
  if ( db ) {
    sqlite3_close( db );
  }
  gtk_widget_destroy( win );

}
EXPORT void on_treeview_notes_list_selection_changed( GtkTreeSelection *selection, gpointer data ){
  GtkWidget *win;
  GtkWidget *wid;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gint id;
  gchar *title;

  sqlite3 *db;
  sqlite3_stmt *vm = NULL;
  gchar *query;
  gchar *error = NULL;
  const gchar **cvals;
  const gchar **cnames;
  const gchar* tail = NULL;
  gint cn;
  gint rc;

  win = GTK_WIDGET( data ); // the top window
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get(
      model, &iter, 0, &id, 1, &title, -1
    );
    // begin: set the title
    wid = g_object_get_data( G_OBJECT(win), "entry_note_title" );
    gtk_entry_set_text( GTK_ENTRY(wid), title );
    g_free( title );
    // end: set the title

    // begin: set text
    db = g_object_get_data( G_OBJECT(win), "notes" );
    query = g_strdup_printf("select text from notes where id=%d", id );
    //rc = sqlite_compile( db, query, NULL, &vm, &error );
    rc = sqlite3_prepare( db, query, strlen(query), &vm, &tail );
    g_free( query );
    if ( rc != SQLITE_OK ) {
      g_warning( "get note from database(compile): (%d)%s\n", rc, sqlite3_errmsg(db) );
      return;
    } else {
      //if (sqlite_step( vm, &cn, &cvals, &cnames ) != SQLITE_ROW ) {
      if (sqlite3_step( vm ) != SQLITE_ROW ) {
        g_warning( "get note from database(step): (%d)%s\n", rc, error);
        if (error != NULL ) {
          sqlite3_free(error);
          error = NULL ;
        }
        return;
      } else {
        GtkTextBuffer *buffer;
        buffer = g_object_get_data(
          G_OBJECT(win), "textbuffer_note_text"
        );
        //gtk_text_buffer_set_text( buffer, cvals[0], -1 );
        gtk_text_buffer_set_text( buffer, sqlite3_column_text(vm, 0), -1 );
      }
      rc = sqlite3_finalize( vm);
    }
    // end: set text
  }
}

EXPORT void on_button_notes_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *win;
  GtkWidget *wid;
  GladeXML *xml;
  gchar *ui_file;

  // database variables
  gchar *db_file;
  sqlite3 *db = NULL;
  gchar *error = NULL;
  sqlite3_stmt *vm = NULL;
  const gchar **cnames;
  const gchar **cvals;
  const gchar* query, *tail;
  gint cn;
  gint rc;

  // treeview variables
  GtkWidget *treeview;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;
  GtkTreeIter iter;

  SESSION_STATE *session;

  // begin: get active session
  GtkWidget *main_window;
  GtkWidget *tab;
  main_window = interface_get_active_window();

   tab = interface_get_active_tab();
   session = g_object_get_data(G_OBJECT(tab), "session");

   // session = interface_get_active_session();
  if (!session) {
    mdebug (DBG_MODULE, 0, "There is no active session.");
    return;
  }
  // end: get active session

  // begin: open data base
  db_file = g_build_path(
    G_DIR_SEPARATOR_S, session->slot, "notes.db", NULL
  );
  rc = sqlite3_open( db_file, &db);
  if (rc != SQLITE_OK){
    g_warning("Couldn't open database: %s", sqlite3_errmsg(db) );
    return;
  }
  g_free( db_file );
  // end: open the database

  // begin: create notes window
  ui_file = g_build_filename(
      // module_path, "notes", "notes.glade", NULL
      mudmagic_data_directory(), "interface", "notes.glade", NULL 
  );

  xml = glade_xml_new( ui_file, "window_notes", NULL);
  if ( !xml ) {
    g_warning("Can NOT create GladeXML");
    return;
  }
  
#ifdef HAVE_WINDOWS
    GtkWidget *gtk_button;

    gtk_button =  glade_xml_get_widget (xml,"notes_close");
    g_signal_connect( gtk_button, "clicked", G_CALLBACK( on_notes_close_clicked ), NULL );

    gtk_button = glade_xml_get_widget( xml, "notes_add");
    g_signal_connect( gtk_button, "clicked", G_CALLBACK( on_notes_add_clicked ), NULL );

    gtk_button = glade_xml_get_widget(xml, "notes_remove");
    g_signal_connect( gtk_button, "clicked", G_CALLBACK( on_notes_remove_clicked ), NULL );

#else
   //this doesn't work on windows :~(
    glade_xml_signal_autoconnect( xml );
#endif

  win = glade_xml_get_widget( xml, "window_notes" );
  g_object_set_data( G_OBJECT(win), "notes", (gpointer)db );

  g_free( ui_file );
  // end: create notes window


  // begin: set the treeview
  treeview = glade_xml_get_widget( xml, "treeview_notes_list" );
  if ( !treeview ){
    g_warning("Can NOT get treeview widget");
    return;
  }

  store = gtk_list_store_new(
    3, GTK_TYPE_INT, GTK_TYPE_STRING,  GTK_TYPE_STRING
  );

  query = "select * from notes";
  //rc = sqlite3_compile( db, "select * from notes", NULL, &vm, &error );
  rc = sqlite3_prepare ( db, query, strlen (query), &vm, &tail );
  if ( rc != SQLITE_OK ) {
    g_print( "compile query error: (%d)%s\n", rc, sqlite3_errmsg(db) );

    if ( rc == SQLITE_ERROR ) {
      rc = sqlite3_exec(
        db, "create table notes( \
            id integer primary key ,\
            title string, text string, datetime string \
          )",
        NULL, NULL, &error
      );
      if ( rc != SQLITE_OK) g_warning(
        "creating notes table: (%d)%s\n", rc, sqlite3_errmsg(db)
      );
    }
  } else {
    //while ( sqlite3_step( vm, &cn, &cvals, &cnames ) != SQLITE_DONE ) {
    while (sqlite3_step (vm) != SQLITE_DONE) {
      gtk_list_store_append ( store, &iter );
      gtk_list_store_set(
        store, &iter, 0,
        sqlite3_column_int (vm, 0),
        1,
        sqlite3_column_text (vm, 1),
        2,
        sqlite3_column_text (vm, 3),
        -1
        //store, &iter, 0, atoi(cvals[0]), 1, cvals[1], 2, cvals[3], -1
      );
    }
    rc = sqlite3_finalize( vm);
  }
  gtk_tree_view_set_model( GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store) );

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes (
    "Title", renderer, "text", 1, NULL
  );
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes (
    "Date", renderer, "text", 2, NULL
  );
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  g_signal_connect(
    G_OBJECT (select), "changed",
    G_CALLBACK (on_treeview_notes_list_selection_changed), win
  );
  
  // end: set the treeview

  // begin:attach some extra information to window
  wid = glade_xml_get_widget( xml, "entry_note_title" );
  g_object_set_data( G_OBJECT(win), "entry_note_title", wid );
  wid = glade_xml_get_widget( xml, "textview_note_text" );
  g_object_set_data(
    G_OBJECT(win), "textbuffer_note_text",
    gtk_text_view_get_buffer( GTK_TEXT_VIEW(wid) )
  );
  wid = glade_xml_get_widget( xml, "treeview_notes_list" );
  g_object_set_data( G_OBJECT(win), "treeview_notes_list", wid );

  g_object_unref( G_OBJECT(xml) );
}

EXPORT void module_notes_toolbar_modify( GtkWidget *toolbar ){
//FIXME: since we use a newer version of GTK on windows, we
//have to make this check below. There needs to be a GTK version
//check, or use the GtkUI manager to handle modifying the toolbar
#ifdef GTK_GREATER_THAN
  GtkToolItem *item;

  item = gtk_tool_button_new_from_stock("gtk-index");
  gtk_tool_button_set_label( GTK_TOOL_BUTTON(item), "Not_es" );
  gtk_tool_button_set_use_underline( GTK_TOOL_BUTTON(item), TRUE );
  g_signal_connect( item, "clicked", G_CALLBACK( on_button_notes_clicked ), NULL );
  gtk_toolbar_insert( GTK_TOOLBAR(toolbar), item, -1 );
  gtk_widget_set_name( GTK_WIDGET(item), "button_notes" );
  gtk_widget_show( GTK_WIDGET(item) );
#else
  GtkWidget *icon;
        GtkWidget *button;
	

        icon = gtk_image_new_from_stock (
                "gtk-index", gtk_toolbar_get_icon_size( GTK_TOOLBAR (toolbar) )
        );
        button = gtk_toolbar_append_element(
                GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, "Not_es",
                NULL, NULL, icon, G_CALLBACK(on_button_notes_clicked), NULL
        );

        gtk_label_set_use_underline(
                GTK_LABEL(((GtkToolbarChild*)(g_list_last(
                        GTK_TOOLBAR (toolbar)->children)->data))->label
                ), TRUE
        );
        gtk_widget_set_name( button, "button_notes" );
        gtk_widget_show( button );
#endif
  mdebug (DBG_MODULE, 0, ">>> toolbar_modify");
}

EXPORT void module_notes_toolbar_reset( GtkWidget *toolbar ){
  GList *l;
  l = gtk_container_get_children( GTK_CONTAINER( toolbar ) );
  while ( l ) {
    if (!strcmp( gtk_widget_get_name(GTK_WIDGET(l->data)), "button_notes")){
      gtk_widget_destroy( GTK_WIDGET(l->data ) );
    }
    l = g_list_next( l );
  }
  mdebug (DBG_MODULE, 0, ">>> toolbar_reset");
}


