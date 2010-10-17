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

#include <glade/glade.h>
#include <gtk/gtk.h>
#include <string.h>
#include <mudmagic.h>
#include <sqlite3.h>
#include <time.h>
#include <database.h>
#include <interface.h>
#include <directories.h>

#define DBQUERYLEN 2048

void on_db_close_clicked( GtkButton *button, gpointer user_data );

GtkWidget* get_widget( GtkWidget *wid, gchar *name ) {
  GtkWidget *ret;
  g_return_val_if_fail( wid != NULL, NULL );
  ret = glade_xml_get_widget( glade_get_widget_tree(wid), name);
  if ( !ret ) {
    g_warning(" %s not found (from %s)\n", name, wid->name);
  }
  return ret;
}

void treeview_table_field_changed( GtkCellRendererText* cell, gchar* arg1, gchar* arg2, gpointer tv ) {
  GtkWidget *win, *wid;
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeIter iter;
  gchar *old_val;
  gint col;
  gint id,i;

  sqlite3 *db = NULL ;
  gchar *error;
  gchar *buff;
  gchar query[DBQUERYLEN];
  gchar *p, *q;
  gint rc;

  g_return_if_fail( cell != NULL );
  g_return_if_fail( tv != NULL );
  treeview = tv;

  store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) );
  g_return_if_fail( store != NULL );

  win = gtk_widget_get_toplevel( GTK_WIDGET( treeview ));
  g_return_if_fail( win != NULL );

  db = g_object_get_data( G_OBJECT(win), "database" );
  g_return_if_fail( db != NULL );

  col = GPOINTER_TO_INT(g_object_get_data( G_OBJECT(cell), "col" ));
  gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL(store), &iter, arg1 );
  gtk_tree_model_get( GTK_TREE_MODEL(store), &iter, col, &old_val,  -1);
  g_free( old_val );
  gtk_list_store_set( store, &iter, col, arg2, -1 );

  wid = g_object_get_data( G_OBJECT( win ), "combo_table" );
  g_return_if_fail( wid != NULL );

  gtk_tree_model_get( GTK_TREE_MODEL(store), &iter, 0, &id, -1 );

  buff = g_strdup_printf(
    "delete from %s where id=%d",
    gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(wid)->entry) ), id
  );
  rc = sqlite3_exec( db, buff, NULL, NULL, &error );
  g_free( buff );
  if  ( rc != SQLITE_OK ) {
    g_warning( " error %d :%s\n", rc, error );
    if (error != NULL ) {
      sqlite3_free(error); error = NULL;
    }
    return;
  }
  memset( query, '\0', DBQUERYLEN ); p = query ;
  q = g_stpcpy( p , "insert into " ); p = q ;
  q = g_stpcpy( p , gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(wid)->entry)));p=q;
  q = g_stpcpy( p , " values ( " ); p = q ;
  buff = g_strdup_printf("%d", id );
  q = g_stpcpy( p , buff ); p = q ;
  g_free( buff );

  for ( i = 1 ; i<gtk_tree_model_get_n_columns(GTK_TREE_MODEL(store)); i++ ) {
    q = g_stpcpy( p , ", '" ); p = q ;
    gtk_tree_model_get( GTK_TREE_MODEL(store), &iter, i, &buff, -1 );
    q = g_stpcpy( p , buff ); p = q ;
    g_free(buff);
    q = g_stpcpy( p , "'" ); p = q ;
  }
  q = g_stpcpy( p , ")" ); p = q ;

  rc = sqlite3_exec( db, query, NULL, NULL, &error );
  if  ( rc != SQLITE_OK ) {
    g_warning( " error %d :%s\n", rc, error );
    if (error != NULL ) {
      sqlite3_free(error); error = NULL;
    }
  }

}

void treeview_table_display( GtkWidget *win, gchar *table_name ) {
  GtkWidget *treeview;
  GtkListStore *store ;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeIter iter;

  sqlite3 *db = NULL;
  sqlite3_stmt *vm = NULL;
  gchar *query = NULL;
  const gchar *tail;
  gint i;
  gint rc;
  gint cn;

  GType *types;
  GList *list,*l;

  db = g_object_get_data( G_OBJECT(win), "database" );
  g_return_if_fail( db != NULL );

  treeview = get_widget( win, "treeview_table" );
  g_return_if_fail( treeview != NULL );

  // remove the old model ...
  store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) );
  gtk_tree_view_set_model( GTK_TREE_VIEW(treeview), NULL );
  // ... and columns
  l = list = gtk_tree_view_get_columns( GTK_TREE_VIEW(treeview) );
  while ( l != NULL ) {
    gtk_tree_view_remove_column(
      GTK_TREE_VIEW( treeview ),
      GTK_TREE_VIEW_COLUMN( l->data )
    );
    l = g_list_next( l );
  }
  g_list_free( list );


  if ( strcmp( table_name, "Select a table") == 0 ) {
    return;
  }

  // get columns name from new table
  query = g_strdup_printf("select * from %s where 1=2", table_name);
  rc = sqlite3_prepare ( db, query, strlen (query), &vm, &tail );
  g_free( query );
  if  ( rc != SQLITE_OK ) {
    g_warning( " error %d :%s\n", rc, sqlite3_errmsg(db) );
    return;
  } else {
    //rc = sqlite3_step( vm, &cn, &cvals, &cnames );
    rc = sqlite3_step( vm );
    if ( rc != SQLITE_DONE ) {
      g_warning( " error \n" );
    }
    cn = sqlite3_column_count (vm);
    types = (GType*)g_malloc0( cn * sizeof( GType* ) );
    types[0] = G_TYPE_INT;
    for ( i = 1 ; i < cn ; i++ ) types[i] = G_TYPE_STRING;
    store = gtk_list_store_newv( cn, types );
    gtk_tree_view_set_model(
      GTK_TREE_VIEW(treeview),
      GTK_TREE_MODEL(store)
    );
    for ( i = 1 ; i < cn ; i++ ) {
      renderer = gtk_cell_renderer_text_new( );
      g_object_set(renderer, "editable", TRUE, NULL );
      g_object_set_data( G_OBJECT(renderer), "col", GINT_TO_POINTER(i) );

      g_signal_connect(
        renderer, "edited",
        (GCallback)treeview_table_field_changed, treeview
      );

      column = gtk_tree_view_column_new_with_attributes(
        sqlite3_column_name (vm, i),
        renderer, "text", i, NULL
        //cnames[i], renderer, "text", i, NULL
      );
      gtk_tree_view_append_column(GTK_TREE_VIEW (treeview), column);
    }
    rc = sqlite3_finalize( vm );
  }

  // put data in table view
  query = g_strdup_printf("select * from %s", table_name);
  //rc = sqlite3_compile( db, query, NULL, &vm, &error );
  rc = sqlite3_prepare( db, query, strlen (query), &vm, &tail );
  g_free( query );
  if  ( rc != SQLITE_OK ) {
    g_warning( " error %d :%s\n", rc, sqlite3_errmsg(db));
    return;
  } else {
    //while ( sqlite3_step( vm, &cn, &cvals, &cnames ) != SQLITE_DONE ) {
    while ( sqlite3_step( vm ) != SQLITE_DONE ) {
      gtk_list_store_append( store, &iter );
      //gtk_list_store_set( store, &iter, 0, atoi(cvals[0]), -1 );
      gtk_list_store_set( store, &iter, 0, sqlite3_column_int (vm, 0), -1 );
      for ( i = 1 ; i < cn ; i++ ) {
        //gtk_list_store_set( store, &iter, i, cvals[i], -1 );
        gtk_list_store_set( store, &iter, i,
                sqlite3_column_text (vm, i), -1 );
      }

    }
    rc = sqlite3_finalize( vm);
  }

}

void treeview_columns_list_changed( GtkCellRendererText* cell, gchar* arg1, gchar* arg2, gpointer store ) {
  GtkTreeIter iter;
  gchar *old_name;
  g_return_if_fail( cell != NULL );
  g_return_if_fail( store != NULL );
  gtk_tree_model_get_iter_from_string( store, &iter, arg1 );
  gtk_tree_model_get( store, &iter, 0, &old_name,  -1);
  g_free( old_name );
  gtk_list_store_set( store, &iter, 0, arg2, -1 );
}

void treeview_model_list_init( GtkWidget *treeview, gchar *head, gboolean editable, GCallback func){
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  g_return_if_fail(treeview != NULL && head != NULL );

  store = gtk_list_store_new( 1, G_TYPE_STRING );

  renderer = gtk_cell_renderer_text_new( );
  g_object_set(renderer, "editable", editable, NULL );
  if ( func ) {
    g_signal_connect( renderer, "edited", func, store );
  }
  column = gtk_tree_view_column_new_with_attributes(
    head, renderer, "text", 0, NULL
  );
  gtk_tree_view_append_column(GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_set_model(
    GTK_TREE_VIEW( treeview),
    GTK_TREE_MODEL( store )
  );
}

static void update_tables_lists( GtkWidget *win ) {
  GtkWidget *wid;
  GtkWidget *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  sqlite3 *db     = NULL;
  sqlite3_stmt *vm = NULL;
  const char *error = NULL;
  gchar *query;
  gint rc;

  GList *list = NULL, *l;
  gchar *s;
  gchar *tmp;

  db = g_object_get_data( G_OBJECT(win), "database" );
  g_return_if_fail( db != NULL );

  query = "select name from sqlite_master where type='table'" ;
  //rc = sqlite3_compile( db, query, NULL, &vm, &error );
  rc = sqlite3_prepare ( db, query, strlen (query), &vm, &error );
  if ( rc != SQLITE_OK ) {
    g_warning( " error %d :%s\n", rc, sqlite3_errmsg(db) );
    return;
  } else {
    //while ( sqlite3_step( vm, &cn, &cvals, &cnames ) != SQLITE_DONE ) {
    while ( sqlite3_step( vm ) != SQLITE_DONE ) 
    {
	tmp = g_strdup_printf("%s", sqlite3_column_text(vm, 0) );
       list = g_list_append( list, tmp );
    }
    rc = sqlite3_finalize( vm);

    // set tables name in combo
    wid = g_object_get_data( G_OBJECT( win ), "combo_table" );
    g_return_if_fail( wid != NULL );
    gtk_combo_set_popdown_strings( GTK_COMBO(wid), list );

    // set table names in treeview
    treeview = get_widget( win, "treeview_tables_list" );
    g_return_if_fail( treeview != NULL );

    store = (GtkListStore*)gtk_tree_view_get_model(
      GTK_TREE_VIEW(treeview)
    );
    g_return_if_fail( store != NULL );

    gtk_list_store_clear( store );
    l = list ;
    while ( l != NULL ) {
      s = (gchar*)l->data;
      gtk_list_store_append( store, &iter );
      gtk_list_store_set( store, &iter, 0, s, -1 );
      g_free( s );
      l = g_list_remove( l, s );
    }
  }
}

void on_button_row_add_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *treeview, *win, *wid;
  GtkListStore *store;
  GtkTreeIter iter;

  sqlite3 *db = NULL;
  gchar *error;
  gchar query[DBQUERYLEN];
  gint rc;

  gchar *p, *q;
  gint id,i;

  win = gtk_widget_get_toplevel( GTK_WIDGET( button));
  g_return_if_fail( win != NULL );

  treeview = get_widget( win, "treeview_table" );
  if( treeview == NULL )
	g_print("kyndig: no treeview\n");

  g_return_if_fail( treeview != NULL );

  store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) );
  if( store == NULL )
	g_print("kyndig: no store\n");

  g_return_if_fail( store != NULL );

  db = g_object_get_data( G_OBJECT(win), "database" );
  if( db == NULL )
	g_print("kyndig: no db\n");

  g_return_if_fail( db != NULL );

  wid = g_object_get_data( G_OBJECT( win ), "combo_table" );
  if( wid == NULL )
	g_print("kyndig: no combo_table\n");

  g_return_if_fail( wid != NULL );


  memset( query, '\0', DBQUERYLEN ); p = query ;
  q = g_stpcpy( p , "insert into " ); p = q ;
  q = g_stpcpy( p , gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(wid)->entry)));p=q;
  q = g_stpcpy( p , " values ( NULL" ); p = q ;
  for ( i = 1 ; i<gtk_tree_model_get_n_columns(GTK_TREE_MODEL(store)); i++ ) {
    q = g_stpcpy( p , ", 'N/A'" ); p = q ;
  }
  q = g_stpcpy( p , ")" ); p = q ;
  rc = sqlite3_exec( db, query, NULL, NULL, &error );
  if  ( rc != SQLITE_OK ) {
    g_warning( " error %d :%s\n", rc, error );
    if (error != NULL ) {
      sqlite3_free(error); error = NULL;
    }
    return;
  }
  id = sqlite3_last_insert_rowid( db );

  gtk_list_store_append( store, &iter );
  gtk_list_store_set( store, &iter, 0, id, -1 );
  for ( i = 1 ; i<gtk_tree_model_get_n_columns(GTK_TREE_MODEL(store)); i++ ) {
    gtk_list_store_set( store, &iter, i, "N/A", -1 );
  }

}
 void on_button_row_del_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *treeview, *win, *wid;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  sqlite3 *db = NULL;
  gchar *error;
  gchar *query;
  gint rc;

  gint id;

  win = gtk_widget_get_toplevel( GTK_WIDGET( button));
  g_return_if_fail( win != NULL );

  treeview = get_widget( win, "treeview_table" );
  g_return_if_fail( treeview != NULL );

  selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ) );
  g_return_if_fail( selection != NULL );

  db = g_object_get_data( G_OBJECT(win), "database" );
  g_return_if_fail( db != NULL );

  wid = g_object_get_data( G_OBJECT( win ), "combo_table" );
  g_return_if_fail( wid != NULL );

  if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
    gtk_tree_model_get( model, &iter, 0, &id, -1 );
    query = g_strdup_printf(
      "delete from %s where id = %d",
      gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(wid)->entry) ), id
    );
    rc = sqlite3_exec( db, query, NULL, NULL, &error );
    g_free( query );
    if ( rc != SQLITE_OK ) {
      g_warning( " error %d :%s\n", rc, error );
      if (error != NULL ) {
        sqlite3_free(error); error = NULL;
      }
      return;
    } else {
      gtk_list_store_remove( GTK_LIST_STORE(model), &iter );
    }
  } else {
    interface_display_message("There is no row selected !");
    return;
  }

}
 void on_button_col_add_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *treeview;
  GtkListStore *store;
  GtkTreeIter iter;
  treeview = get_widget( GTK_WIDGET(button), "treeview_columns_list" );
  g_return_if_fail( treeview != NULL );
  store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) );
  g_return_if_fail( store != NULL );

  gtk_list_store_append( store, &iter );
  gtk_list_store_set( store, &iter, 0, "New Column", -1 );

}
 void on_button_col_del_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  treeview = get_widget( GTK_WIDGET(button), "treeview_columns_list" );
  g_return_if_fail( treeview != NULL );

  selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ) );
  g_return_if_fail( selection != NULL );

  if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
    gtk_list_store_remove( GTK_LIST_STORE(model), &iter );
  } else {
    interface_display_message("There is no name selected !");
    return;
  }
}
 void on_button_col_up_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter, iter2;
  GtkTreePath *path;

  treeview = get_widget( GTK_WIDGET(button), "treeview_columns_list" );
  g_return_if_fail( treeview != NULL );

  selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ) );
  g_return_if_fail( selection != NULL );

  if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
    path = gtk_tree_model_get_path( model, &iter );
    gtk_tree_path_prev( path );
    if (gtk_tree_model_get_iter( model, &iter2, path ) )
      gtk_list_store_swap( GTK_LIST_STORE(model), &iter, &iter2 );
    gtk_tree_path_free( path );

  } else {
    interface_display_message("There is no name selected !");
    return;
  }
}
 void on_button_col_down_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter, iter2;
  GtkTreePath *path;

  treeview = get_widget( GTK_WIDGET(button), "treeview_columns_list" );
  g_return_if_fail( treeview != NULL );

  selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ) );
  g_return_if_fail( selection != NULL );

  if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
    path = gtk_tree_model_get_path( model, &iter );
    gtk_tree_path_next( path );
    if (gtk_tree_model_get_iter( model, &iter2, path ) )
      gtk_list_store_swap( GTK_LIST_STORE(model), &iter, &iter2 );
    gtk_tree_path_free( path );

  } else {
    interface_display_message("There is no name selected !");
    return;
  }
}

 void on_button_table_drop_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *treeview, *win;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name;

  sqlite3 *db = NULL;
  gchar *query;
  gchar *error;
  gint rc;

  win = gtk_widget_get_toplevel( GTK_WIDGET( button ));
  g_return_if_fail( win );

  db = g_object_get_data( G_OBJECT(win), "database" );
  g_return_if_fail( db != NULL );

  treeview = get_widget( win, "treeview_tables_list" );
  g_return_if_fail( treeview != NULL );

  selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ) );
  g_return_if_fail( selection != NULL );

  if ( gtk_tree_selection_get_selected( selection, &model, &iter ) ) {
    gtk_tree_model_get( model, &iter, 0, &name, -1 );
    query = g_strdup_printf("drop table %s", name);
    g_free( name );
    rc = sqlite3_exec( db, query, NULL, NULL, &error );
    g_free( query );
    if ( rc != SQLITE_OK ) {
      g_warning( " error %d :%s\n", rc, error );
      if (error != NULL ) {
        sqlite3_free(error); error = NULL;
      }
      return;
    } else {
      update_tables_lists( win );
    }
  } else {
    interface_display_message("There is no name selected !");
    return;
  }
}

 void on_button_table_create_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *wid, *win;
  GtkWidget *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  sqlite3 *db = NULL;
  gchar *error = NULL;
  gchar query[DBQUERYLEN];
  gint rc;

  const gchar *s;
  gchar *p, *q, *name;

  // get database window
  win = gtk_widget_get_toplevel( GTK_WIDGET( button ));
  g_return_if_fail( win != NULL );

  // get database
  db = g_object_get_data( G_OBJECT(win), "database" );
  g_return_if_fail( db != NULL );

  // get table name
  wid = get_widget( GTK_WIDGET(button), "entry_table_name" );
  g_return_if_fail( wid != NULL );
  s = gtk_entry_get_text( GTK_ENTRY(wid) );
  g_return_if_fail( s != NULL );
  if ( *s == '\0' || g_ascii_isspace( *s ) ) {
    interface_display_message("Invalid table name !");
    return;
  }

  // get treeview
  treeview = get_widget( GTK_WIDGET(button), "treeview_columns_list" );
  g_return_if_fail( treeview != NULL );

  // get model from tree view
  store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) );
  g_return_if_fail( store != NULL );

  memset( query, '\0', DBQUERYLEN ); p = query ;
  // copute query
  q = g_stpcpy( p , "create table " ); p = q ;
  q = g_stpcpy( p , s ); p = q ; // add table name
  q = g_stpcpy( p , "( id integer primary key " ); p = q ;

  if (gtk_tree_model_get_iter_first( GTK_TREE_MODEL(store), &iter )) {
    gtk_tree_model_get( GTK_TREE_MODEL(store), &iter, 0, &name, -1 );
    q = g_stpcpy( p , ", " ); p = q;
    if ( DBQUERYLEN - (p - query) < strlen(name) + 1 ) {
      g_warning(" query too long " ); g_free(name);
      return ;
    }
    q = g_stpcpy( p , name ); p = q ;
    g_free( name );
  }
  while (gtk_tree_model_iter_next( GTK_TREE_MODEL(store), &iter )) {
    gtk_tree_model_get( GTK_TREE_MODEL(store), &iter, 0, &name, -1 );
    q = g_stpcpy( p , ", " ); p = q;
    if ( DBQUERYLEN - (p - query) < strlen(name) + 2 ) {
      g_warning(" query too long " ); g_free(name);
      return ;
    }
    q = g_stpcpy( p , name ); p = q ;
    g_free( name );
  }
  q = g_stpcpy( p , ")" ); p = q ;

  rc = sqlite3_exec( db, query, NULL, NULL, &error );
  if ( rc != SQLITE_OK ) {
    if ( error != NULL )
      interface_display_message( error );
    g_free( error ); error = NULL ;
  } else {
    gtk_list_store_clear( store );
    gtk_entry_set_text( GTK_ENTRY(wid), "" );
    update_tables_lists( win );
  }

}

void combo_table_changed( GtkEntry *entry, gpointer data ) {
  GtkWidget *win;
  gchar *name;
  win = gtk_widget_get_toplevel( GTK_WIDGET( entry ));
  g_return_if_fail( win != NULL );

  name = g_strdup( gtk_entry_get_text(entry) );
  if ( *name != '\0' ) {
    treeview_table_display( win, name );
  }
  g_free( name );
}


void on_button_database_clicked( GtkButton *button, gpointer user_data ){
  GladeXML *xml;
  SESSION_STATE *session;
  gchar *ui_file;
  gchar *db_file;
  GtkWidget *win, *wid, *wid_c;
  int rc;

  sqlite3 *db     = NULL;
  gchar  *error = NULL;

  // get current session
  session = interface_get_active_session();
  if ( session==NULL ) {
    interface_display_message( "There is no active session.");
    return;
  }

  // open database
  db_file = g_build_path(
    G_DIR_SEPARATOR_S, session->slot, "database.db", NULL
  );
  rc = sqlite3_open( db_file, &db);
  if ( rc != SQLITE_OK ){
    g_warning("Couldn't open database: %s", error );
    return;
  }
  g_free( db_file );

  // open database window
  ui_file = g_build_filename(
    mudmagic_data_directory(), "interface", "database.glade", NULL 
  );
  xml = glade_xml_new( ui_file, "window_database", NULL);
  g_return_if_fail( xml != NULL );

#ifdef HAVE_WINDOWS
    GtkWidget *gtk_button;

    gtk_button =  glade_xml_get_widget (xml,"button_table_create");
    g_signal_connect( gtk_button, "clicked", G_CALLBACK( on_button_table_create_clicked ), NULL );
	
    gtk_button =  glade_xml_get_widget (xml,"button_table_drop");
    g_signal_connect( gtk_button, "clicked", G_CALLBACK( on_button_table_drop_clicked ), NULL );

    gtk_button =  glade_xml_get_widget (xml,"db_close");
    g_signal_connect( gtk_button, "clicked", G_CALLBACK( on_db_close_clicked ), NULL );

    gtk_button =  glade_xml_get_widget (xml,"button_row_add");
    g_signal_connect( gtk_button, "clicked", G_CALLBACK( on_button_row_add_clicked ), NULL );

    gtk_button =  glade_xml_get_widget (xml,"button_row_del");
    g_signal_connect( gtk_button, "clicked", G_CALLBACK( on_button_row_del_clicked ), NULL );

#else
  glade_xml_signal_autoconnect( xml );
#endif

  g_free( ui_file );

  // attach db to window
  win = glade_xml_get_widget( xml, "window_database" );
  g_return_if_fail( win != NULL );
  g_object_set_data( G_OBJECT(win), "database", (gpointer)db );


  // add the combo with talbles by hand to avoid a windows libglade bug
  wid_c = glade_xml_get_widget( xml, "combo_c");
  g_return_if_fail( wid_c != NULL );
  wid = gtk_combo_new( );
  gtk_widget_show( wid );

  gtk_container_add( GTK_CONTAINER(wid_c), wid );

  //gtk_combo_set_value_in_list( GTK_COMBO( wid ), TRUE, TRUE );

  g_signal_connect(
    G_OBJECT( GTK_COMBO(wid)->entry ), "changed",
    G_CALLBACK(combo_table_changed), NULL
  );

  g_object_set_data( G_OBJECT( win ), "combo_table", wid );
  gtk_entry_set_editable( GTK_ENTRY(GTK_COMBO(wid)->entry), FALSE );

  // some signal connection
  wid = get_widget( win, "treeview_columns_list" );
  treeview_model_list_init(
    wid, "Column name", TRUE, (GCallback)treeview_columns_list_changed
  );

  wid = get_widget( win, "treeview_tables_list" );
  treeview_model_list_init(wid, "Tables name", FALSE, NULL );

  update_tables_lists( win );

}

 void on_db_close_clicked( GtkButton *button, gpointer user_data ){
  GtkWidget *win;
  sqlite3 *db = NULL;

  win = gtk_widget_get_toplevel( GTK_WIDGET( button ));
  g_return_if_fail( win != NULL );
  db = g_object_get_data( G_OBJECT(win), "database" );
  if ( db != NULL ) {
    sqlite3_close( db );
  }
  gtk_widget_destroy( win );

}

/*************************** module entries *********************************/

 void module_database_load() {
  mdebug (DBG_MODULE, 0, "Module database is loaded.");
}

 void module_database_unload (void) {
  mdebug (DBG_MODULE, 0, "module database is unloaded");
}

 void module_database_toolbar_modify( GtkWidget *toolbar ){
//FIXME: since we use a newer version of GTK on windows, we
//have to make this check below. There needs to be a GTK version
//check, or use the GtkUI manager to handle modifying the toolbar
#ifdef GTK_GREATER_THAN
      GtkToolItem *item;

  item = gtk_tool_button_new_from_stock("gtk-convert");
  gtk_tool_button_set_label( GTK_TOOL_BUTTON(item), "_Database" );
  gtk_tool_button_set_use_underline( GTK_TOOL_BUTTON(item), TRUE );
  g_signal_connect( item, "clicked", G_CALLBACK(on_button_database_clicked), NULL );
  gtk_toolbar_insert( GTK_TOOLBAR(toolbar), item, -1 );

  gtk_widget_set_name( GTK_WIDGET(item), "button_database" );
  gtk_widget_show( GTK_WIDGET(item) );
#else
        GtkWidget *icon;
        GtkWidget *button;
        icon = gtk_image_new_from_stock (
                "gtk-convert", gtk_toolbar_get_icon_size( GTK_TOOLBAR (toolbar) )
        );

        button = gtk_toolbar_append_element(
                GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, "_Database",
                NULL, NULL, icon, G_CALLBACK(on_button_database_clicked), NULL
        );
        gtk_label_set_use_underline(
                GTK_LABEL(((GtkToolbarChild*)(g_list_last(
                        GTK_TOOLBAR (toolbar)->children)->data))->label
                ), TRUE
        );
  gtk_widget_set_name( GTK_WIDGET(button), "button_database" );
        gtk_widget_show( GTK_WIDGET(button) );

	mdebug (DBG_MODULE, 0, ">>> toolbar_modify.\n");
#endif
}

 void module_database_toolbar_reset( GtkWidget *toolbar ){
  GList *l;
  l = gtk_container_get_children( GTK_CONTAINER( toolbar ) );
  while ( l != NULL ) {
    if (!strcmp( gtk_widget_get_name(GTK_WIDGET(l->data)), "button_database")){
      gtk_widget_destroy( GTK_WIDGET(l->data ) );
    }
    l = g_list_next( l );
  }
  mdebug (DBG_MODULE, 0, ">>> toolbar_reset");
}

