/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* gamelist.c:                                                             *
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
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <glib/gdataset.h>
#include <mudmagic.h>
#include <gamelist.h>

void gl_free_header (GameListHeader * g) {
	if (g->title) g_free (g->title);
	if (g->description) g_free (g->description);
	if (g->link) g_free (g->link);
	if (g->lbdate) g_free (g->lbdate);
	if (g->generator) g_free (g->generator);
	g_free (g);
}

void gl_free_item (GameListItem * i) {
	if (i->title) g_free (i->title);
	if (i->link) g_free (i->link);
	if (i->description) g_free (i->description);
	if (i->author) g_free (i->author);
	if (i->pub_date) g_free (i->pub_date);
	if (i->game_intro) g_free (i->game_intro);
	if (i->game_host) g_free (i->game_host);
	if (i->game_ip) g_free (i->game_ip);
	if (i->game_genre) g_free (i->game_genre);
	if (i->game_base) g_free (i->game_base);
	if (i->game_theme) g_free (i->game_theme);
	if (i->game_icon) g_free (i->game_icon);
	if (i->pixbuf) gdk_pixbuf_unref (i->pixbuf);
	if (i->sponsor_flash) g_free (i->sponsor_flash);
	if (i->sponsor_image) g_free (i->sponsor_image);
	if (i->meta_keyword) g_free (i->meta_keyword);
	if (i->meta_description) g_free (i->meta_description);
	g_free (i);
}

void gl_gfunc_free_item (gpointer data, gpointer user_data) {
	gl_free_item ((GameListItem *) data);
}

void gl_gamelist_free (GList * gl) {
	if (gl) {
		g_list_foreach (gl, gl_gfunc_free_item, NULL);
		g_list_free (gl);
	}
}

struct _game_list_callbacks {
	char * name;
	void (* callback) (xmlNodePtr, GList **, GameListHeader *);
};

struct _game_list_item_callbacks {
	char * name;
	void (* callback) (xmlNodePtr, GameListItem *);
};

#define CREATE_GLI_STR_FUNC(STRUCTURAL) \
void game_list_item_##STRUCTURAL (xmlNodePtr n, GameListItem * it) { \
	xmlChar * s; \
	s = xmlNodeGetContent (n); \
	it->STRUCTURAL = g_strdup (s); \
	xmlFree (s); \
}

#define CREATE_GLI_STR0_FUNC(STRUCTURAL) \
void game_list_item_##STRUCTURAL (xmlNodePtr n, GameListItem * it) { \
	xmlChar * s; \
	s = xmlNodeGetContent (n); \
	if (g_ascii_strcasecmp ("0", s)) { \
		it->STRUCTURAL = g_strdup (s); \
	} else { \
		it->STRUCTURAL = NULL; \
	} \
	xmlFree (s); \
}

#define CREATE_GLI_INT_FUNC(STRUCTURAL) \
void game_list_item_##STRUCTURAL (xmlNodePtr n, GameListItem * it) { \
	xmlChar * s; \
	s = xmlNodeGetContent (n); \
	it->STRUCTURAL = atoi (s); \
	xmlFree (s); \
}

#define CREATE_GL_FUNC(STRUCTURAL) \
void game_list_##STRUCTURAL (xmlNodePtr n, GList ** l, GameListHeader * h) { \
	xmlChar * s; \
	s = xmlNodeGetContent (n); \
	h->STRUCTURAL = g_strdup (s); \
	xmlFree (s); \
}

CREATE_GL_FUNC (title)
CREATE_GL_FUNC (description)
CREATE_GL_FUNC (link)
CREATE_GL_FUNC (lbdate)
CREATE_GL_FUNC (generator)

CREATE_GLI_STR_FUNC (title)
CREATE_GLI_STR_FUNC (link)
CREATE_GLI_STR_FUNC (description)
CREATE_GLI_STR_FUNC (author)
CREATE_GLI_INT_FUNC (comments)
CREATE_GLI_STR_FUNC (pub_date)
CREATE_GLI_STR_FUNC (game_intro)
CREATE_GLI_STR_FUNC (game_host)
CREATE_GLI_INT_FUNC (game_port)
CREATE_GLI_STR_FUNC (game_ip)
CREATE_GLI_STR_FUNC (game_genre)
CREATE_GLI_STR_FUNC (game_base)
CREATE_GLI_STR_FUNC (game_theme)
CREATE_GLI_INT_FUNC (mudmagic_hosted)
CREATE_GLI_INT_FUNC (game_rank)
CREATE_GLI_INT_FUNC (sponsor_game)
CREATE_GLI_STR_FUNC (sponsor_image)
CREATE_GLI_STR_FUNC (sponsor_flash)
CREATE_GLI_INT_FUNC (bool_gameofmonth)
CREATE_GLI_STR_FUNC (meta_keyword)
CREATE_GLI_STR_FUNC (meta_description)

void game_list_item_game_icon (xmlNodePtr n, GameListItem * it) {
	xmlChar * s;
	s = xmlNodeGetContent (n);
	if (g_ascii_strcasecmp ("0", s)) {
		char * fn = gl_get_icon_filename (s);
		it->game_icon = g_strdup (s);
		it->pixbuf = gdk_pixbuf_new_from_file (fn, NULL);
		g_free (fn);
	} else {
		it->game_icon = NULL;
	}
	xmlFree (s);
}


#define FIND_AND_CALL(I,N,F,S,X,A) \
	F = FALSE; \
	for (I = 0; (I < N) && !F; I++) \
	if (!g_ascii_strcasecmp (S [I].name, X->name)) { \
		S [I].callback A; \
		F = TRUE; \
	} \
	if (!F) fprintf (stderr, "unknown tag found in game list: %s\n", X->name);


#define _game_list_item_callbacks_count (22)
const struct _game_list_item_callbacks glicallbacks [_game_list_item_callbacks_count] = {
	{"title", game_list_item_title},
	{"link", game_list_item_link},
	{"description", game_list_item_description},
	{"author", game_list_item_author},
	{"comments", game_list_item_comments},
	{"pubDate", game_list_item_pub_date},
	{"game_intro", game_list_item_game_intro},
	{"game_host", game_list_item_game_host},
	{"game_port", game_list_item_game_port},
	{"game_ip", game_list_item_game_ip},
	{"game_genre", game_list_item_game_genre},
	{"game_base", game_list_item_game_base},
	{"game_theme", game_list_item_game_theme},
	{"mudmagic_hosted", game_list_item_mudmagic_hosted},
	{"game_rank", game_list_item_game_rank},
	{"game_icon", game_list_item_game_icon},
	{"sponsor_game", game_list_item_sponsor_game},
	{"sponsor_image", game_list_item_sponsor_image},
	{"sponsor_flash", game_list_item_sponsor_flash},
	{"bool_gameofmonth", game_list_item_bool_gameofmonth},
	{"meta_keyword", game_list_item_meta_keyword},
	{"meta_description", game_list_item_meta_description}
};

void game_list_item (xmlNodePtr n, GList ** l, GameListHeader * h) { 
	GameListItem * gli = g_new (GameListItem, 1);
	xmlNodePtr item;

	gli->title = NULL;
	gli->link = NULL;
	gli->description = NULL;
	gli->author = NULL;
	gli->comments = 0;
	gli->pub_date = NULL;
	gli->game_intro = NULL;
	gli->game_host = NULL;
	gli->game_port = 0;
	gli->game_ip = NULL;
	gli->game_genre = NULL;
	gli->game_base = NULL;
	gli->game_theme = NULL;
	gli->mudmagic_hosted = 0;
	gli->game_rank = 0;
	gli->game_icon = NULL;
	gli->sponsor_game = 0;
	gli->bool_gameofmonth = 0;
	gli->pixbuf = NULL;
	gli->sponsor_image= NULL;
	gli->sponsor_flash= NULL;
	for (item = n->children; item; item = item->next) {
		if (XML_ELEMENT_NODE == item->type) {
			gboolean found;
			int i;
					
			FIND_AND_CALL (i, _game_list_item_callbacks_count, found, glicallbacks, item, (item, gli))
		}
	}
	* l = g_list_append (*l, gli);
}

#define _game_list_callbacks_count (6)
const struct _game_list_callbacks glcallbacks [_game_list_callbacks_count] = {
	{"title", game_list_title},
	{"description", game_list_description},
	{"link", game_list_link},
	{"lastBuildDate", game_list_lbdate},
	{"generator", game_list_generator},
	{"item", game_list_item}
};

void gl_get_games (char * filename, GList ** gla, GameListHeader ** glha) {
	xmlDocPtr doc; /* the resulting document tree */
	xmlNodePtr root, chan = NULL, item;
	GList * gl = NULL;
	GameListHeader * glh = NULL;

	doc = xmlReadFile (filename, NULL, 0);
	if (!doc) {
		fprintf (stderr, "Failed to parse %s\n", filename);
		return;
	}
	root = xmlDocGetRootElement (doc);
	if (root && (XML_ELEMENT_NODE == root->type)) {
		for (
			chan = root->children;
			chan && ((XML_ELEMENT_NODE != chan->type) || g_ascii_strcasecmp ((gchar *) chan->name, "channel"));
			chan = chan->next
		);
	} else {
		fprintf (stderr, "root node not found in %s\n", filename);
	}
	if (chan) {
		glh = g_new (GameListHeader, 1);
		glh->title = NULL;
		glh->description = NULL;
		glh->link = NULL;
		glh->lbdate = NULL;
		glh->generator = NULL;
		for (item = chan->children; item; item = item->next) {
			if (XML_ELEMENT_NODE == item->type) {
				gboolean found;
				int i;
				
				FIND_AND_CALL (i, _game_list_callbacks_count, found, glcallbacks, item, (item, &gl, glh))
			}
		}
	} else {
		fprintf (stderr, "game list is malformed in %s\n", filename);
	}
	xmlFreeDoc (doc);
	xmlCleanupParser ();
	if (glha) * glha = glh;
	else gl_free_header (glh);
	*gla = gl;
}

gchar * gl_get_icon_filename (gchar * url) {
	char * fname = g_strrstr (url, "/");

	if (fname) fname = g_build_path (G_DIR_SEPARATOR_S, get_configuration ()->imagedir, fname + 1, NULL);
	return fname;
}

