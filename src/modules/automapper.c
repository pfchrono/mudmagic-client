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
#include "automapper.h"
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <stdlib.h>
#include <mudmagic.h>
#include <interface.h>
#include <directories.h>

#include <sqlite3.h>
#include <unistd.h>

/******************** internal structures functions ********************/
static void debug_atlas_dump( ATLAS *atlas ) {
  GList *m;
  MAP *map;
  GList *n;
  NODE *node;
  GList *p;
  PATH *path;

  g_return_if_fail( atlas );
  g_print("begin ATLAS dump\n");
  m = atlas->maps;
  while ( m ) {
    map = (MAP*)m->data;
    g_print( 
      "\t>>> MAP name=%s id=%d max_node_id=%d\n", 
      map->name, map->id, map->max_node_id
    );
    n = map->nodes;
    while ( n ) {
      node = (NODE*)n->data;
      g_print( 
        "\t\t>>> NODE name=%s id=%d from map_id:%d\n", 
        node->name, node->id, node->map->id
      );
      p = node->paths_out;
      while ( p ) {
        path = (PATH*)p->data;
        g_print( 
          "\t\t\t>>> PATH out command=%s to map:%d node:%d\n",
          path->command, path->map_id, path->node_id
        );
        p = g_list_next( p );
      }
      p = node->paths_in;
      while ( p ) {
        path = (PATH*)p->data;
        g_print( 
          "\t\t\t>>> PATH in  command=%s from map:%d node:%d\n",
          path->command, path->map_id, path->node_id
        );
        p = g_list_next( p );
      }
      n = g_list_next( n );
    }
    m = g_list_next( m );
  }
  
  g_print("end ATLAS dump\n");
}


static PATH* automapper_path_new( const gchar *command, gint map_id, gint node_id ) {
  PATH* ret;
  ret = g_new0( PATH, 1);
  ret->command = g_strdup( command );
  ret->map_id = map_id;
  ret->node_id = node_id;
  return ret;
}

static NODE *automapper_atlas_get_node( ATLAS *atlas, gint map_id, gint node_id ){
  MAP *map = NULL;
  NODE *node = NULL;
  //g_print("looking for node (map_id=%d)(node_id=%d)\n", map_id, node_id ); 
  g_return_val_if_fail( atlas, NULL );
  map = g_hash_table_lookup( atlas->maps_id, GINT_TO_POINTER(map_id) );
  g_return_val_if_fail( map, NULL );
  node = g_hash_table_lookup( map->nodes_id, GINT_TO_POINTER(node_id) );
  g_return_val_if_fail( node, NULL );
  //g_print("exit automapper_atlas_get_node\n");
  return node;
}


static void automapper_path_delete( PATH* path ) {
  if ( !path ) return;
  if ( path->command ) g_free( path->command );
  g_free( path );
}

static NODE* automapper_node_new( gint id, gint x, gint y ){
  NODE* ret;
  gint i;
  ret = g_new0( NODE, 1 ); 
  ret->id = id;
  ret->position.x = x;
  ret->position.y = y;
  ret->name = g_strdup_printf( "#%d", id );
  for ( i = 0 ; i < 8 ; i++ ) 
    ret->links_in[i] = ret->links_out[i] = -1;
  return ret;
}

static void automapper_node_delete( NODE* node ) {
  GList *l; 
  ATLAS *atlas = NULL ;
  NODE *tnode = NULL ;
  PATH *path = NULL ;
  g_return_if_fail( node );
  
  // get the atlas
  if ( node->map ) 
    atlas = node->map->atlas;
  if ( !atlas ) {
    g_critical( "Corupted atlas." );
  } 

  // clean paths_out list
  l = node->paths_out ; 
  while ( l ) {
    path = (PATH*)l->data;
    // get the node where path goes in 
    tnode = automapper_atlas_get_node( atlas, path->map_id, path->node_id );
    if (tnode) {
      // delete the corespondig entry from paths_in list
      GList *l2 = tnode->paths_in;
      PATH *path2;
      while ( l2 ) {
        path2 = (PATH*)l2->data;
        if ((path2->node_id == node->id ) &&
            (path2->map_id == node->map->id) &&
          !strcmp( path->command, path2->command )) {
          tnode->paths_in = g_list_remove( tnode->paths_in, path2 );
          break;
        }
        l2 = g_list_next( l2 );
      }
    }
    automapper_path_delete( l->data );
    l = g_list_next ( l );
  }
  g_list_free ( node->paths_out );

  // clean paths_in list
  l = node->paths_in; 
  while ( l ) {
    path = (PATH*)l->data;
    // get the node where path goes in 
    tnode = automapper_atlas_get_node( atlas, path->map_id, path->node_id );
    if (tnode) {
      // delete the corespondig entry from paths_out list
      GList *l2 = tnode->paths_out;
      PATH *path2;
      while ( l2 ) {
        path2 = (PATH*)l2->data;
        if ((path2->node_id == node->id ) &&
            (path2->map_id == node->map->id) &&
          !strcmp( path->command, path2->command )) {
          tnode->paths_out = g_list_remove( tnode->paths_out, path2 );
          break;
        }
        l2 = g_list_next( l2 );
      }
    }
    automapper_path_delete( l->data );
    l = g_list_next ( l );
  }
  g_list_free ( node->paths_in );

  if ( node->name ) g_free( node->name );
  g_free( node );
}

static void automapper_node_set_name( NODE* node, const gchar *name ) {
  g_return_if_fail( node && name );
  if ( node->name ) g_free( node->name );
  node->name = g_strdup( name );
}

static PATH* automapper_node_get_out_path_by_name(NODE *node, gchar *command) {
  PATH *path;
  GList *l;
  if ( (!node) || ( !command )) return NULL; 
  l = node->paths_out;
  while ( l ) {
    path = (PATH*)l->data;
    if ( !strcmp( path->command , command ) ) { 
      return path;
    }
    l = g_list_next ( l );
  }
  return NULL;
}
static guint ghashfunc_position( gpointer key ) {
  POSITION *pos = (POSITION*)key;
  return (abs(pos->x) + abs(pos->y)) % 25; // a simple hash function 
}

static gboolean gequalfunc_position( gpointer a, gpointer b ) {
  POSITION *posa, *posb;
  posa = (POSITION*)a;
  posb = (POSITION*)b;
  return (( posa->x == posb->x ) && ( posa->y == posb->y ));
}

static NODE* automapper_map_add_node( MAP *map, gint node_x, gint node_y ){
  NODE *node;
  g_return_val_if_fail( map, NULL );
  node = automapper_node_new( map->max_node_id++, node_x, node_y ); 
  map->nodes = g_list_append( map->nodes, node );
  g_hash_table_insert( map->nodes_id, GINT_TO_POINTER(node->id) ,node );
  g_hash_table_insert( map->nodes_pos, &node->position, node );
  node->map = map;
  if ( (node_x) < map->minx ) map->minx = node_x;
  if ( (node_y) < map->miny ) map->miny = node_y;
  if ( (node_x) > map->maxx ) map->maxx = node_x;
  if ( (node_y) > map->maxy ) map->maxy = node_y;
  return node;
}

static MAP* automapper_map_new( gint id ) {
  MAP* map;
  map = g_new0( MAP , 1 );
  map->id  = id;
  // we'll use this hashtables to access a node by id or by position
  map->name = g_strdup_printf( "Map #%d", id );
  map->nodes_id = g_hash_table_new( NULL, NULL);
  map->nodes_pos = g_hash_table_new( 
    (GHashFunc)ghashfunc_position, (GEqualFunc)gequalfunc_position 
  );
  // create a initial node
  // map->cnode = automapper_map_add_node( map, 0, 0 );
  
  return map;
}

static void automapper_map_delete( MAP* map ) {
  GList *l;
  POSITION pos;
  NODE *node;
  gint id;
  g_return_if_fail( map );
  g_print(">>> delete map %s (%d)\n", map->name, map->id );
  while ( map->nodes ) {
    l = map->nodes;
    //g_print("Delete node %s (%s)\n", (((NODE*)l->data)->name), map->name);
    node = (NODE*)l->data;
    pos.x = node->position.x;
    pos.y = node->position.y;
    id = node->id;
    automapper_node_delete( node );
    g_hash_table_remove( map->nodes_id, GINT_TO_POINTER( id ) );
    g_hash_table_remove( map->nodes_pos, &pos );
    map->nodes = g_list_remove ( map->nodes, l->data );
  }
  if (map->nodes) g_list_free( map->nodes );
  map->cnode = NULL; // it's already freed
  if ( map->name ) g_free( map->name );
  g_hash_table_destroy( map->nodes_id );
  g_hash_table_destroy( map->nodes_pos );
  g_free( map );
}

static NODE *automapper_map_get_node_by_name( MAP *map, gchar *name ) {
  NODE *node;
  GList *l;
  if ( (!map) || ( !name )) return NULL; 
  l = map->nodes;
  while ( l ) {
    node = (NODE*)l->data;
    if ( !strcmp( node->name, name ) ) { 
      return node;
    }
    l = g_list_next ( l );
  }
  return NULL;
}

static void automapper_map_set_name( MAP *map, const gchar *name ) {
  if (( !map ) || ( !name )) return;
  if ( map->name ) g_free( map->name );
  map->name = g_strdup( name );
}

static void automapper_get_delta( guchar direction, gint *dx, gint *dy ) {
  switch ( direction ) {
    case AM_N  : { *dx = 0; *dy = -1; }  break;
    case AM_NE : { *dx = 1; *dy = -1; }  break;
    case AM_E  : { *dx = 1; *dy =  0; }  break;
    case AM_SE : { *dx = 1; *dy =  1; }  break;
    case AM_S  : { *dx = 0; *dy =  1; }  break;
    case AM_SW : { *dx =-1; *dy =  1; }  break;
    case AM_W  : { *dx =-1; *dy =  0; }  break;
    case AM_NW : { *dx =-1; *dy = -1; }  break;
    default : { *dx = 0; *dy =  0; }  
  }
}

static void automapper_map_move( MAP* map, guchar direction, gboolean bidirectional ){
  gint dx, dy;
  POSITION pos;
  NODE *node;
  if ( !map ) return; // a sanity check :)
  automapper_get_delta( direction, &dx, &dy );
  g_print("dx=%d dy=%d\n", dx, dy );
  pos.x = map->cnode->position.x + dx ;
  pos.y = map->cnode->position.y + dy ;
  
  // try to find if there is a node one step in that direction
  node = g_hash_table_lookup( map->nodes_pos, &pos );
  
  if ( !node ) {
    //g_print("node in pos (%d,%d) not found \n", pos.x, pos.y );
    node = automapper_map_add_node( map, pos.x, pos.y );
  }
  // link the current node with the new node and move there
  node->links_in[ OPPOSITE(direction) ] = map->cnode->id; 
  map->cnode->links_out[ direction ] = node->id;
  if ( bidirectional ){
    map->cnode->links_in[ direction ] = node->id;
    node->links_out[ OPPOSITE(direction) ] = map->cnode->id; 
  } 
  map->cnode = node;
}

static void automapper_map_remove_node( MAP *map, NODE *node ){
  gint i, j, t;
  NODE *tnode;
  if (( !map ) || ( !node ) || ( g_list_length( map->nodes ) == 1 ))
    return;
  
  // remove the node from list
  map->nodes = g_list_remove( map->nodes, node );
  // remove the node from 1st hashtable
  g_hash_table_remove( map->nodes_id, GINT_TO_POINTER(node->id) );
  // remove the node from 2nd hashtable
  g_hash_table_remove( map->nodes_pos, &node->position );

  if ( map->cnode->id == node->id )
    map->cnode = NULL;

  // remove links to and from node
  for ( i = 0 ; i < 8 ; i++ )  {
    if ( (t = node->links_in[i]) != -1 ) {
      tnode = g_hash_table_lookup( map->nodes_id, GINT_TO_POINTER( t ));
      if (tnode) {
        //g_print(" remove link %d-%d\n", tnode->id, node->id );
        for ( j = 0 ; j < 8 ; j ++ )  {
          if ( tnode->links_out[j] == node->id ) 
            tnode->links_out[j] = -1;
        }
        //tnode->links_out[OPPOSITE(i)] = -1;
        if ( !map->cnode )map->cnode = tnode;
      }
    }
    if ( (t = node->links_out[i]) != -1 ) {
      tnode = g_hash_table_lookup( map->nodes_id, GINT_TO_POINTER( t ));
      if (tnode) {
        //g_print(" remove link %d-%d\n", node->id, tnode->id );
        for ( j = 0 ; j < 8 ; j ++ )  {
          if ( tnode->links_in[j] == node->id ) 
            tnode->links_in[j] = -1;
        }
        //tnode->links_in[OPPOSITE(i)] = -1;
        if ( !map->cnode )map->cnode = tnode;
      }
    }
  }
  if ( !map->cnode )map->cnode = map->nodes->data;
  automapper_node_delete( node );
 }


static MAP* automapper_atlas_add_map( ATLAS *atlas ){
  MAP *map;
  g_return_val_if_fail( atlas, NULL );
  map = automapper_map_new( atlas->max_map_id++ );
  atlas->maps = g_list_append( atlas->maps, map );
  g_hash_table_insert( atlas->maps_id, GINT_TO_POINTER(map->id), map );
  map->atlas = atlas;
  return map;
}

static ATLAS *automapper_atlas_new() {
  ATLAS *atlas;
  atlas = g_new0( ATLAS, 1 );
  atlas->maps_id = g_hash_table_new( NULL, NULL );
  atlas->lat = 20;
  atlas->len = 10;
  atlas->zoom = 1.0;
  return atlas;
}

static void automapper_atlas_delete( ATLAS *atlas ) {
  GList *l;
  MAP *map;
  gint id;
  g_return_if_fail( atlas );
  while (atlas->maps) {
    l = atlas->maps;
    map = (MAP*)l->data;
    id = map->id;
    automapper_map_delete( map );
    g_hash_table_remove( atlas->maps_id, GINT_TO_POINTER( id ));
    atlas->maps = g_list_remove ( atlas->maps, l->data );
  }
  if ( atlas->maps ) g_list_free( atlas->maps );
  if ( atlas->maps_id) g_hash_table_destroy( atlas->maps_id );
  g_free( atlas );
}

static MAP* automapper_atlas_get_map_by_name( ATLAS *atlas, gchar *name ) {
  MAP *map;
  GList *l;
  if ( (!atlas) || (!name) ) return NULL;
  l = atlas->maps;
  while ( l ) {
    map = (MAP*)l->data;
    if ( map->name ) 
      if ( !strcmp( map->name, name ) ) 
        return map;
    l = g_list_next ( l );
  }
  return NULL;
}

static void automapper_atlas_save( ATLAS *atlas, gchar *slot ){
  gchar *db_file;
  gchar *error;
  sqlite3 *db;
  gint rc;
  gchar *query;
  GList *m;
  MAP *map;
  GList *n;
  NODE *node;
  GList *p;
  PATH *path;
  
  db_file = g_build_path( G_DIR_SEPARATOR_S, slot, "automapper.db", NULL );
  g_print("save the atlas in : %s\n", db_file );
  if ( unlink( db_file ) == -1 ) {
    mdebug (DBG_MODULE, 0, "There is not a saved atlas.");
  }
  // create database
  rc = sqlite3_open( db_file, &db );
  if (rc != SQLITE_OK){ g_warning("Couldn't open database: %s", error ); return; }
  g_free( db_file );
  // create paths table
  rc = sqlite3_exec( 
    db, "create table PATHS( command string, from_map_id int, from_node_id int, to_map_id int, to_node_id int )" ,
    NULL, NULL, &error 
  );
  if ( rc != SQLITE_OK ){g_warning( "SQL error: %s\n", error ); return; }
  // create nodes table
  rc = sqlite3_exec( 
    db, "create table NODES( \
      id int, x int, y int, name string, map_id int, \
      fN int, fNE int, fE int, fSE int, fS int, fSW int, fW int, fNW int,\
      tN int, tNE int, tE int, tSE int, tS int, tSW int, tW int, tNW int \
    )" ,
    NULL, NULL, &error 
  );
  if ( rc != SQLITE_OK ){g_warning( "SQL error: %s\n", error ); return; }
  // create maps table
  rc = sqlite3_exec( 
    db, "create table MAPS( id int, name string, current_node_id int, max_node_id int )" ,
    NULL, NULL, &error 
  );
  if ( rc != SQLITE_OK ){g_warning( "SQL error: %s\n", error ); return; }
  rc = sqlite3_exec( 
    db, "create table ATLAS( current_map_id int, zoom float, max_map_id )" ,
    NULL, NULL, &error 
  );
  if ( rc != SQLITE_OK ){g_warning( "SQL error: %s\n", error ); return; }
  // we have nice database ... let put some data there
  query = g_strdup_printf(
    "insert into ATLAS VALUES( %d, %f, %d )", 
    atlas->cmap->id, atlas->zoom, atlas->max_map_id
  );
  rc = sqlite3_exec( db, query, NULL, NULL, &error );
  if ( rc != SQLITE_OK ){g_warning( "SQL error: %s\n", error ); return; }
  g_free( query );

  
  m = atlas->maps;
  while ( m ) {
    map = (MAP*)m->data;
    query = g_strdup_printf(
      "insert into MAPS values( %d, \"%s\", %d, %d )", 
      map->id, map->name, map->cnode->id, map->max_node_id 
    );
    rc = sqlite3_exec( db, query, NULL, NULL, &error );
    if ( rc != SQLITE_OK ){g_warning( "SQL error: %s\n", error ); return; }
    g_free( query );
    n = map->nodes;
    while ( n ) {
      node = (NODE*)n->data;
      query = g_strdup_printf(
        "insert into NODES values( %d, %d, %d, \"%s\", %d, \
        %d, %d, %d, %d, %d, %d, %d, %d,\
        %d, %d, %d, %d, %d, %d, %d, %d)", 
        node->id, node->position.x, node->position.y,
        node->name, node->map->id, 
        node->links_out[0], node->links_out[1], 
        node->links_out[2], node->links_out[3], 
        node->links_out[4], node->links_out[5], 
        node->links_out[6], node->links_out[7],
        node->links_in[0], node->links_in[1], 
        node->links_in[2], node->links_in[3], 
        node->links_in[4], node->links_in[5], 
        node->links_in[6], node->links_in[7] 
      );
      rc = sqlite3_exec( db, query, NULL, NULL, &error );
      if ( rc != SQLITE_OK ){
        g_warning( "SQL error: %s\n", error ); return;
      }
      p = node->paths_out;
      while ( p ) {
        path = (PATH*)p->data;
        query = g_strdup_printf(
          "insert into PATHS values( \"%s\", %d, %d, %d, %d )", 
          path->command, node->map->id, node->id, 
          path->map_id, path->node_id
        );
        rc = sqlite3_exec( db, query, NULL, NULL, &error );
        if ( rc != SQLITE_OK ){
          g_warning( "SQL error: %s\n", error ); return; 
        }
        g_free( query );
        p = g_list_next( p );
      }
      n = g_list_next( n );
    }
    m = g_list_next( m );
  }
  sqlite3_close( db );
}

static ATLAS *automapper_atlas_load( gchar *slot ){
  ATLAS *atlas = NULL;
  MAP *map;
  NODE *node;
  PATH *path;
  gchar *db_file;
  gchar *query;
  sqlite3 *db;
  gint rc;
  gint cmap_id = 0;
        gint v0, v1, v2;

  sqlite3_stmt *vm = NULL;  
  const char *col_name;
  const char *error;
  gint i,nc; // number of columns
  

  g_return_val_if_fail( slot, FALSE );
  
  db_file = g_build_path( G_DIR_SEPARATOR_S, slot, "automapper.db", NULL );
  g_print("load atlas from: %s\n", db_file );
  rc = sqlite3_open( db_file, &db );
  if (rc != SQLITE_OK){ g_warning("Couldn't open database: %s", error ); return NULL; }
  g_free( db_file );

  // get ATLAS information
  atlas = automapper_atlas_new();
        query = "select * from ATLAS";
  rc = sqlite3_prepare( db, query, strlen (query), &vm, &error );
  if ( rc != SQLITE_OK ) {
    g_warning( "compile query error: %s\n", error ); 
    return NULL;
  }
        rc = sqlite3_step( vm );
  //rc = sqlite3_step( vm, &nc, &vals, &col_names );
  if ( rc != SQLITE_ROW ) {
    g_warning( "Can NOT get ATLAS row." ); 
    return NULL;
  }
        nc = sqlite3_column_count(vm);
  for ( i = 0 ; i < nc ; i++ ) {
                col_name = sqlite3_column_name (vm, i);
    if ( !strcmp( col_name, "current_map_id" ) ) 
                        cmap_id = sqlite3_column_int (vm, i);
      //cmap_id = atoi( vals[i] );  
    if ( !strcmp( col_name, "zoom" ) ) 
      //atlas->zoom = atof( vals[i] );  
                        atlas->zoom = sqlite3_column_double (vm, i);
    if ( !strcmp( col_name, "max_map_id" ) ) 
      //atlas->max_map_id = atof( vals[i] );
                        atlas->max_map_id = sqlite3_column_double (vm, i);
  }
  rc = sqlite3_finalize( vm); 

  // get MAPS information
        query = "select * from MAPS";
  rc = sqlite3_prepare( db, query, strlen(query), &vm, &error );
  if ( rc != SQLITE_OK ) {
    g_warning( "compile query error: %s\n", error ); 
    automapper_atlas_delete( atlas );
    return NULL;
  }
        while ( sqlite3_step (vm) != SQLITE_DONE)
        {
  //while ( sqlite3_step( vm, &nc, &vals, &col_names ) != SQLITE_DONE ) {
    map = automapper_map_new( sqlite3_column_int (vm, 0) ); 
    automapper_map_set_name( map, sqlite3_column_text (vm, 1) );  
    // store the node id until the proper node is loaded
    map->cnode = GINT_TO_POINTER( -sqlite3_column_int (vm, 2) );
    map->max_node_id = sqlite3_column_int (vm, 3);
    // add this map to atlas
    atlas->maps = g_list_append( atlas->maps, map );
    g_hash_table_insert( atlas->maps_id, GINT_TO_POINTER(map->id), map );
    map->atlas = atlas;
  }
  rc = sqlite3_finalize( vm); 
  
  // get NODES information
        query = "select * from NODES";
  rc = sqlite3_prepare( db, query, strlen (query), &vm, &error );
  if ( rc != SQLITE_OK ) {
    g_warning( "compile query error: %s\n", error ); 
    automapper_atlas_delete( atlas );
    return NULL;
  }
  //while ( sqlite3_step( vm, &nc, &vals, &col_names ) != SQLITE_DONE )
        while (sqlite3_step (vm) != SQLITE_DONE)
        {
                v0 = sqlite3_column_int (vm, 0);
                v1 = sqlite3_column_int (vm, 1);
                v2 = sqlite3_column_int (vm, 2);
    node = automapper_node_new(
                        v0, v1, v2
      //atoi(vals[0]), atoi(vals[1]), atoi(vals[2]) 
    );
    g_print("Create node id=%d x=%d y=%d\n",
                        v0, v1, v2
      //atoi(vals[0]), atoi(vals[1]), atoi(vals[2]) 
    );
    automapper_node_set_name( node, sqlite3_column_text (vm, 3) );
    for ( i = 0 ; i< 8 ; i++ ) 
      //node->links_out[i] = atoi( vals[5 + i] ); 
      node->links_out[i] = sqlite3_column_int (vm, 5 + i); 
    for ( i = 0 ; i< 8 ; i++ ) 
      //node->links_in[i] = atoi( vals[13 + i] ); 
      node->links_out[i] = sqlite3_column_int (vm, 13 + i); 
    map = g_hash_table_lookup( 
      //atlas->maps_id, GINT_TO_POINTER( atoi(vals[4]) )
      atlas->maps_id, GINT_TO_POINTER( sqlite3_column_int(vm, 4) )
    );
    if (!map ) g_error("Coresponding map not found.");
    map->nodes = g_list_append( map->nodes, node );
    g_hash_table_insert( map->nodes_id, GINT_TO_POINTER(node->id), node );
    g_hash_table_insert( map->nodes_pos, &node->position, node );
    node->map = map;
                if (v1 < map->minx) map->minx = v1;
                if (v2 < map->miny) map->miny = v2;
                if (v1 > map->maxx) map->maxx = v1;
                if (v2 > map->maxy) map->maxy = v2;
    //if ( (atoi(vals[1])) < map->minx ) map->minx = atoi(vals[1]);
    //if ( (atoi(vals[2])) < map->miny ) map->miny = atoi(vals[2]);
    //if ( (atoi(vals[1])) > map->maxx ) map->maxx = atoi(vals[1]);
    //if ( (atoi(vals[2])) > map->maxy ) map->maxy = atoi(vals[2]);

    if ( -GPOINTER_TO_INT(map->cnode) == node->id )
      map->cnode= node;
  
  }
  rc = sqlite3_finalize( vm); 

  // get PATHS information
        query = "select * from PATHS";
  rc = sqlite3_prepare( db, query, strlen (query), &vm, &error );
  if ( rc != SQLITE_OK ) {
    g_warning( "compile query error: %s\n", error ); 
    automapper_atlas_delete( atlas );
    return NULL;
  }
  //while ( sqlite_step( vm, &nc, &vals, &col_names ) != SQLITE_DONE )
  while ( sqlite3_step (vm) != SQLITE_DONE )
        {
    path = automapper_path_new( 
                        sqlite3_column_text (vm, 0),
                        sqlite3_column_int (vm, 3),
                        sqlite3_column_int (vm, 4)
      //vals[0], atoi(vals[3]), atoi(vals[4])
    );
    node = automapper_atlas_get_node( 
                        atlas,
                        sqlite3_column_int (vm, 1),
                        sqlite3_column_int (vm, 2)
      //atlas, atoi(vals[1]), atoi(vals[2])
    );
    if ( !node ) {
      g_critical( "corrupted atlas in database\n");
      automapper_atlas_delete( atlas );
      return NULL;
    }
    node->paths_out = g_list_append( node->paths_out, path );
    path = automapper_path_new( 
                        sqlite3_column_text (vm, 0),
                        sqlite3_column_int (vm, 1),
                        sqlite3_column_int (vm, 2)
      //vals[0], atoi(vals[1]), atoi(vals[2])
    );
    node = automapper_atlas_get_node( 
                        atlas,
                        sqlite3_column_int (vm, 3),
                        sqlite3_column_int (vm, 4)
      //atlas, atoi(vals[3]), atoi(vals[4])
    );
    if ( !node ) {
      g_critical( "corrupted atlas in database\n");
      automapper_atlas_delete( atlas );
      return NULL;
    }
    node->paths_in = g_list_append( node->paths_in, path );
  }

  
  sqlite3_close( db );

  atlas->cmap = g_hash_table_lookup( 
      atlas->maps_id, GINT_TO_POINTER( cmap_id )
  );
  if ( !atlas->cmap ) {
    g_critical( "corrupted atlas in database\n");
    automapper_atlas_delete( atlas );
    return NULL;
  }
  
  return atlas;
}

/******************** end internal structures functions ********************/


/******************** callback functions ********************/


static void on_entry_node_name_activate( GtkWidget *entry, gpointer user_data ){
  GtkWidget *automapper_c;
  ATLAS *atlas;
  automapper_c = gtk_widget_get_ancestor( entry, GTK_TYPE_VBOX );
  atlas = g_object_get_data( G_OBJECT(automapper_c), "atlas");
  automapper_node_set_name( 
    atlas->cmap->cnode , gtk_entry_get_text( GTK_ENTRY(entry) )
  );
}
static void on_entry_map_name_activate( GtkWidget *entry, gpointer user_data ){
  GtkWidget * automapper_c;
  ATLAS *atlas;
  automapper_c = gtk_widget_get_ancestor( entry, GTK_TYPE_VBOX );
  atlas = g_object_get_data( G_OBJECT(automapper_c), "atlas");
  automapper_map_set_name( 
    atlas->cmap , gtk_entry_get_text( GTK_ENTRY(entry) )
  );
}


static void automapper_update_names( GtkWidget *automapper_c) {
  GtkWidget *entry_map_name, *entry_node_name;
  ATLAS *atlas;
  if ( !automapper_c ) return;
  atlas = g_object_get_data( G_OBJECT(automapper_c), "atlas");
  if ( !atlas ) return;
  
  entry_node_name = g_object_get_data( 
    G_OBJECT(automapper_c), "entry_node_name" 
  );
  if ( ( entry_node_name ) && ( atlas->cmap->cnode->name ) )
    gtk_entry_set_text( 
      GTK_ENTRY(entry_node_name), atlas->cmap->cnode->name 
    ); 

  entry_map_name = g_object_get_data( 
    G_OBJECT(automapper_c), "entry_map_name" 
  );
  if ( ( entry_map_name ) && ( atlas->cmap->name ) ) 
    gtk_entry_set_text( GTK_ENTRY(entry_map_name), atlas->cmap->name ); 
}

static void automapper_map_fit( GtkWidget *map, ATLAS *atlas ) {
  gint w,h;
  gint neww, newh;
  gint x0,y0;
  gint lat, len;
  gint minx, miny;
  gint maxx, maxy;
  if ((!map) || (!atlas)) return;
  g_print(">>> enter in automapper_map_fit\n");
  w = map->allocation.width; h = map->allocation.height;
  //g_print("w=%d h=%d\n", w, h );

  minx = atlas->cmap->minx; miny = atlas->cmap->miny;
  maxx = atlas->cmap->maxx; maxy = atlas->cmap->maxy;
  lat = atlas->lat * atlas->zoom ; len = atlas->len*atlas->zoom; 
  //g_print("minx= %d, miny=%d, max=%d, maxy=%d\n", minx, miny, maxx, maxy );
  
  // calculate the needs 
  neww = ( maxx-minx + 1) * ( lat + len );
  newh = ( maxy-miny + 1 ) * ( lat + len );

  gtk_widget_set_size_request ( map, neww, newh );
  x0 = ( 0 - minx ) * ( lat + len ) + (len + lat) /2 ;
  y0 = ( 0 - miny ) * ( lat + len ) + (len + lat) /2 ; 
  w = map->allocation.width; h = map->allocation.height;
  //g_print("w=%d h=%d\n", w, h );
  if ( w > neww) x0 += (w-neww)/2;
  if ( h > newh) y0 += (h-newh)/2;
  
  g_object_set_data( G_OBJECT( map ), "x0", GINT_TO_POINTER( x0 ));
  g_object_set_data( G_OBJECT( map ), "y0", GINT_TO_POINTER( y0 ));

}
static void automapper_map_draw( GtkWidget *map, ATLAS *atlas ) {
  gint w,h;
  gint x0,y0;
  GList *list;
  NODE *node;
  gint lat, len;
  gint minx, miny;
  gint maxx, maxy;
  gint dx, dy;
  gint i;
  // some initializations for code clean
  // g_print(">>> enter in automapper_map_draw\n");
  w = map->allocation.width; h = map->allocation.height;
  //g_print("w=%d h=%d\n", w, h );

  minx = atlas->cmap->minx; miny = atlas->cmap->miny;
  maxx = atlas->cmap->maxx; maxy = atlas->cmap->maxy;
  lat = atlas->lat * atlas->zoom ; len = atlas->len * atlas->zoom; 

  x0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( map ), "x0" ));
  y0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( map ), "y0" ));
  if ((!x0) && (!y0)) {
    g_print(">>> call fit ; there is no x0 y0 \n");
    automapper_map_fit( map, atlas );
    x0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( map ), "x0" ));
    y0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( map ), "y0" ));
  }
  
  w = map->allocation.width; h = map->allocation.height;
  if ((x0 + minx*(lat+len) < 0 ) || (y0 + miny*(lat+len) < 0 ) ||
    (x0 + maxx*(lat+len) > w ) || (y0 + maxy*(lat+len) > h )) {
    g_print(">>> call fit ; there is no space \n");
    automapper_map_fit( map, atlas );
  }
  x0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( map ), "x0" ));
  y0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( map ), "y0" ));

  list = atlas->cmap->nodes;
  while ( list ) {
    node = (NODE*)list->data;
    for ( i = 0 ; i < 8 ; i++ )
      if ( node->links_out[ i ] == atlas->cmap->cnode->id ) break;
    if ( i < 8 ) { // don't hightlight the last node
      gdk_draw_rectangle ( 
        map->window, map->style->bg_gc[GTK_WIDGET_STATE (map)], TRUE, 
        x0 + node->position.x*( lat + len ) - lat/2 ,
        y0 + node->position.y*( lat + len ) - lat/2 , 
        lat, lat 
      );
    }
    //g_print("node %d : ", node->id );
    for ( i = 0 ; i < 8 ; i++ ) 
      if ( node->links_out[ i ] != -1 ) {
        automapper_get_delta( i, &dx, &dy );
        gdk_draw_line ( 
          map->window, map->style->fg_gc[GTK_WIDGET_STATE (map)] ,
          x0 + node->position.x*( lat + len )+dx*lat/2,
          y0 + node->position.y*( lat + len )+dy*lat/2, 
          x0 + (dx + node->position.x)*( lat + len )-dx*lat/2,
          y0 + (dy + node->position.y)*( lat + len )-dy*lat/2 
        );
      }
    gdk_draw_rectangle ( 
      map->window, map->style->fg_gc[GTK_WIDGET_STATE (map)], FALSE, 
      x0 + node->position.x*( lat + len ) - lat/2 ,
      y0 + node->position.y*( lat + len ) - lat/2 , 
      lat, lat
    );
    list = g_list_next( list );
  }
  gdk_draw_rectangle ( 
    map->window, map->style->fg_gc[GTK_WIDGET_STATE (map)], 
    TRUE, 
    x0 + atlas->cmap->cnode->position.x*( lat + len ) - lat/2 ,
    y0 + atlas->cmap->cnode->position.y*( lat + len ) - lat/2 ,
    lat, lat
  );

  //g_object_set_data( G_OBJECT( map ), "x0", GINT_TO_POINTER( x0 ));
  //g_object_set_data( G_OBJECT( map ), "y0", GINT_TO_POINTER( y0 ));

}

static void automapper_map_check_adjust( GtkWidget *wid, ATLAS *atlas, gboolean on ) {
  // now scroll if there is the case
  GtkWidget *sw;            // scrolled window widget
  GtkWidget *view;          // the view
  GtkAdjustment *vadj, *hadj;     // vertical, horizontal adjustment
  gint x0, y0;
  gint lat, len;
  gint cx, cy; 
  gint wview, hview;          // the width and hight for view 
  gdouble vadj_value, hadj_value;
  gint w,h;
  g_print(">>> enter in automapper_map_check_adjust\n");
  
  w = wid->allocation.width; h = wid->allocation.height;
  //g_print("w=%d h=%d\n", w, h );
  
  lat = atlas->lat * atlas->zoom ; len = atlas->len * atlas->zoom;

  view = gtk_widget_get_ancestor( wid, GTK_TYPE_VIEWPORT );
  wview = GTK_WIDGET(view)->allocation.width;
  hview = GTK_WIDGET(view)->allocation.height;

  sw = gtk_widget_get_ancestor( wid, GTK_TYPE_SCROLLED_WINDOW );
  vadj = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW(sw));
  vadj_value = gtk_adjustment_get_value( vadj );
  hadj = gtk_scrolled_window_get_hadjustment( GTK_SCROLLED_WINDOW(sw));
  hadj_value = gtk_adjustment_get_value( hadj );

  x0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( wid ), "x0" ));
  y0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( wid ), "y0" ));

  // x,y for current node
  cx = x0 + atlas->cmap->cnode->position.x * ( lat + len ) ;
  cy = y0 + atlas->cmap->cnode->position.y * ( lat + len ) ;

  if ( on || ( cx < hadj_value ) || ( cx > hadj_value + wview - lat )) {
    gtk_adjustment_set_value( hadj, (cx-wview/2 <= 0) ? 0 : (cx-wview/2) );
    gtk_adjustment_value_changed( hadj );
  }
  if ( on || ( cy < vadj_value ) || ( cy > vadj_value + hview - lat )) {
    gtk_adjustment_set_value( vadj, (cy-hview/2 <= 0) ? 0 : (cy-hview/2) );
    gtk_adjustment_value_changed( vadj );
  }

}


static void on_direction_button_clicked(GtkButton *button, gpointer user_data){
  GtkWidget *automapper_c;
  ATLAS *atlas = NULL;
  GtkWidget *da = NULL; // drawing area where the map is drew 
  guchar direction=-1;
  // get automapper container
  automapper_c = gtk_widget_get_ancestor( 
    GTK_WIDGET( button ), GTK_TYPE_VBOX 
  );
  atlas = g_object_get_data( G_OBJECT(automapper_c), "atlas");
  if (!atlas) {
    g_warning( "atlas for this this session NOT found.");
    return;
  }
  da = g_object_get_data( G_OBJECT(automapper_c), "map");
  if (!da) {
    g_warning( "drawing area for this this session NOT found.");
    return;
  }
  if ( !strcmp( GTK_WIDGET(button)->name , "button_n") )  direction = 0; 
  if ( !strcmp( GTK_WIDGET(button)->name , "button_ne") ) direction = 1; 
  if ( !strcmp( GTK_WIDGET(button)->name , "button_e") )  direction = 2; 
  if ( !strcmp( GTK_WIDGET(button)->name , "button_se") ) direction = 3; 
  if ( !strcmp( GTK_WIDGET(button)->name , "button_s") )  direction = 4; 
  if ( !strcmp( GTK_WIDGET(button)->name , "button_sw") ) direction = 5; 
  if ( !strcmp( GTK_WIDGET(button)->name , "button_w") )  direction = 6; 
  if ( !strcmp( GTK_WIDGET(button)->name , "button_nw") ) direction = 7; 
  automapper_map_move( atlas->cmap, direction, TRUE );
  automapper_map_draw( da, atlas );
  automapper_update_names( automapper_c );
  automapper_map_check_adjust( da, atlas, FALSE );

}

static void on_remove_node_clicked(GtkButton *button, gpointer user_data){
  GtkWidget *automapper_c;
  ATLAS *atlas = NULL;
  GtkWidget *da = NULL; // drawing area where the map is drew 
  // get automapper container
  automapper_c = gtk_widget_get_ancestor( 
    GTK_WIDGET( button ), GTK_TYPE_VBOX 
  );
  atlas = g_object_get_data( G_OBJECT(automapper_c), "atlas");
  if (!atlas) {
    g_warning( "atlas for this this session NOT found.");
    return;
  }
  da = g_object_get_data( G_OBJECT(automapper_c), "map");
  if (!da) {
    g_warning( "drawing area for this this session NOT found.");
    return;
  }
  automapper_map_remove_node( atlas->cmap, atlas->cmap->cnode );
  gtk_widget_queue_draw( da );
  automapper_update_names( automapper_c );
  automapper_map_check_adjust( da, atlas, TRUE );
}


gboolean expose_event_callback (GtkWidget *map, GdkEventExpose *event, gpointer data) {
  GtkWidget *automapper_c;
  g_print(">>> enter in expose_event_callback\n");
  automapper_map_draw( map, data );
  automapper_c = gtk_widget_get_ancestor( map, GTK_TYPE_VBOX );
  automapper_update_names( automapper_c );
  return TRUE;
}

static void internal_set_menu_sesitivity( GtkWidget *menu, gboolean on ) {
  GList *l;
  l = gtk_container_get_children( GTK_CONTAINER( menu ) );
  while ( l ) {
    gtk_widget_set_sensitive( GTK_WIDGET(l->data), on );
    l = g_list_next ( l );
  }
}


static void callback_menu_activated( GtkMenuItem *menuitem, gpointer user_data) { GtkWidget *automapper_c;
  GtkWidget *menu;
  ATLAS *atlas;
  SESSION_STATE* session;
  GList *l;
  gboolean on;
  menu = gtk_menu_item_get_submenu( menuitem );
  if (!menu) return;
  session = interface_get_active_session();
  if (!session) {
    mdebug (DBG_MODULE, 0, "No active session.");
    internal_set_menu_sesitivity( menu, FALSE );
    return;
  }
  automapper_c = g_hash_table_lookup( session->extra, "automapper_c" );
  if ( !automapper_c ) {
    g_warning( "there is no automapper_c register for current session." );
    internal_set_menu_sesitivity( menu, FALSE );
    return;
  }
  atlas = g_object_get_data( G_OBJECT(automapper_c), "atlas");
  if ( !atlas ) {
    g_warning( "there is no ATLAS attached to automapper container." );
    internal_set_menu_sesitivity( menu, FALSE );
    return;
  }
  internal_set_menu_sesitivity( menu, TRUE );
  l = gtk_container_get_children( GTK_CONTAINER( menu ) );
  
  on = GTK_WIDGET_VISIBLE( automapper_c);
  gchar *zoom = g_strdup_printf( "%d", (gint)(atlas->zoom*100) );
  while (l) {
    const gchar *name = gtk_widget_get_name( GTK_WIDGET(l->data) );
    if ( !strcmp( name, "Enable" ) ){
      gtk_check_menu_item_set_active( 
        GTK_CHECK_MENU_ITEM( l->data ), on 
      );
    }
    if ( g_str_has_prefix( name, "zoom" ) ) {
      GTK_CHECK_MENU_ITEM( l->data )->active = !strcmp( name+4, zoom );
    /*
      gtk_check_menu_item_set_active( 
        GTK_CHECK_MENU_ITEM( l->data ), !strcmp( name+4, zoom ) 
      );
      gtk_widget_set_sensitive( GTK_WIDGET(l->data), on );
      */
    }
    l = g_list_next ( l );
  }
  g_free( zoom );
}

static void callback_menu_enable_activated( GtkMenuItem *menuitem, gpointer data) {
  GtkWidget *automapper_c;
  SESSION_STATE* session;
  
  session = interface_get_active_session();
  if (!session) {
    g_warning( "there is no active session.");
    return;
  }
  automapper_c = g_hash_table_lookup( session->extra, "automapper_c" );
  if ( !automapper_c ) {
    g_warning( "there is no automapper_c register for current session." );
    return;
  }
  if ( GTK_CHECK_MENU_ITEM(menuitem)->active) {
    gtk_widget_show( automapper_c );    
  } else {
    gtk_widget_hide( automapper_c );    
  }
  
}

static void callback_menu_zoom_activated( GtkMenuItem *menuitem, gpointer data) {
  GtkWidget *automapper_c;
  SESSION_STATE* session;
  ATLAS *atlas;
  GtkWidget *da;
  
  session = interface_get_active_session();
  if (!session) {
    g_warning( "there is no active session.");
    return;
  }
  automapper_c = g_hash_table_lookup( session->extra, "automapper_c" );
  if ( !automapper_c ) {
    g_warning( "there is no automapper_c register for current session." );
    return;
  }
  atlas = g_object_get_data( G_OBJECT(automapper_c), "atlas");
  if ( !atlas ) {
    g_warning( "there is no ATLAS attached to automapper container." );
    return;
  }
  da = g_object_get_data( G_OBJECT(automapper_c), "map");
  if ( !da ) {
    g_warning( "there is no map attached to automapper container." );
    return;
  }
  const gchar *name = gtk_widget_get_name( GTK_WIDGET(menuitem) );
  atlas->zoom = (gdouble)atoi( name+4 )/100;
  g_print("zoom is %f\n", atlas->zoom );
  automapper_map_fit( da, atlas );
  automapper_map_draw( da, atlas );
  //gtk_widget_queue_draw( da );
  //automapper_map_check_adjust( da, atlas, TRUE );
  
}

// for a widget this function will returns associated elements 
static gboolean internal_automapper_elements( GtkWidget *base, GtkWidget **automapper_c, SESSION_STATE **session, ATLAS **atlas, GtkWidget **da) {
  GtkWidget *wid;
  if (!base)  return FALSE;
  wid = gtk_widget_get_ancestor( base , GTK_TYPE_VBOX );
  if ( !wid ) return FALSE;
  if ( automapper_c ) *automapper_c = wid;
  if ( session ) {
    *session = g_object_get_data( G_OBJECT( wid ), "session");
    if ( !(*session) ) return FALSE;
  }
  if ( atlas ) {
    *atlas = g_object_get_data( G_OBJECT( wid ), "atlas");
    if ( !(*atlas) ) return FALSE;
  }
  if ( da ) {
    *da = g_object_get_data( G_OBJECT( wid ), "map");
    if ( !(*da) ) return FALSE;
  }
  return TRUE;
}

// will follow the path cmd1 from current nod if exits
static void automapper_atlas_follow_path( ATLAS *atlas, gchar *cmd1, gchar *cmd2 ){
  GList *l;
  PATH *path;
  MAP *map;
  NODE *node;

  g_return_if_fail( atlas );
  g_return_if_fail( cmd1 );

  l = atlas->cmap->cnode->paths_out;
  while ( l ) { 
    path = (PATH*)l->data;
    if ( !strcmp( path->command, cmd1 ) ) { // we have a map  
      map = g_hash_table_lookup( 
        atlas->maps_id, GINT_TO_POINTER( path->map_id )
      );
      if (!map) { 
        g_warning("Path to an invalid map:%d !", path->map_id );
        return;
      }
      atlas->cmap = map;
      node = g_hash_table_lookup( 
        map->nodes_id, GINT_TO_POINTER( path->node_id)
      );
      if (!node) { 
        g_warning(
          "Path to an invalid node ... node:%d map%d", 
          path->map_id, path->node_id
        );
        return;
      }
      atlas->cmap->cnode = node;
      //automapper_map_draw( da, atlas );
      //gtk_widget_queue_draw( da );
      //automapper_map_check_adjust( da, atlas, TRUE );
      //atlas->cmap->cnode = path->node_id;
      return;
    }
    l = g_list_next ( l );
  }
  // if we get here that means there is no path with cmd1 command
  // ... so create one
  
  map = automapper_atlas_add_map( atlas );
  map->cnode = automapper_map_add_node( map, 0, 0 );
  
  // and link it with current node
  path = automapper_path_new( cmd1, map->id, map->cnode->id );
  atlas->cmap->cnode->paths_out = g_list_append( 
    atlas->cmap->cnode->paths_out, path 
  );
  path = automapper_path_new( 
    cmd1, atlas->cmap->id, atlas->cmap->cnode->id 
  );
  map->cnode->paths_in = g_list_append (
    map->cnode->paths_in, path 
  );

  if ( cmd2 ) { // the 
    path = automapper_path_new( 
      cmd2, atlas->cmap->id, atlas->cmap->cnode->id 
    );
    map->cnode->paths_out = g_list_append (
      map->cnode->paths_out, path 
    );
    path = automapper_path_new( cmd2, map->id, map->cnode->id );
    atlas->cmap->cnode->paths_in = g_list_append( 
      atlas->cmap->cnode->paths_in, path 
    );
  }
  atlas->cmap = map;
}

static void on_button_up_clicked( GtkButton * button, gpointer data ) {
  GtkWidget *da;
  ATLAS *atlas;
  gboolean ok;
  ok = internal_automapper_elements( 
    GTK_WIDGET(button), NULL, NULL, &atlas, &da 
  ); 
  if ( !ok ) {
    g_warning( "on_button_up_clicked : not elements found " );
    return;
  }
  
  // if we get there that means that there is nothig up ...
  // ... so create new map 
  automapper_atlas_follow_path( atlas, "up", "down" );

  automapper_map_draw( da, atlas );
  gtk_widget_queue_draw( da );
  automapper_map_check_adjust( da, atlas, TRUE );

}

static void on_button_down_clicked( GtkButton * button, gpointer data ) {
  GtkWidget *da;
  ATLAS *atlas;
  gboolean ok;
  ok = internal_automapper_elements( 
    GTK_WIDGET(button), NULL, NULL, &atlas, &da 
  ); 
  if ( !ok ) {
    g_warning( "on_button_up_clicked : not elements found " );
    return;
  }
  automapper_atlas_follow_path( atlas, "down", "up" );

  automapper_map_draw( da, atlas );
  gtk_widget_queue_draw( da );
  automapper_map_check_adjust( da, atlas, TRUE );

}

static void combo_command_change( GtkEntry *entry, gpointer data ) {
  GtkWidget *win;
  GtkWidget *combo2;
  GtkWidget *combo3;
  ATLAS *atlas;
  PATH *path;
  const gchar *text = gtk_entry_get_text( entry );
  
  if ( text[0] == '\0' ) return;
  win = gtk_widget_get_toplevel( GTK_WIDGET(entry) );
  combo2 = g_object_get_data( G_OBJECT( win ), "combo2" );
  if ( !combo2 ) return ;
  combo3 = g_object_get_data( G_OBJECT( win ), "combo3" );
  if ( !combo3 ) return ;

  atlas = g_object_get_data( G_OBJECT( win ), "atlas");
  if ( !atlas ) return;
  g_print(" >>>>>>>>>> command is <%s>\n", text );
  path = automapper_node_get_out_path_by_name( 
    atlas->cmap->cnode, (gchar*)text
  );
  if ( path ) {
    MAP *tmap = g_hash_table_lookup( 
        atlas->maps_id, GINT_TO_POINTER( path->map_id )
    );
    if (!tmap) { return; }
    if (tmap->name) {
      gtk_entry_set_text( 
        GTK_ENTRY(GTK_COMBO(combo2)->entry), tmap->name 
      );
    }
    NODE *tnode = g_hash_table_lookup( 
      tmap->nodes_id, GINT_TO_POINTER( path->node_id )
    );
    if (!tnode) { return; }
    if (tnode->name) {
      gtk_entry_set_text( 
        GTK_ENTRY(GTK_COMBO(combo3)->entry), tnode->name 
      );
    }
  } else {
    gtk_entry_set_text( GTK_ENTRY(GTK_COMBO(combo2)->entry), "new map" );
  }
}

static void combo_map_change( GtkEntry *entry, gpointer data ) {
  GtkWidget *win;
  GtkWidget *wid;
  ATLAS *atlas;
  GList *l1, *l2;
  MAP *map;
  
  const gchar *text = gtk_entry_get_text( entry );
  if ( text[0] == '\0' ) return;
  win = gtk_widget_get_toplevel( GTK_WIDGET(entry) );
  atlas = g_object_get_data( G_OBJECT( win ), "atlas");
  if ( !atlas ) return;
  wid = g_object_get_data( G_OBJECT( win ), "combo3" );
  if ( !wid ) return ;
  g_print(" >>>>>>>>>> map is <%s>\n", text );
  if ( !strcmp( text, "new map" ) ) {
    gtk_widget_set_sensitive( wid, FALSE );
    return;
  } 
  gtk_widget_set_sensitive( wid, TRUE );
  map = automapper_atlas_get_map_by_name ( atlas, (gchar*)text );
  if ( !map ) return;
  l1 = map->nodes; l2= NULL;
  while ( l1 ) {
    l2 = g_list_append( l2, ((NODE*)l1->data)->name );
    l1 = g_list_next( l1 );
  }
  gtk_combo_set_popdown_strings( GTK_COMBO( wid ), l2 );
  g_list_free( l2 );
}

static void on_button_add_path_ok_clicked( GtkButton *button, gpointer data ) {
  GtkWidget *win;
  GtkWidget *wid;
  GtkWidget *da;
  ATLAS *atlas;
  MAP *map;
  NODE *node;
  PATH *path;
  const gchar *command;
  const gchar *map_name;
  const gchar *node_name;
  win = gtk_widget_get_toplevel( GTK_WIDGET(button) );

  atlas = g_object_get_data( G_OBJECT( win ), "atlas");
  if ( !atlas ) return;
  
  wid = g_object_get_data( G_OBJECT(win), "combo1" );
  if ( !wid ) return ;
  command = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(wid)->entry) );
  if ( command[0]=='\0' ) {
    interface_display_message("Command can NOT be NULL !");
    return;
  }
  
  wid = g_object_get_data( G_OBJECT(win), "combo2" );
  if ( !wid ) return ;
  map_name = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(wid)->entry) );

  wid = g_object_get_data( G_OBJECT(win), "combo3" );
  if ( !wid ) return ;
  node_name = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(wid)->entry) );

  da = g_object_get_data( G_OBJECT(win), "map");
  if ( !da ) return;

  // get path list
  path = automapper_node_get_out_path_by_name( 
    atlas->cmap->cnode, (gchar*)command 
  );
  if ( !strcmp( map_name, "new map" ) ) { // create new one
    map = automapper_atlas_add_map( atlas );
    map->cnode = automapper_map_add_node( map, 0, 0 );
  } else {
    map = automapper_atlas_get_map_by_name( atlas, (gchar*)map_name );
    if ( !map ) return;
    node = automapper_map_get_node_by_name( map, (gchar*)node_name );
    if ( !node ) return;
    map->cnode = node;
  }
  
  
  if ( !path  ) { // add a path to current node
    path = automapper_path_new( (gchar*)command, map->id, map->cnode->id );
    atlas->cmap->cnode->paths_out = g_list_append ( 
      atlas->cmap->cnode->paths_out, path 
    );
    path = automapper_path_new( 
      (gchar*)command, atlas->cmap->id, atlas->cmap->cnode->id 
    );
    map->cnode->paths_in = g_list_append(
      map->cnode->paths_in, path 
    );
  } else { 
    // do some cleanup
    MAP *tmap = g_hash_table_lookup( 
        atlas->maps_id, GINT_TO_POINTER( path->map_id )
    );
    if (!tmap) { return; }
    NODE *tnode = g_hash_table_lookup( 
      tmap->nodes_id, GINT_TO_POINTER( path->node_id )
    );
    if (!tnode) { return; }
    tnode->paths_in = g_list_remove( tnode->paths_in, path );
    GList *l2 = tnode->paths_in;
    PATH *path2;
    while ( l2 ) {
      path2 = (PATH*)l2->data;
      if ((path2->node_id == atlas->cmap->cnode->id ) &&
        (path2->map_id == atlas->cmap->id) &&
        !strcmp( command, path2->command )) {
        tnode->paths_in = g_list_remove( tnode->paths_in, path2 );
        break;
      }
      l2 = g_list_next( l2 );
    }
    // update path
    path->map_id = map->id;
    path->node_id = map->cnode->id;
    g_print("PATH UPDATE %d %d\n", map->id, map->cnode->id );

    path = automapper_path_new( 
      (gchar*)command, atlas->cmap->id, atlas->cmap->cnode->id 
    );
    map->cnode->paths_in = g_list_append( 
      map->cnode->paths_in, path
    );
  
  } 
  atlas->cmap = map;
  automapper_map_fit( da, atlas );
  automapper_map_draw( da, atlas );
  gtk_widget_destroy( win );
  
}

static void on_button_path_add_clicked( GtkButton *button, gpointer data ) {
  GladeXML *xml;
  GtkWidget *wid, *win, *wid_c;
  GtkWidget *da;
  ATLAS *atlas;
  gchar *ui_file;
  GList *l1, *l2;
  gchar s[] = "new map";
  gboolean ok;

  ok = internal_automapper_elements( 
    GTK_WIDGET(button), NULL, NULL, &atlas, &da 
  ); 
  if ( !ok ) {
    g_warning( "on_button_path_add_clicked: not elements found " );
    return;
  }
  
  ui_file = g_build_filename( 
    mudmagic_data_directory(), "interface", "automapper.glade", NULL 
  );
  xml = glade_xml_new( ui_file, "window_add_path", NULL);
  win = glade_xml_get_widget( xml, "window_add_path" );
  g_object_set_data( G_OBJECT( win ), "atlas", atlas );
  g_object_set_data( G_OBJECT( win ), "map", da );
  
  // *** command combo
  wid_c = glade_xml_get_widget( xml, "combo1_c" );
  wid = gtk_combo_new( );
  gtk_widget_show( wid );
  gtk_widget_grab_focus( GTK_COMBO(wid)->entry );
  gtk_container_add( GTK_CONTAINER(wid_c), wid );
  g_object_set_data( G_OBJECT( win ), "combo1", wid );
  gtk_combo_disable_activate( GTK_COMBO(wid) );
  g_signal_connect( 
    G_OBJECT( GTK_COMBO(wid)->entry ), "changed", 
    G_CALLBACK(combo_command_change), NULL 
  );

  l1 = atlas->cmap->cnode->paths_out; l2 = NULL;
  while ( l1 ) {
    l2 = g_list_append( l2, ((PATH*)l1->data)->command );
    l1 = g_list_next ( l1 );
  }
  if ( l2 ) gtk_combo_set_popdown_strings( GTK_COMBO( wid ), l2 );
  g_list_free( l2 );
  gtk_entry_set_text( GTK_ENTRY(GTK_COMBO(wid)->entry), "" );
  //gtk_entry_select_region( GTK_ENTRY(GTK_COMBO(wid)->entry), 0, -1 );
  
  
  // *** maps combo
  wid_c = glade_xml_get_widget( xml, "combo2_c" );
  wid = gtk_combo_new();
  gtk_widget_show( wid );
  gtk_container_add( GTK_CONTAINER(wid_c), wid );
  g_object_set_data( G_OBJECT( win ), "combo2", wid );
  gtk_combo_disable_activate( GTK_COMBO(wid) );
  // set map list
  l1 = atlas->maps; l2 = NULL;
  l2 = g_list_append( l2, s );
  while ( l1 ) {
    l2 = g_list_append( l2, ((MAP*)l1->data)->name );
    l1 = g_list_next ( l1 );
  }
  if ( l2 ) gtk_combo_set_popdown_strings( GTK_COMBO( wid ), l2 );
  g_list_free( l2 );
  gtk_combo_set_value_in_list( GTK_COMBO( wid ), TRUE, FALSE ); 
  g_signal_connect( 
    G_OBJECT( GTK_COMBO(wid)->entry ), "changed", 
    G_CALLBACK(combo_map_change), NULL 
  );
  
  // *** room combo
  wid_c = glade_xml_get_widget( xml, "combo3_c" );
  wid = gtk_combo_new();
  gtk_widget_show( wid );
  gtk_widget_set_sensitive( wid, FALSE );
  gtk_container_add( GTK_CONTAINER(wid_c), wid );
  g_object_set_data( G_OBJECT( win ), "combo3", wid );
  gtk_combo_disable_activate( GTK_COMBO(wid) );
  g_free( ui_file );

  // *** cancel button
  wid = glade_xml_get_widget( xml, "add_path_cancel" );
  g_signal_connect_swapped( 
    G_OBJECT(wid), "clicked", G_CALLBACK(gtk_widget_destroy), win
  );
  // *** ok button
  wid = glade_xml_get_widget( xml, "add_path_ok" );
  g_signal_connect( 
    G_OBJECT(wid), "clicked", 
    G_CALLBACK(on_button_add_path_ok_clicked), NULL 
  );
  g_object_unref( G_OBJECT(xml) );
}

static void automapper_node_remove_path_out( NODE *node, const gchar *command ){
  GList *l, *l2;
  NODE *dnode;
  PATH *path;
  PATH *path2;
  g_return_if_fail( node && command );
  
  l = node->paths_out;
  while ( l ) {
    path = (PATH*)l->data;  
    if (!strcmp( command, path->command)) {
      // get destination node
      dnode = automapper_atlas_get_node( 
        node->map->atlas, path->map_id, path->node_id
      );
      if ( dnode ) {
        l2 = dnode->paths_in;
        while ( l2 ) {
          path2 = (PATH*)l2->data;  
          if ((path2->map_id  == node->map->id) &&
            (path2->node_id == node->id ) &&
            (!strcmp(path2->command, command) )){
            dnode->paths_in = g_list_remove(
              dnode->paths_in, path2
            );
            automapper_path_delete( path2 );
            break;
          }
          l2 = g_list_next( l2 );
        }
      }
      // remove path from list
      node->paths_out = g_list_remove( node->paths_out, path );
      // and delete path
      automapper_path_delete( path );
      break;
    }
    l = g_list_next ( l );
  }
}

static void on_button_remove_path_clicked( GtkButton *button, gpointer data ) {
  GtkWidget *win, *wid;
  ATLAS *atlas;
  win = gtk_widget_get_toplevel( GTK_WIDGET(button) );

  atlas = g_object_get_data( G_OBJECT( win ), "atlas");
  g_return_if_fail( atlas );
  
  wid = g_object_get_data( G_OBJECT(win), "combo" );
  g_return_if_fail( wid );
  debug_atlas_dump( atlas );
  automapper_node_remove_path_out( 
    atlas->cmap->cnode, 
    gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(wid)->entry))
  );

  gtk_widget_destroy( win );
}

static void on_button_path_remove_clicked( GtkButton *button, gpointer data ) {
  GtkWidget *wid, *win, *wid_c;
  gboolean ok;
  gchar *ui_file;
  GtkWidget *da;
  ATLAS *atlas;
  GladeXML *xml;
  GList *l1, *l2;
  ok = internal_automapper_elements( 
    GTK_WIDGET(button), NULL, NULL, &atlas, &da 
  ); 
  if ( !ok ) {
    g_warning( "on_button_path_remove_clicked: not elements found " );
    return;
  }
  ui_file = g_build_filename( 
    mudmagic_data_directory(), "interface", "automapper.glade", NULL 
  );
  xml = glade_xml_new( ui_file, "window_remove_path", NULL);
  win = glade_xml_get_widget( xml, "window_remove_path" );
  g_object_set_data( G_OBJECT( win ), "atlas", atlas );
  g_object_set_data( G_OBJECT( win ), "map", da );

  wid_c = glade_xml_get_widget( xml, "combo_c" );
  wid = gtk_combo_new();
  gtk_widget_show( wid );
  gtk_container_add( GTK_CONTAINER(wid_c), wid );
  g_object_set_data( G_OBJECT( win ), "combo", wid );
  gtk_combo_disable_activate( GTK_COMBO(wid) );
  gtk_combo_set_value_in_list( GTK_COMBO( wid ), TRUE, FALSE ); 
  
  l1 = atlas->cmap->cnode->paths_out; l2 = NULL;
  while ( l1 ) {
    l2 = g_list_append( l2, ((PATH*)l1->data)->command );
    l1 = g_list_next ( l1 );
  }
  if ( l2 ) gtk_combo_set_popdown_strings( GTK_COMBO( wid ), l2 );
  g_list_free( l2 );

  // *** cancel button
  wid = glade_xml_get_widget( xml, "remove_path_cancel" );
  g_signal_connect_swapped( 
    G_OBJECT(wid), "clicked", G_CALLBACK(gtk_widget_destroy), win
  );
  // *** ok button
  wid = glade_xml_get_widget( xml, "remove_path_remove" );
  g_signal_connect( 
    G_OBJECT(wid), "clicked", 
    G_CALLBACK(on_button_remove_path_clicked), NULL 
  );
  g_object_unref( G_OBJECT(xml) );
  
}

static gboolean on_mouse_button_pressed( GtkWidget *widget, GdkEventButton *event, gpointer user_data ){
  GtkWidget *da;
  ATLAS *atlas;
  POSITION pos;
  NODE *node;
  gboolean ok;
  gint x0,y0,lat,len,x,y,sgn;
  ok = internal_automapper_elements( widget, NULL, NULL, &atlas, &da ); 
  if (!ok) return FALSE;
  lat = atlas->lat * atlas->zoom ; len = atlas->len*atlas->zoom; 
  x0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( da ), "x0" ));
  y0 = GPOINTER_TO_INT(g_object_get_data( G_OBJECT( da ), "y0" ));
  x = (gint)event->x; y = (gint)event->y;
  sgn = (( x - x0 ) > 0 ) ? 1 : -1 ;
  pos.x = (x-x0 + sgn * lat/2)/(lat+len);
  sgn = (( y - y0 ) > 0 ) ? 1 : -1 ;
  pos.y = (y-y0 + sgn * lat/2)/(lat+len);
  //g_print( "x=%d, y=%d x0=%d y0=%d xx=%d yy=%d\n", (gint)event->x, (gint)event->y , x0, y0, pos.x, pos.y );
  node = g_hash_table_lookup( atlas->cmap->nodes_pos, &pos );
  if (node)  {
    atlas->cmap->cnode = node;
    automapper_map_draw( da, atlas );
    gtk_widget_queue_draw( da );
  }
  return FALSE;
}

/******************** end callback functions ********************/



/******************** module entries ********************/ 

 void module_automapper_load() {
  mdebug (DBG_MODULE, 0, "Module: Automapper loaded.");
}


 void module_automapper_unload(void ) {
  mdebug (DBG_MODULE, 0, "Module: Automapper unloaded.");
}

 void module_automapper_menu_modify ( GtkWidget *menubar ) {
  GtkWidget *menu;
  GtkWidget *item;
  GtkWidget *item2;
  GSList *group;
  gint i;
  gchar *label = NULL;
  item = gtk_menu_item_new_with_mnemonic( "Auto_map" );
  gtk_widget_set_name( item, "automapper_menu");
  gtk_widget_show( item );
  gtk_container_add( GTK_CONTAINER (menubar), item );
  g_signal_connect (
    (gpointer) item, "activate", G_CALLBACK(callback_menu_activated), NULL
  );

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  
  item2 = gtk_check_menu_item_new_with_mnemonic( "Enable" );
  gtk_widget_set_name( item2, "Enable" );
  gtk_widget_show( item2 );
  gtk_container_add( GTK_CONTAINER(menu), item2 );
  
  g_signal_connect_after (
    (gpointer) item2, "activate", 
    G_CALLBACK (callback_menu_enable_activated), NULL
  );
  
  item2 = gtk_separator_menu_item_new( );
  gtk_widget_show( item2 );
  gtk_container_add( GTK_CONTAINER(menu), item2 );
  
  group = NULL;
  for ( i = 0 ; i < 4 ; i++ ) {
    label = g_strdup_printf( "_%d zoom %d%%", i+1, (i+1)*50);
    item2 = gtk_radio_menu_item_new_with_mnemonic( group, label );
    g_free( label );
    
    label = g_strdup_printf( "zoom%d", (i+1)*50);
    gtk_widget_set_name( item2, label );
    g_free(label);
    
    gtk_widget_show( item2 );
    gtk_container_add( GTK_CONTAINER(menu), item2 );
    g_signal_connect_after(
      (gpointer) item2, "activate", 
      G_CALLBACK (callback_menu_zoom_activated), NULL
    );
    group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM( item2 ) );
  }
  
}


 void module_automapper_menu_reset ( GtkWidget *menubar ) {
  GList *l;
  const gchar *name;
  l = gtk_container_get_children( GTK_CONTAINER( menubar ) );
  while ( l ) {
    name = gtk_widget_get_name( GTK_WIDGET(l->data) );
    if (!strcmp( name, "automapper_menu")){
      gtk_widget_destroy( GTK_WIDGET(l->data ) );
    }
    l = g_list_next( l );
  }
}


 void module_automapper_session_open( SESSION_STATE *session ) {
  GladeXML *xml;
  GtkWidget *area_right;
  GtkWidget *automapper_c;
  GtkWidget *wid;
  ATLAS *atlas;
  gchar *ui_file;
  GdkColor color;
  
  ui_file = g_build_filename( 
    mudmagic_data_directory(), "interface", "automapper.glade", NULL 
  );
  xml = glade_xml_new( ui_file, "automapper_c", NULL);
  g_free( ui_file );
  automapper_c = glade_xml_get_widget( xml, "automapper_c" );
  glade_xml_signal_connect( 
    xml, "on_direction_button_clicked", 
    G_CALLBACK( on_direction_button_clicked )
  );
  glade_xml_signal_connect( 
    xml, "on_remove_node_clicked", 
    G_CALLBACK( on_remove_node_clicked )
  );
  glade_xml_signal_connect( 
    xml, "on_entry_node_name_activate", 
    G_CALLBACK( on_entry_node_name_activate )
  );
  glade_xml_signal_connect( 
    xml, "on_entry_map_name_activate", 
    G_CALLBACK( on_entry_map_name_activate )
  );
  glade_xml_signal_connect( 
    xml, "on_button_up_clicked", 
    G_CALLBACK( on_button_up_clicked )
  );
  glade_xml_signal_connect( 
    xml, "on_button_down_clicked", 
    G_CALLBACK( on_button_down_clicked )
  );
  glade_xml_signal_connect( 
    xml, "on_button_path_add_clicked", 
    G_CALLBACK( on_button_path_add_clicked )
  );
  glade_xml_signal_connect( 
    xml, "on_button_path_remove_clicked", 
    G_CALLBACK( on_button_path_remove_clicked )
  );
  glade_xml_signal_connect( 
    xml, "on_mouse_button_pressed",
    G_CALLBACK( on_mouse_button_pressed )
  );

  // register automap to session
  g_hash_table_insert( session->extra, "automapper_c", automapper_c );
  
  area_right = interface_tab_get_area_right( session->tab );
  gtk_container_add( GTK_CONTAINER( area_right ), automapper_c );
  wid =  gtk_widget_get_ancestor( area_right, GTK_TYPE_SCROLLED_WINDOW );
  gtk_widget_show( wid );
  
  wid = glade_xml_get_widget( xml, "map" );
  gdk_color_parse( "black", &color);
  gtk_widget_modify_bg( wid, GTK_STATE_NORMAL, &color );
  gdk_color_parse( "white", &color);
  gtk_widget_modify_fg( wid, GTK_STATE_NORMAL, &color );
  gtk_widget_set_size_request (wid, 100, 100);


  atlas = automapper_atlas_load( session->slot ); // try to load atlas
  debug_atlas_dump( atlas );
  if (!atlas ) {
    atlas = automapper_atlas_new();
    atlas->cmap = automapper_atlas_add_map( atlas );
    atlas->cmap->cnode = automapper_map_add_node( atlas->cmap, 0, 0 );
  }
    

  g_signal_connect (
    G_OBJECT (wid), "expose_event", 
    G_CALLBACK (expose_event_callback), atlas 
  );

  // attach some data to automapper_c for easy acces
  g_object_set_data( G_OBJECT(automapper_c), "atlas", atlas ); 
  g_object_set_data( G_OBJECT(automapper_c), "map", wid ); 
  g_object_set_data( G_OBJECT(automapper_c), "session", session ); 

  wid = glade_xml_get_widget( xml, "entry_map_name" );
  g_object_set_data( G_OBJECT(automapper_c), "entry_map_name", wid ); 
  wid = glade_xml_get_widget( xml, "entry_node_name" );
  g_object_set_data( G_OBJECT(automapper_c), "entry_node_name", wid ); 
  
  g_object_unref( G_OBJECT(xml) );
}

 void module_automapper_session_close( SESSION_STATE *session ) {
  GtkWidget *wid;
  ATLAS *atlas;

  g_print("session close begin \n");
  wid = GTK_WIDGET( g_hash_table_lookup( session->extra, "automapper_c" ) );
  g_hash_table_remove( session->extra, "automapper_c" );
  if ( !wid ) {
    g_warning( "automapper container NOT found." );
    return;
  }
  atlas = g_object_get_data( G_OBJECT(wid), "atlas");
  if ( !atlas ) {
    g_warning( "atlas NOT found." );
    return;
  }
  g_print(" before delete atlas !\n");
  automapper_atlas_save( atlas, session->slot );
  debug_atlas_dump( atlas );
  automapper_atlas_delete( atlas );
  g_print(" after delete atlas !\n");
  gtk_widget_destroy( wid );
  
  g_print("session close end \n");
}

 void module_automapper_data_out( SESSION_STATE *session, gchar **data, gsize *size) {
  GtkWidget *wid;
  GtkWidget *da;
  ATLAS *atlas;
  g_return_if_fail( session && data && *data && size );
  wid = GTK_WIDGET( g_hash_table_lookup( session->extra, "automapper_c" ) );
  g_return_if_fail( wid );
  atlas = g_object_get_data( G_OBJECT(wid), "atlas");
  da = g_object_get_data( G_OBJECT(wid), "map");
  g_return_if_fail( atlas && da );
  if ( 0 ) return;  // FIXME check if automapper_c is not activated 
  if ( !strcmp( *data, "n" ) || !strcmp( *data, "north" )) {
    automapper_map_move( atlas->cmap, AM_N, TRUE );
  }
  if ( !strcmp( *data, "ne" ) || !strcmp( *data, "northeast" )) {
    automapper_map_move( atlas->cmap, AM_NE, TRUE );
  }
  if ( !strcmp( *data, "e" ) || !strcmp( *data, "east" )) {
    automapper_map_move( atlas->cmap, AM_E, TRUE );
  }
  if ( !strcmp( *data, "se" ) || !strcmp( *data, "southeast" )) {
    automapper_map_move( atlas->cmap, AM_SE, TRUE );
  }
  if ( !strcmp( *data, "s" ) || !strcmp( *data, "south" )) {
    automapper_map_move( atlas->cmap, AM_S, TRUE );
  }
  if ( !strcmp( *data, "sw" ) || !strcmp( *data, "southwest" )) {
    automapper_map_move( atlas->cmap, AM_SW, TRUE );
  }
  if ( !strcmp( *data, "w" ) || !strcmp( *data, "west" )) {
    automapper_map_move( atlas->cmap, AM_W, TRUE );
  }
  if ( !strcmp( *data, "nw" ) || !strcmp( *data, "northwest" )) {
    automapper_map_move( atlas->cmap, AM_NW, TRUE );
  }
  if ( !strcmp( *data, "up" ) ) {
    automapper_atlas_follow_path( atlas, "up", "down" );
  }
  if ( !strcmp( *data, "down" ) ) {
    automapper_atlas_follow_path( atlas, "down", "up" );
  }
  automapper_map_draw( da, atlas );
  gtk_widget_queue_draw( da );
}

/******************** end module entries ********************/ 
