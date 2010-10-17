/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* gamelist.h:                                                             *
*                2005  Shlykov Vasiliy ( vash@vasiliyshlykov.org )        *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef __GAMELIST_H__MUDMAGIC
#define __GAMELIST_H__MUDMAGIC

#include <glib/gtypes.h>
#include <muderr.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _game_list_header {
	char * title;
	char * description;
	char * link;
	char * lbdate;
	char * generator;
};

struct _game_list_item {
	char * title;
	char * link;
	char * description;
	char * author;
	int comments;
	char * pub_date;
	char * game_intro;
	char * game_host;
	int game_port;
	char * game_ip;
	char * game_genre;
	char * game_base;
	char * game_theme;
	int mudmagic_hosted;
	int game_rank;
	char * game_icon;
	int sponsor_game;
	char * sponsor_image;
	char * sponsor_flash;
	char * meta_keyword;
	char * meta_description;
	int bool_gameofmonth;
	GdkPixbuf * pixbuf;
};

typedef struct _game_list_header GameListHeader;
typedef struct _game_list_item GameListItem;

/* parses given XML file, returns list of GameListItem`s and header. glha could be NULL if header not need */ 
void gl_get_games (char * filename, GList ** gl, GameListHeader ** glha);
void gl_free_header (GameListHeader * g);
void gl_free_item (GameListItem * i);
void gl_gamelist_free (GList * gl);
gchar * gl_get_icon_filename (gchar * url);

#ifdef __cplusplus
}
#endif

#endif // __GAMELIST_H__MUDMAGIC

