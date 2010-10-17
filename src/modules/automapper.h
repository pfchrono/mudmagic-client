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
#ifndef AUTOMAPPER
#define AUTOMAPPER 1
#include <glib.h>

enum { AM_N, AM_NE, AM_E, AM_SE, AM_S, AM_SW, AM_W, AM_NW };
#define OPPOSITE(x) ((x)^4) // N->S, SW->NE, W->E ..etc :)

typedef struct _POSITION 	POSITION;
typedef struct _NODE 		NODE;
typedef struct _PATH 		PATH;
typedef struct _MAP			MAP;
typedef struct _ATLAS		ATLAS;

struct _POSITION{
	gint x;
	gint y;
};

struct _NODE{
	gint id;
	POSITION position; 	// node position 
	gint links_in[8]; 	// N,NE,E,SE,S,SW,W,NW links to this node
	gint links_out[8]; 	// N,NE,E,SE,S,SW,W,NW links from this node
	gchar *name;		// node name
	GList *paths_in;	// paths to this done
	GList *paths_out;	// paths from this done
	MAP *map; 			// link to map where node belog to 
};

// a path is a link to a node that ca be in a different map
struct _PATH{
	gchar *command; // unusual move command like in, out, gate, enter
					// here is included "up" and "down" cause the player
					// is moved to another map
	gint map_id;
	gint node_id;
};

struct _MAP {
	gint id; 				// map id
	gint max_node_id;		// 
	NODE *cnode;			// a link to current node
	gchar *name; 			// map name
	GList *nodes;			// nodes list
	GHashTable *nodes_id;	// id   -> node
	GHashTable *nodes_pos;  // pos  -> node
	//GHashTable *nodes_name; // name -> node 
	gint maxx, minx;		// used for drawing area size
	gint maxy, miny;		//
	ATLAS *atlas; 			// a link to atlas where map belong to
};

struct _ATLAS {
	gint max_map_id;		
	MAP	*cmap;				// a link to current map
	GList *maps;
	GHashTable *maps_id;	// a hashtable with maps id
	//GHashTable *maps_name;	// a hashtable with maps names 
	gint lat;  				// the size of the room wall
	gint len;				// the lenght of the link
	gdouble zoom;			// zoom factor
};

// PATH 
static PATH* automapper_path_new( const gchar *command, gint mapid, gint nodeid ); 
static void automapper_path_delete( PATH* path ); 

// NODE
static NODE* automapper_node_new( gint id, gint x, gint y );
static void automapper_node_delete( NODE* node ); 
static void automapper_node_set_name( NODE* node, const gchar *name ); 
static PATH* automapper_node_get_out_path_by_name(NODE *node, gchar *command ); 

// MAP 
static MAP* automapper_map_new( gint id ); 
static void automapper_map_delete( MAP* map ); 
static void automapper_map_set_name( MAP *map, const gchar *name ); 
static void automapper_map_move( MAP* map, guchar direction, gboolean bidirectional );
static NODE* automapper_map_add_node( MAP *map, gint node_x, gint node_y );
static void automapper_map_remove_node( MAP *map, NODE *node );
static NODE *automapper_map_get_node_by_name( MAP *map, gchar *name ); 

// ATLAS
static ATLAS *automapper_atlas_new( ); 
static void automapper_atlas_delete( ATLAS *atlas ); 
static MAP* automapper_atlas_add_map( ATLAS *atlas );

static void debug_atlas_dump( ATLAS *atlas ); 
static NODE *automapper_atlas_get_node( ATLAS *atlas, gint mapid, gint nodeid );
static MAP*  automapper_atlas_get_map_by_name( ATLAS *atlas, gchar *name ); 



#endif //AUTOMAPPER
