/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* sessions.c:                                                             *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                2005  Shlykov Vasiliy ( vash@vasiliyshlykov.org )        *
*                   *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdlib.h>
#include <gtk/gtk.h>
#include <mudmagic.h>
#include <protocols.h>
#include <network.h>
#include <log.h>
#include <website.h>
#include <module.h>
#include <gamelist.h>
#include <fcntl.h>
#include "interface.h"
#include "proxy.h"
#include "directories.h"
#include <glib/gstdio.h>
#include <pcre.h>

extern CONFIGURATION *config;

void internal_attach_session (GtkWidget * window, SESSION_STATE * session);
void session_update_init_gamelist_downloading (char * url, char * message, char * dest, gboolean force);

void on_game_list_selection_changed (GtkTreeSelection * sel, gpointer user_data) {
	GtkTreeView * tv = gtk_tree_selection_get_tree_view (sel);

	if (sel) {
		GtkTreeModel * model;
		GList * rows;
		GameListItem * gli;
		GtkTreeIter iter;
		GtkLabel * author, * link, * host, * port;
		GtkTextView * descr;
		GtkWidget * top = gtk_widget_get_toplevel (GTK_WIDGET (tv));
		char buf [128];

		model = gtk_tree_view_get_model (tv);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		if (rows) {
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) g_list_first (rows)->data);
			gtk_tree_model_get (model, &iter, 14, &gli, -1);
			author = GTK_LABEL (interface_get_widget (top, "label_author"));
			link = GTK_LABEL (interface_get_widget (top, "label_link"));
			host = GTK_LABEL (interface_get_widget (top, "label_host"));
			port = GTK_LABEL (interface_get_widget (top, "label_port"));
			descr = GTK_TEXT_VIEW (interface_get_widget (top, "textview_description"));
			gtk_label_set_text (author, gli->author);
			gtk_label_set_text (host, gli->game_host);
			g_snprintf (buf, 128, "%d", gli->game_port);
			gtk_label_set_text (port, buf);
			gtk_text_buffer_set_text (gtk_text_view_get_buffer (descr), gli->description, -1);
			g_snprintf (buf, 128, "<u><span stretch=\"condensed\" foreground=\"blue\">%s</span></u>", gli->link);
			gtk_label_set_markup (link, buf);
		}
	}
}

static gboolean link_event_after (GtkWidget * label, GdkEvent  * ev) {
	GdkEventButton *event;
	GtkWidget * top;
	GtkLabel * link;
	char * url;

	if (ev->type != GDK_BUTTON_RELEASE) return FALSE;
	event = (GdkEventButton *) ev;
	if (event->button != 1) return FALSE;

	top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (label)));
	link = GTK_LABEL (interface_get_widget (top, "label_link"));
	url = (char *) gtk_label_get_text (link);
	try_to_execute_url (WEB_BROWSER, url);
	return TRUE;
}

void session_create_new_char (GameListItem * gli, GtkWidget * window, GtkWidget * top) {
	GtkDialog * dialog;
	GtkComboBox * cbp;
	GtkEntry * name;
	SESSION_STATE *session;
	gchar * text;

	dialog = GTK_DIALOG (interface_create_object_by_name ("dialog_new_char"));
	cbp = GTK_COMBO_BOX (interface_get_widget (GTK_WIDGET (dialog), "combo_proxy"));
	name = GTK_ENTRY (interface_get_widget (GTK_WIDGET (dialog), "entry_name"));
 	proxy_setup_combo (cbp, config->proxies);
	if (GTK_RESPONSE_OK == gtk_dialog_run (dialog)) {
		session = session_new ();
		// add this session to sessions list
		config->sessions = g_list_append (config->sessions, session);
		session->slot = session_get_free_slot (config);
		text = (char *) gtk_entry_get_text (name);
		if (!strlen (text)) session->name = g_strdup ("N/A");
		else session->name = g_strdup (text);
		session->game_host = g_strdup (gli->game_host);
		session->game_port = gli->game_port;
		session->game_name = g_strdup (gli->title);
		if (gtk_combo_box_get_active (cbp)) {
			char * proxy = (gchar *) gtk_combo_box_get_active_text (cbp);
			session->proxy = proxy;
		} else {
			session->proxy = g_strdup ("Default");
		}
		internal_attach_session (window, session);
		session_save (session);
		if (top) gtk_widget_destroy(top);
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static gboolean spim_event_after (GtkWidget * wid, GdkEvent  * ev, GameListItem * gli) {
	GdkEventButton *event;

	if (ev->type != GDK_BUTTON_RELEASE) return FALSE;
	event = (GdkEventButton *) ev;
	if (event->button != 1) return FALSE;

	if (gli) {
		GtkWidget * top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (wid)));
		GtkWidget * window = g_object_get_data (G_OBJECT (top), "active window");
		session_create_new_char (gli, window, top);
	}
	return TRUE;
}

void session_show_gamelist (GtkWidget * win, gpointer user_data);

#define check_search_count (7)
const char * check_search [check_search_count] = {
	"check_name",
	"check_theme",
	"check_intro",
	"check_codebase",
	"check_author",
	"check_keyword",
	"check_description"
};

#define CHECK_IT(BUTTON,TEXT) \
	if ((-1 == rc) && (TEXT) && gtk_toggle_button_get_active (BUTTON)) \
		rc = pcre_exec (what, NULL, TEXT, strlen (TEXT), 0, 0, ov, 30);

gboolean session_gl_check_occurence (GtkWidget * gl, GtkWidget * dlg, GtkTreeModel * model, GtkTreeIter * where, pcre * what) {
	GtkToggleButton * name = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_name"));
	GtkToggleButton * theme = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_theme"));
	GtkToggleButton * intro = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_intro"));
	GtkToggleButton * codebase = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_codebase"));
	GtkToggleButton * author = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_author"));
	GtkToggleButton * keyword = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_keyword"));
	GtkToggleButton * description = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_description"));
	GameListItem * gli;
	int ov [30];
	int rc = -1;

	gtk_tree_model_get (model, where, 14, &gli, -1);
	CHECK_IT (name, gli->title)
	CHECK_IT (theme, gli->game_theme)
	CHECK_IT (intro, gli->game_intro)
	CHECK_IT (codebase, gli->game_base)
	CHECK_IT (author, gli->author)
	CHECK_IT (keyword, gli->meta_keyword)
	CHECK_IT (description, gli->meta_description)
	return -1 != rc;
}

void session_gl_find_up (GtkWidget * wid, gpointer user_data) {
	GtkWidget * gl = GTK_WIDGET (user_data);
	GtkWidget * dlg = gtk_widget_get_toplevel (GTK_WIDGET (wid));
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (gl, "treeview_games"));
	GtkTreeSelection * sel = gtk_tree_view_get_selection(tv);
	GtkTreeModel * model = gtk_tree_view_get_model (tv);
	GList * rows = gtk_tree_selection_get_selected_rows (sel, &model);
	GtkEntry * en = GTK_ENTRY (interface_get_widget (dlg, "entry_find"));
	GtkToggleButton * bcase = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_case"));
	const char * what = gtk_entry_get_text (en);
	GtkTreeIter iter;
	gboolean found, last;
	pcre * r;
	const gchar * error;
	int erroffset;
	int flags = 0;

	if (1 == g_list_length (rows)) {
		GtkTreePath * tp = (GtkTreePath *) g_list_first (rows)->data;

		flags |= gtk_toggle_button_get_active (bcase) ? 0 : PCRE_CASELESS;
		r = pcre_compile (what, flags, &error, &erroffset, NULL);
		if (!r) {
			GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				"Regular expression format error"
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
			return;
		}
		do {
			last = !(gtk_tree_path_prev (tp) && gtk_tree_model_get_iter (model, &iter, tp));
			found = last ? FALSE : session_gl_check_occurence (gl, dlg, model, &iter, r);
		} while (!(last || found));
		if (found) {
			gtk_tree_selection_select_iter (sel, &iter);
			tp = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_scroll_to_cell (tv, tp, NULL, FALSE, 0, 0);
			gtk_tree_path_free (tp);
		} else {
			GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				"Occurence not found"
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
		}
		pcre_free (r);
	}
}

void session_gl_find_down (GtkWidget * wid, gpointer user_data) {
	GtkWidget * gl = GTK_WIDGET (user_data);
	GtkWidget * dlg = gtk_widget_get_toplevel (GTK_WIDGET (wid));
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (gl, "treeview_games"));
	GtkTreeSelection * sel = gtk_tree_view_get_selection(tv);
	GtkTreeModel * model = gtk_tree_view_get_model (tv);
	GList * rows = gtk_tree_selection_get_selected_rows (sel, &model);
	GtkEntry * en = GTK_ENTRY (interface_get_widget (dlg, "entry_find"));
	GtkToggleButton * bcase = GTK_TOGGLE_BUTTON (interface_get_widget (dlg, "check_case"));
	const char * what = gtk_entry_get_text (en);
	GtkTreeIter iter;
	gboolean found, last;
	pcre * r;
	const gchar * error;
	int erroffset;
	int flags = 0;

	if (1 == g_list_length (rows)) {
		GtkTreePath * tp = (GtkTreePath *) g_list_first (rows)->data;

		flags |= gtk_toggle_button_get_active (bcase) ? 0 : PCRE_CASELESS;
		r = pcre_compile (what, flags, &error, &erroffset, NULL);
		if (!r) {
			GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				"Regular expression format error"
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
			return;
		}
		gtk_tree_model_get_iter (model, &iter, tp);
		do {
			last = !gtk_tree_model_iter_next (model, &iter);
			found = last ? FALSE : session_gl_check_occurence (gl, dlg, model, &iter, r);
		} while (!(last || found));
		if (found) {
			gtk_tree_selection_select_iter (sel, &iter);
			tp = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_scroll_to_cell (tv, tp, NULL, FALSE, 0, 0);
			gtk_tree_path_free (tp);
		} else {
			GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				"Occurence not found"
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
		}
		pcre_free (r);
	}
}

void session_gamelist_find (GtkWidget * wid, gpointer user_data) {
	GtkWidget * top = gtk_widget_get_toplevel (GTK_WIDGET (wid));
	GtkWidget * d = GTK_WIDGET (interface_create_object_by_name ("dialog_find"));
	int i;

	for (i = 0; i < check_search_count; i++) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (interface_get_widget (d, (char *) check_search [i])), TRUE);
	}
	g_signal_connect (G_OBJECT (interface_get_widget (d, "button_up")), "clicked", G_CALLBACK (session_gl_find_up), top);
	g_signal_connect (G_OBJECT (interface_get_widget (d, "button_down")), "clicked", G_CALLBACK (session_gl_find_down), top);
	gtk_widget_show_all (d);
}

void session_show_selected_colums (GtkTreeView * tv) {
	int i;
	unsigned long int n = config->wiz_vis_cols;

	if (!n) {
		n = 0x0F;
		config->wiz_vis_cols = n;
	}
	for (i = 0; i < 12; i++) { // first two columns are always visible
		gtk_tree_view_column_set_visible (gtk_tree_view_get_column (tv, i + 2), 1 & n);
		n >>= 1;
	}
}

#define SET_COLUMN_CHECK(A) \
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (A), 1 & n); \
	n >>= 1;

#define GET_COLUMN_CHECK(A) \
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (A))) n |= k; \
	k <<= 1;

void session_gamelist_pref (GtkWidget * wid, gpointer user_data) {
	GtkWidget * top = gtk_widget_get_toplevel (GTK_WIDGET (wid));
	GtkWidget * d = GTK_WIDGET (interface_create_object_by_name ("dialog_wiz_pref"));
	GtkWidget * name = (interface_get_widget (d, "check_name"));
	GtkWidget * theme = (interface_get_widget (d, "check_theme"));
	GtkWidget * intro = (interface_get_widget (d, "check_intro"));
	GtkWidget * codebase = (interface_get_widget (d, "check_codebase"));
	GtkWidget * genre = (interface_get_widget (d, "check_genre"));
	GtkWidget * pub_date = (interface_get_widget (d, "check_pub_date"));
	GtkWidget * comments = (interface_get_widget (d, "check_comments"));
	GtkWidget * author = (interface_get_widget (d, "check_author"));
	GtkWidget * link = (interface_get_widget (d, "check_link"));
	GtkWidget * ip = (interface_get_widget (d, "check_ip"));
	GtkWidget * host = (interface_get_widget (d, "check_host"));
	GtkWidget * port = (interface_get_widget (d, "check_port"));
	unsigned long int n = config->wiz_vis_cols;
	unsigned long k = 1;

	SET_COLUMN_CHECK (name)
	SET_COLUMN_CHECK (theme)
	SET_COLUMN_CHECK (intro)
	SET_COLUMN_CHECK (codebase)
	SET_COLUMN_CHECK (genre)
	SET_COLUMN_CHECK (pub_date)
	SET_COLUMN_CHECK (comments)
	SET_COLUMN_CHECK (author)
	SET_COLUMN_CHECK (link)
	SET_COLUMN_CHECK (ip)
	SET_COLUMN_CHECK (host)
	SET_COLUMN_CHECK (port)
	if (GTK_RESPONSE_OK == gtk_dialog_run (GTK_DIALOG (d))) {
		n = 0;
		GET_COLUMN_CHECK (name)
		GET_COLUMN_CHECK (theme)
		GET_COLUMN_CHECK (intro)
		GET_COLUMN_CHECK (codebase)
		GET_COLUMN_CHECK (genre)
		GET_COLUMN_CHECK (pub_date)
		GET_COLUMN_CHECK (comments)
		GET_COLUMN_CHECK (author)
		GET_COLUMN_CHECK (link)
		GET_COLUMN_CHECK (ip)
		GET_COLUMN_CHECK (host)
		GET_COLUMN_CHECK (port)
		config->wiz_vis_cols = n ? n : 4; // show name if none colums selected
		session_show_selected_colums (GTK_TREE_VIEW (interface_get_widget (top, "treeview_games")));
	}
	gtk_widget_destroy (d);
}

gint pixbufs_cmp_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data) {
	gpointer pa, pb;
	gint i = (gint) user_data;

	gtk_tree_model_get (model, a, i, &pa, -1);
	gtk_tree_model_get (model, b, i, &pb, -1);
	return ((NULL == pa) ^ (NULL == pb)) ? (NULL == pa ? 1 : -1) : 0;
}

void setup_game_view_interface (GtkWidget * win) {
	GtkListStore * store;
	GtkCellRenderer * ren;
	GtkTreeViewColumn * col;
	GtkToolItem * refresh;
	GtkTreeView * tv;
	GtkTooltips * tips;
	GtkToolbar * bar;
	char buf [128];
	GtkToggleToolButton * all, * featured, * top30;
	GtkWidget * link, * eb, * search, * pref, * frame;
	GdkCursor * hand = gdk_cursor_new (GDK_HAND2);
	GdkColor color;
	
    g_assert (win);
	tv = GTK_TREE_VIEW (interface_get_widget (win, "treeview_games"));
	bar = GTK_TOOLBAR (interface_get_widget (win, "toolbar_new_char"));
	refresh = GTK_TOOL_ITEM (interface_get_widget (win, "tool_refresh"));
	all = GTK_TOGGLE_TOOL_BUTTON (interface_get_widget (win, "radio_all"));
	featured = GTK_TOGGLE_TOOL_BUTTON (interface_get_widget (win, "radio_featured"));
	top30 = GTK_TOGGLE_TOOL_BUTTON (interface_get_widget (win, "radio_top30"));
	frame = interface_get_widget (win, "event_frame");
	eb = interface_get_widget (win, "eventbox_link");
	link = interface_get_widget (win, "label_link");
	search = interface_get_widget (win, "tool_find");
	pref = interface_get_widget (win, "tool_pref");
	gtk_widget_hide_all (frame);
	gdk_color_parse ("white", &color);
	gtk_widget_modify_bg (frame, GTK_STATE_NORMAL, &color);
	tips = gtk_tooltips_new ();
	gtk_toolbar_set_tooltips (bar, TRUE);
	if (((time_t) (-1)) == config->gl_last_upd) {
		g_snprintf (buf, 128, "last updated: unknown/never");
	} else {
		strftime (buf, 128, "last updated: %x %X", localtime (&config->gl_last_upd));
	}
	gtk_tool_item_set_tooltip (refresh, tips, buf, buf);
	/* look of table */
	store = gtk_list_store_new (
		15,
		GDK_TYPE_PIXBUF,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,

		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_INT,
		
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_INT,

		G_TYPE_POINTER
	);
	gtk_tree_view_set_model (tv, GTK_TREE_MODEL (store));
	ren = gtk_cell_renderer_pixbuf_new ();
	// icon
	col = gtk_tree_view_column_new_with_attributes ("", ren, "pixbuf", 0, NULL);
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (col, 48);
	gtk_tree_view_column_set_sort_column_id (col, 0);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store), 0, pixbufs_cmp_func, (gpointer) 0, NULL);
	gtk_tree_view_append_column (tv, col);
	// hosted
	col = gtk_tree_view_column_new_with_attributes ("", ren, "pixbuf", 1, NULL);
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (col, 48);
	gtk_tree_view_append_column (tv, col);
	gtk_tree_view_column_set_sort_column_id (col, 1);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store), 1, pixbufs_cmp_func, (gpointer) 1, NULL);
	ren = gtk_cell_renderer_text_new ();
	// name
	col = gtk_tree_view_column_new_with_attributes ("Name", ren, "text", 2, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 2);
	gtk_tree_view_append_column (tv, col);
	// theme
	col = gtk_tree_view_column_new_with_attributes ("Theme", ren, "text", 3, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 3);
	gtk_tree_view_append_column (tv, col);
	// intro
	col = gtk_tree_view_column_new_with_attributes ("Intro", ren, "text", 4, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 4);
	gtk_tree_view_append_column (tv, col);
	// codebase
	col = gtk_tree_view_column_new_with_attributes ("Codebase", ren, "text", 5, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 5);
	gtk_tree_view_append_column (tv, col);
	// genre
	col = gtk_tree_view_column_new_with_attributes ("Genre", ren, "text", 6, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 6);
	gtk_tree_view_append_column (tv, col);
	// pub_date
	col = gtk_tree_view_column_new_with_attributes ("Pub. date", ren, "text", 7, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 7);
	gtk_tree_view_append_column (tv, col);
	// comments
	col = gtk_tree_view_column_new_with_attributes ("Comments", ren, "text", 8, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 8);
	gtk_tree_view_append_column (tv, col);
	// author
	col = gtk_tree_view_column_new_with_attributes ("Author", ren, "text", 9, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 9);
	gtk_tree_view_append_column (tv, col);
	// link
	col = gtk_tree_view_column_new_with_attributes ("Link", ren, "text", 10, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 10);
	gtk_tree_view_append_column (tv, col);
	// ip
	col = gtk_tree_view_column_new_with_attributes ("IP", ren, "text", 11, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 11);
	gtk_tree_view_append_column (tv, col);
	// host
	col = gtk_tree_view_column_new_with_attributes ("Host", ren, "text", 12, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 12);
	gtk_tree_view_append_column (tv, col);
	// port
	col = gtk_tree_view_column_new_with_attributes ("Port", ren, "text", 13, NULL);
	gtk_tree_view_column_set_sort_column_id (col, 13);
	gtk_tree_view_append_column (tv, col);
	session_show_selected_colums (tv);
	g_signal_connect (G_OBJECT (search), "clicked", G_CALLBACK (session_gamelist_find), NULL);
	g_signal_connect (G_OBJECT (pref), "clicked", G_CALLBACK (session_gamelist_pref), NULL);
	g_signal_connect (G_OBJECT (all), "toggled", G_CALLBACK (session_show_gamelist), NULL);
	g_signal_connect (G_OBJECT (top30), "toggled", G_CALLBACK (session_show_gamelist), NULL);
	g_signal_connect (G_OBJECT (featured), "toggled", G_CALLBACK (session_show_gamelist), NULL);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (tv)), "changed", G_CALLBACK (on_game_list_selection_changed), NULL);
	gdk_window_set_cursor (eb->window, hand);
	g_signal_connect (eb, "event-after", G_CALLBACK (link_event_after), NULL);
	if (!config->gamelist) gl_get_games (config->gamelistfile, &config->gamelist, NULL);
	if (config->gamelist) {
		GameListItem * gli = NULL;
		GList * i = g_list_first (config->gamelist);

		while (i && !((gli = (GameListItem *) i->data), gli->sponsor_game)) i = g_list_next (i);
		if (gli && gli->sponsor_image) {
			GtkImage * spim = GTK_IMAGE (interface_get_widget (win, "image_featured"));
			GtkEventBox * espim = GTK_EVENT_BOX (interface_get_widget (win, "eventbox_spim"));

			gtk_image_set_from_file (
				spim,
				gl_get_icon_filename (gli->sponsor_image)
			);
			g_signal_connect (espim, "event-after", G_CALLBACK (spim_event_after), gli);
		}
	}
}

struct session_gl_insertion {
	GtkListStore * store;
	GdkPixbuf * mudmagic_icon;
	int with_icon; // 1 - show games with icons, 0 - show ones without icons, -1 - don't care
};

void session_gl_show_all (gpointer data, gpointer user_data) {
	GameListItem * gli = (GameListItem *) data;
	struct session_gl_insertion * sgi = (struct session_gl_insertion *) user_data;
	GtkTreeIter iter;
	char buf [128];
	time_t t;

	if ((-1 == sgi->with_icon) ||  ((0 == sgi->with_icon) ^ (NULL != gli->pixbuf))) {
		gtk_list_store_append (sgi->store, &iter);
		t = curl_getdate (gli->pub_date, NULL);
		strftime (buf, 128, "%x %X", localtime (&t));
		gtk_list_store_set (sgi->store, &iter, 
			2, gli->title, 
			3, gli->game_theme, 
			4, gli->game_intro, 
			5, gli->game_base, 
			6, gli->game_genre,
			7, buf,
			8, gli->comments,
			9, gli->author,
			10, gli->link,
			11, gli->game_ip,
			12, gli->game_host,
			13, gli->game_port,
			14, gli, 
		-1);
		if (gli->pixbuf) gtk_list_store_set (sgi->store, &iter, 0, gli->pixbuf, -1);
		if (gli->mudmagic_hosted) gtk_list_store_set (sgi->store, &iter, 1, sgi->mudmagic_icon, -1);
	}
}

void session_gl_show_top30 (gpointer data, gpointer user_data) {
	GameListItem * gli = (GameListItem *) data;
	struct session_gl_insertion * sgi = (struct session_gl_insertion *) user_data;
	GtkTreeIter iter;
	char buf [128];
	time_t t;

	if (gli->game_rank && ((-1 == sgi->with_icon) ||  ((0 == sgi->with_icon) ^ (NULL != gli->pixbuf)))) {
		gtk_list_store_append (sgi->store, &iter);
		t = curl_getdate (gli->pub_date, NULL);
		strftime (buf, 128, "%x %X", localtime (&t));
		gtk_list_store_set (sgi->store, &iter, 
			2, gli->title, 
			3, gli->game_theme, 
			4, gli->game_intro, 
			5, gli->game_base, 
			6, gli->game_genre,
			7, buf,
			8, gli->comments,
			9, gli->author,
			10, gli->link,
			11, gli->game_ip,
			12, gli->game_host,
			13, gli->game_port,
			14, gli, 
		-1);
		if (gli->pixbuf) gtk_list_store_set (sgi->store, &iter, 0, gli->pixbuf, -1);
		if (gli->mudmagic_hosted) gtk_list_store_set (sgi->store, &iter, 1, sgi->mudmagic_icon, -1);
	}
}

void session_show_gamelist (GtkWidget * w, gpointer user_data) {
	GtkWidget * win = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (w)));
	GtkTreeView * tv;
	GtkToggleToolButton * all, * featured, * top30;
	struct session_gl_insertion sgi;
	GtkTreeIter iter;
	GtkWidget * paned, * frame, * search, * pref;
	char * fn;

	g_assert (win);
	if (!gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (w))) return;
	tv = GTK_TREE_VIEW (interface_get_widget (win, "treeview_games"));
	g_assert (tv);
	all = GTK_TOGGLE_TOOL_BUTTON (interface_get_widget (win, "radio_all"));
	featured = GTK_TOGGLE_TOOL_BUTTON (interface_get_widget (win, "radio_featured"));
	top30 = GTK_TOGGLE_TOOL_BUTTON (interface_get_widget (win, "radio_top30"));
	sgi.store = (GtkListStore *) gtk_tree_view_get_model (tv);
	fn = g_build_filename (mudmagic_data_directory (), "interface", "mudmagic_hosted.png", NULL);
	sgi.mudmagic_icon = gdk_pixbuf_new_from_file (fn, NULL);
	g_free (fn);
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (sgi.store), &iter)) while (gtk_list_store_remove (sgi.store, &iter));
	paned = interface_get_widget (win, "vpaned_gamelist");
	frame = interface_get_widget (win, "event_frame");
	search = interface_get_widget (win, "tool_find");
	pref = interface_get_widget (win, "tool_pref");
	if (!config->gamelist) gl_get_games (config->gamelistfile, &config->gamelist, NULL);
	if (!config->gamelist) {
		GtkDialog * md = GTK_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "Game list is empty. Press 'Update' button to download one from MudMagic.Com."
		));
		gtk_dialog_run (md);
		gtk_widget_destroy (GTK_WIDGET (md));
	}
	if (gtk_toggle_tool_button_get_active (featured)) {
		GtkLabel * author = GTK_LABEL (interface_get_widget (win, "label_author"));
		GtkLabel * link = GTK_LABEL (interface_get_widget (win, "label_link"));
		GtkLabel * host = GTK_LABEL (interface_get_widget (win, "label_host"));
		GtkLabel * port = GTK_LABEL (interface_get_widget (win, "label_port"));
		GtkLabel * f_name = GTK_LABEL (interface_get_widget (win, "label_f_name"));
		GtkLabel * f_theme = GTK_LABEL (interface_get_widget (win, "label_f_theme"));
		GtkTextView * descr = GTK_TEXT_VIEW (interface_get_widget (win, "textview_description"));
		GList * i = g_list_first (config->gamelist);
		GameListItem * gli = NULL;

		while (i && !((gli = (GameListItem *) i->data), gli->sponsor_game)) i = g_list_next (i);
		if (gli) {
			char buf [128];

			gtk_label_set_text (author, gli->author);
			gtk_label_set_text (host, gli->game_host);
			gtk_label_set_text (f_name, gli->title);
			gtk_label_set_text (f_theme, gli->game_theme);
			g_snprintf (buf, 128, "%d", gli->game_port);
			gtk_label_set_text (port, buf);
			g_snprintf (buf, 128, "<u><span stretch=\"condensed\" foreground=\"blue\">%s</span></u>", gli->link);
			gtk_label_set_markup (link, buf);
			gtk_text_buffer_set_text (gtk_text_view_get_buffer (descr), gli->description, 0);
		} else {
			gtk_label_set_text (author, "");
			gtk_label_set_text (host, "");
			gtk_label_set_text (port, "");
			gtk_label_set_text (link, "");
			gtk_label_set_text (f_name, "");
			gtk_label_set_text (f_theme, "");
			gtk_text_buffer_set_text (gtk_text_view_get_buffer (descr), "", 0);
		}
		gtk_widget_show_all (frame);
		gtk_widget_hide_all (paned);
		gtk_widget_set_sensitive(search, FALSE);
		gtk_widget_set_sensitive(pref, FALSE);
	} else {
		gtk_widget_show_all (paned);
		gtk_widget_hide_all (frame);
		if (gtk_toggle_tool_button_get_active (all)) {
			sgi.with_icon = 1;
			g_list_foreach (config->gamelist, session_gl_show_all, &sgi);
			sgi.with_icon = 0;
			g_list_foreach (config->gamelist, session_gl_show_all, &sgi);
		}
		if (gtk_toggle_tool_button_get_active (top30)) {
			sgi.with_icon = 1;
			g_list_foreach (config->gamelist, session_gl_show_top30, &sgi);
			sgi.with_icon = 0;
			g_list_foreach (config->gamelist, session_gl_show_top30, &sgi);
		}
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (sgi.store), &iter)) {
			gtk_tree_selection_select_iter (gtk_tree_view_get_selection (tv), &iter);
		}
		gtk_widget_set_sensitive(search, TRUE);
		gtk_widget_set_sensitive(pref, TRUE);
	}
}

void sessions_create_new_char_intf (char * def_mode) {
	GtkWidget * win, * t, * rbut;

    win = GTK_WIDGET (interface_create_object_by_name ("window_new_char"));
	if ((t = interface_get_active_window ())) {
		g_object_set_data(G_OBJECT(win), "active window", t);
		rbut = interface_get_widget (win, def_mode);
		g_assert (rbut);
		gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (rbut), TRUE);
		setup_game_view_interface (win);
		session_show_gamelist (rbut, NULL);
	}
}

void on_new1_activate (GtkMenuItem * menuitem, gpointer user_data) {
	sessions_create_new_char_intf ("radio_all");
}

// multiline button toggle
void on_toggle_ml_toggled(GtkToggleButton * togglebutton,
        gpointer user_data)
{
  GtkWidget *tab;
  SESSION_STATE *session;
  tab =
      interface_get_widget(GTK_WIDGET(togglebutton), "session_tab");
  session = g_object_get_data(G_OBJECT(tab), "session");
  session->single_line = !togglebutton->active;
  interface_tab_refresh(tab);
}

void on_button_reconnect_clicked(GtkButton * button, gpointer user_data)
{
  GtkWidget *tab;
  SESSION_STATE *session;

  tab = interface_get_active_tab();
  if (!tab) {
    interface_display_message("No active session !!!\n");
    return;
  }
  session = g_object_get_data(G_OBJECT(tab), "session");
  if (!session) {
    interface_display_message("No active session !!!\n");
    return;
  }
  if (session->telnet->fd < 0 
  ||  session->telnet->fd == -1) 
  {
	if (session->pconn) proxy_connection_close (session->pconn);
	session->pconn = proxy_connection_open(
      session->game_host, session->game_port, proxy_get_by_name (session->proxy, config->proxies)
    );
	if (session->pconn) session->telnet->fd = session->pconn->sock; else session->telnet->fd = NO_CONNECTION;

    if (session->telnet->fd < 0 )
    {
      interface_messagebox( GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                  network_errmsg (session->telnet->fd));
      return;
    }
    session->input_event_id = gtk_input_add_full(session->telnet->fd, GDK_INPUT_READ,
               (GdkInputFunction)on_data_available, NULL, tab, NULL);
  } else {
    interface_display_message("Already connected !!!\n");
  }
}


void on_saved_games_delete_clicked(GtkButton * button, gpointer user_data)
{
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *slot;
  treeview = interface_get_widget(GTK_WIDGET(button),
          "saved_games_treeview");
  if (!treeview)
    return;
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  if (!selection) {
    interface_display_message("No selection !!!");
    return;
  }
  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, 4, &slot, -1);
	mud_dir_remove (slot);
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  } else {
    interface_display_message("No selection !!!");
  }
}

void on_saved_games_selection_changed (GtkTreeView * tv, gpointer user_data) {
	GtkTreeSelection * sel;
	GtkWidget * top;

	top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (tv)));
	sel = gtk_tree_view_get_selection (tv);
	if (sel) {
		GtkTreeModel * model;
		gint r;
		GList * rows;
		GtkComboBox * combo;

		combo = GTK_COMBO_BOX (interface_get_widget (top, "combobox_proxy"));
		model = gtk_tree_view_get_model (tv);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		if (1 == g_list_length (rows)) {
			char * proxy_name;
			GtkTreeIter iter;
			GtkTreeModel * combo_model = gtk_combo_box_get_model (combo);

			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) g_list_first (rows)->data);
			gtk_tree_model_get (model, &iter, 3, &proxy_name, -1);

			if (proxy_name) {
				if (!g_ascii_strcasecmp (proxy_name, "Default")) {
					gtk_combo_box_set_active (combo, 0);
				} else if (gtk_tree_model_get_iter_first (combo_model, &iter)) {
					char * n;

					while ( 
						(gtk_tree_model_get (combo_model, &iter, 0, &n, -1), r = g_ascii_strcasecmp (proxy_name, n)) && 
						gtk_tree_model_iter_next (combo_model, &iter)
					) g_free (n);
					g_free (n);
					if (r) {
						gtk_combo_box_set_active (combo, 0);
					} else {
						gtk_combo_box_set_active_iter (combo, &iter);
					}
				}
				g_free (proxy_name);
			} else {
				gtk_combo_box_set_active (combo, 1);
			}
		} else {
			gtk_combo_box_set_active (combo, -1);
		}
		g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (rows);
	} else {
		g_printf ("no selection\n");
	}
}

void on_saved_games_proxy_changed (GtkComboBox * cb, gpointer user_data) {
	GtkTreeSelection * sel;
	GtkWidget * top;
	GtkTreeView * tv;
	char * proxy_name;
	gint idx;

	top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (cb)));
	tv = GTK_TREE_VIEW (interface_get_widget (top, "saved_games_treeview"));
	sel = gtk_tree_view_get_selection (tv);
	proxy_name = gtk_combo_box_get_active_text (cb);
	idx = gtk_combo_box_get_active (cb);
	if (sel) {
		GtkTreeModel * model;
		GtkTreeIter iter;
		GList * rows;

		model = gtk_tree_view_get_model (tv);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		if (1 == g_list_length (rows)) {
			char * slot;
			char * pp;
			char * p = idx ? proxy_name : "Default";

			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) g_list_first (rows)->data);
			gtk_tree_model_get (model, &iter, 3, &pp, 4, &slot, -1);
			if ((pp != p) && ((!p) || (!pp) || g_ascii_strcasecmp (p, pp))) {
				gtk_list_store_set ((GtkListStore *) model, &iter, 3, p, -1);
				session_saved_set_proxy (slot, p);
			}
		}
	}
	g_free (proxy_name);
}

void on_open1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GError *error = NULL;
  GtkWidget *win;
  GtkListStore *store;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;
  GtkCellRenderer *renderer;
  GtkWidget *treeview;
  GtkTreeIter iter;
  GtkComboBox * proxies;
  GDir *dir;
  gchar *entry = NULL;
  gchar *t = NULL;
  gchar *name = NULL;
  gchar *game_name = NULL;
  gchar *proxy_name = NULL;
  GtkWidget *active_window;
  GdkPixbuf * pix;
  gboolean exist;

  win = interface_create_object_by_name("window_saved_characters");
  if ((active_window = interface_get_active_window())) {
    /* link the new character with current active window */
    g_object_set_data(G_OBJECT(win), "active window",
          active_window);
  } else {
    /* never should get here */
#ifdef DEBUG
    g_print("[on_open1_activate] no windows active ??\n");
#endif
    gtk_widget_destroy(win);
  }

  // open saves directory
  if (!g_file_test(config->savedir, G_FILE_TEST_IS_DIR)) {
    return;
  }
  dir = g_dir_open(config->savedir, 0, &error);
  if (!dir) {
    return;
  }


  treeview = interface_get_widget(win, "saved_games_treeview");

  store =
	gtk_list_store_new (5,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING
	);

  //      fill the store with some data
  while ((entry = (gchar *) g_dir_read_name(dir))) {
    t = g_build_path(G_DIR_SEPARATOR_S, config->savedir, entry,
         NULL);
    exist = session_saved_get_name(t, &name, &game_name, &proxy_name);

                if (exist && (name != NULL || game_name != NULL))
                {
				if (!proxy_is_valid_name (config->proxies, proxy_name)) {
					g_free (proxy_name);
					proxy_name = g_strdup ("Default");
					session_saved_set_proxy (t, proxy_name);
				}

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 1, name, 2, game_name, 3, proxy_name, 4, t, -1);
		session_saved_load_icon (game_name, &pix);
		if (pix) {
	        gtk_list_store_set(store, &iter, 0, pix, -1);
			gdk_pixbuf_unref (pix);
		}
		
                }
                else
                {
                    if (session_slot_is_empty (t)
              && interface_remove_empty_slot (entry) )
                      {
                        session_remove_empty_slot (t);
                      }
                }
    g_free(t);
    g_free(name);
    g_free(game_name);
    g_free(proxy_name);
                name = NULL;
                game_name = NULL;
                proxy_name = NULL;
  }


  gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),
        GTK_TREE_MODEL(store));

	renderer = gtk_cell_renderer_pixbuf_new ();
	// icon
	column = gtk_tree_view_column_new_with_attributes ("", renderer, "pixbuf", 0, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 48);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

  renderer = gtk_cell_renderer_text_new();
  column =
      gtk_tree_view_column_new_with_attributes("Name", renderer,
                 "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  renderer = gtk_cell_renderer_text_new();
  column =
      gtk_tree_view_column_new_with_attributes("Mud", renderer,
                 "text", 2, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  column = gtk_tree_view_column_new_with_attributes ("Proxy", renderer, "text", 3, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  proxies = GTK_COMBO_BOX (interface_get_widget (win, "combobox_proxy"));
  proxy_setup_combo (proxies, config->proxies);
  g_signal_connect (G_OBJECT (treeview), "cursor_changed", G_CALLBACK (on_saved_games_selection_changed), NULL);
  g_signal_connect (G_OBJECT (proxies), "changed", G_CALLBACK (on_saved_games_proxy_changed), NULL);

  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

}

void internal_set_tab_label(GtkWidget * notebook, GtkWidget * tab)
{
  GtkWidget *wid, *hbox, *icon;
  GtkWidget *label;
  SESSION_STATE *session;
  gchar *text;
  g_return_if_fail(notebook != NULL && tab != NULL);
  session = g_object_get_data(G_OBJECT(tab), "session");
  g_return_if_fail(session != NULL);

  wid = gtk_event_box_new();
  if (session->game_name != NULL) {
    label = gtk_label_new(session->game_name);
  } else {
    text =
        g_strdup_printf("%s:%d", session->game_host,
            session->game_port);
    label = gtk_label_new(text);
    g_free(text);
  }
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
        icon = gtk_image_new_from_stock (GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
        hbox = gtk_hbox_new (FALSE, 2);
        gtk_box_pack_start (GTK_BOX(hbox), icon, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(wid), hbox);
  gtk_widget_show_all (wid);

        g_object_set_data (G_OBJECT (wid), "label_icon", icon);

  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), tab, wid);

  g_signal_connect((gpointer) wid, "button_press_event",
       G_CALLBACK(on_eventbox_tab_button_press_event),
       tab);

}

void internal_create_tagtable(GtkWidget * tab)
{
  GtkWidget *out1;
  GtkTextBuffer *buffer;
  GtkTextTagTable *tagtable;
  
  out1 = interface_get_widget(tab, "output1");
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(out1));
  tagtable = gtk_text_buffer_get_tag_table(buffer);
  gtk_text_buffer_create_tag(buffer, "underline",
           "underline-set", TRUE,
           "underline", PANGO_UNDERLINE_SINGLE,
           NULL);
  gtk_text_buffer_create_tag(buffer, "italic",
           "style-set", TRUE,
           "style", PANGO_STYLE_ITALIC, NULL);
  gtk_text_buffer_create_tag(buffer, "bold",
           "weight-set", TRUE,
           "weight", PANGO_WEIGHT_BOLD, NULL);
  gtk_text_buffer_create_tag(buffer, "horzline",
           "strikethrough-set", TRUE,
           "strikethrough", TRUE,
            NULL);
}

void internal_attach_session(GtkWidget * window, SESSION_STATE * session)
{
  GtkWidget *tab,
      *notebook;

  tab = interface_add_tab(window, NULL);
  session->tab = (gpointer) tab;
  g_object_set_data(G_OBJECT(tab), "session", session);
  //g_message ("Session attached to tab");

  // set the tab label
  notebook =
      (GtkWidget *) g_object_get_data(G_OBJECT(window), "notebook");
  internal_set_tab_label(notebook, tab);

  internal_create_tagtable(tab);

  interface_tab_refresh(tab);

  // start logging if there is the case
  if (session->logging) {
    session->log_file = log_open_logfile(session->slot);
  }

  interface_tab_connect(tab);
  module_call_all_session_open(config->modules, session);

  // update statusvars and gauges
  update_svlist (session->svlist);
  update_gaugelist (session->gaugelist);
}

void on_new_char_create_clicked (GtkButton * button, gpointer user_data) {
	GtkWidget *window, *top;
	GtkTreeSelection * sel;
	GtkTreeView * tv;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkToggleToolButton * featured;
	GList * rows;
	GameListItem * gli = NULL;

	top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	g_return_if_fail (top != NULL);
	window = g_object_get_data (G_OBJECT (top), "active window");
	g_return_if_fail (window != NULL);
	featured = GTK_TOGGLE_TOOL_BUTTON (interface_get_widget (top, "radio_featured"));
	if (gtk_toggle_tool_button_get_active (featured)) {
		GList * i = g_list_first (config->gamelist);

		while (i && !((gli = (GameListItem *) i->data), gli->sponsor_game)) i = g_list_next (i);
	} else {
		tv = GTK_TREE_VIEW (interface_get_widget (top, "treeview_games"));
		g_return_if_fail (tv != NULL);
		sel = gtk_tree_view_get_selection (tv);
		g_return_if_fail (sel != NULL);
		model = gtk_tree_view_get_model (tv);
		g_return_if_fail (model != NULL);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		if (1 == g_list_length (rows)) {
			// create a new session
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) g_list_first (rows)->data);
			gtk_tree_model_get (model, &iter, 14, &gli, -1);
		}
	}
	if (gli) session_create_new_char (gli, window, top);
}

void on_saved_games_new_clicked(GtkButton * button, gpointer user_data)
{
  GtkWidget* top = gtk_widget_get_toplevel(GTK_WIDGET(button));
        g_return_if_fail(top != NULL);

  gtk_widget_destroy(top);

  on_new1_activate ((GtkMenuItem*) button, user_data);
}

void on_saved_games_load_clicked(GtkButton * button, gpointer user_data)
{
  GtkTreeSelection *selection;
  GtkWidget *treeview;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *slot;

  SESSION_STATE *session;
  GtkWidget *top;
  GtkWidget *active_window;

  treeview =
      interface_get_widget(GTK_WIDGET(button),
         "saved_games_treeview");
  g_return_if_fail(treeview != NULL);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, 4, &slot, -1);
    //g_message("load from %s", slot);

    top = gtk_widget_get_toplevel(GTK_WIDGET(button));
    active_window =
        g_object_get_data(G_OBJECT(top), "active window");

    session = session_new();
                session->slot = slot;
    session_load(session, session->slot);
                if (session->gerrors != NULL)
                {
                    interface_show_gerrors (session->gerrors, "Can't load session.");
                    session_delete (session);
                }
                else
                {
                    // add this session to sessions list
                    config->sessions =
                        g_list_append(config->sessions, session);

                    gtk_widget_destroy(top);

                    internal_attach_session(active_window, session);
                }

  } else {
    interface_display_message("Please select game from list.");
    return;
  }
}


gboolean on_saved_games_treeview_button_press_event (GtkWidget *widget,
                            GdkEventButton *event, gpointer func_data)
{
  GtkTreeSelection *selection;
  GtkWidget        *treeview, *button;
  GtkTreeIter      iter;
  GtkTreeModel    *model;

  treeview =
      interface_get_widget(GTK_WIDGET(widget),
         "saved_games_treeview");
  g_return_val_if_fail(treeview != NULL, FALSE);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  if (gtk_tree_selection_get_selected(selection, &model, &iter))
        {
            if (event->type == GDK_2BUTTON_PRESS
                    || event->type == GDK_3BUTTON_PRESS)
            {
                button = interface_get_widget (widget, "saved_games_load");
                g_return_val_if_fail(GTK_BUTTON (button) != NULL, FALSE);
                
                on_saved_games_load_clicked(GTK_BUTTON(button), NULL);
                return TRUE;
            }
  }
        return FALSE;
}


gchar *internal_key_to_string(gint state, gint key)
{
  gchar *buff = g_malloc0(125);
  if (state & GDK_CONTROL_MASK)
    strcat(buff, "Ctrl+");
  if (state & GDK_MOD1_MASK)
    strcat(buff, "Alt+");
  strcat(buff, gdk_keyval_name(key));
  return buff;
}

gboolean on_entry_macro_expr_key_press_event(gpointer button,
               GdkEventKey * event,
               GtkWidget * widget)
{
  gboolean done = FALSE;

  gint state = event->state;
  gint key = gdk_keyval_to_upper(event->keyval);

  if ((state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) != 0) {
    if (key < 65500) {
      gchar *buff = internal_key_to_string(state, key);
      gtk_entry_set_text(GTK_ENTRY(widget), buff);
      g_free(buff);
      done = TRUE;
    }
  } else {
    if ((key > 255) && (key < 65500)) {
      gtk_entry_append_text(GTK_ENTRY(widget),
                gdk_keyval_name(key));
      done = TRUE;
    }
  }
  if (done) {
    GTK_WIDGET_UNSET_FLAGS(widget, GTK_CAN_FOCUS);
    gtk_widget_grab_focus(button);
  }
  return FALSE;
}

/* User clicks on the CAPTURE button of macro */
void on_button_macro_capture_clicked(gpointer entry, GtkButton * button)
{
  g_return_if_fail(entry != NULL);
  gtk_entry_set_text(GTK_ENTRY(entry), "");
  GTK_WIDGET_SET_FLAGS(entry, GTK_CAN_FOCUS);
  gtk_widget_grab_focus(GTK_WIDGET(entry));
}

void on_close1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *window;
  GtkWidget *notebook;
  GtkWidget *tab;
  gint t;

  window = interface_get_active_window();
  g_return_if_fail(window != NULL);
  notebook = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "notebook"));
  if (notebook) {
    if ((t =
         gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)))
        > -1) {
      // get current tab
      tab =
          gtk_notebook_get_nth_page(GTK_NOTEBOOK
                  (notebook), t);

      // remove it
      interface_remove_tab(tab);
      return;
    }
  }
  g_message("no notebook!");
  interface_remove_window(window);
}

void on_quick_connect_1_activate(GtkMenuItem * menuitem,
         gpointer user_data)
{
  GtkWidget *dialog, *entry1, *entry2, *win;
  SESSION_STATE *session;
  gchar* host, *port, *proxy;
  gint response;
  GtkComboBox * proxies;

  win = interface_get_active_window();

  dialog = interface_create_object_by_name("dialog_quick_connect");
  g_return_if_fail(dialog != NULL);
  entry1 = interface_get_widget(dialog, "entry_host");
  g_return_if_fail(entry1 != NULL);
  entry2 = interface_get_widget(dialog, "entry_port");
  g_return_if_fail(entry2 != NULL);
  proxies = GTK_COMBO_BOX (interface_get_widget (dialog, "combobox_proxy"));
  proxy_setup_combo (proxies, config->proxies);

  while (TRUE) {
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_CANCEL) {
      break;
    }
    host = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry1)));
    g_strstrip(host);
    port = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2)));
    g_strstrip(port);
    if (strlen(host) == 0) {
      interface_display_message
          ("'Host' field is empty.");
      g_free(host);
      g_free(port);
      continue;
    }
    if (strlen(port) == 0) {
      interface_display_message
          ("'Port' field is empty.");
      g_free(host);
      g_free(port);
      continue;
                }

/** kyndig: -for keeping connections local

                if (mud_ip_parse (host, &ip))
                {
                    if (!mud_ip_local (&ip))
                    {
                        if (get_configuration()->gamelist == NULL)
                        {
                            load_gamelist (get_configuration());
                        }

                        if (get_configuration()->gamelist == NULL
                                || !gamelist_find (get_configuration()->gamelist,
                                                   &find_ip_gamelist,
                                                   host)
                           )
                        {
                            interface_messagebox ( GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    "Cannot connect to game host.");
                            g_free (host);
                            g_free (port);
                            return;
                        }
                    }
                }
**/
    // create a new session
    session = session_new();
    // add this session to sessions list
    config->sessions =
        g_list_append(config->sessions, session);
    session->slot = session_get_free_slot(config);
    session->game_host = host;
    session->game_port = utils_atoi(port, -1);
	if (gtk_combo_box_get_active (proxies)) {
	    proxy = (gchar *) gtk_combo_box_get_active_text (proxies);
    	session->proxy = proxy;
	} else {
		session->proxy = g_strdup ("Default");
	}
   	g_free(port);
    session->game_name =
        g_strdup_printf("%s:%d", session->game_host,
            session->game_port);
    internal_attach_session(win, session);
    session_save(session);
    break;
  }

  gtk_widget_destroy(dialog);
}


void on_character1_menu_activated(GtkMenuItem * menuitem,
          gpointer user_data)
{
  SESSION_STATE *session;
  GtkWidget *item;
  item = interface_get_widget((GtkWidget *) menuitem, "reconnect1");
  g_return_if_fail(item != NULL);
  session = interface_get_active_session();
  if (session == NULL) {
    gtk_widget_set_sensitive(item, FALSE);
  } else {
    if (session->telnet->fd == NO_CONNECTION ) {
      gtk_widget_set_sensitive(item, FALSE);
    } else {
      gtk_widget_set_sensitive(item, TRUE);
    }
  }
}

void on_reconnect1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *tab;

  tab = interface_get_active_tab();
  g_return_if_fail(tab != NULL);

  interface_tab_disconnect(tab);
  interface_tab_connect(tab);
}

/*
	update game list
*/

struct games_thread_data {
	GtkWidget * progress_bar;
	GList * icons;
	gboolean destroyed;
	gboolean done;
	double dtotal;
	double dnow;
	long status;
	DownloadedData * dd;
	GAsyncQueue * queue;
	gboolean force;
	char * url;
	char * dest;
	char * msg;
	char * err;
};

int session_update_progress (void * clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	struct games_thread_data * gd = (struct games_thread_data *) clientp;

	if (gd->destroyed) {
		gd->err = "Cancelled.";
	} else {
		gd->dtotal = dltotal;
		gd->dnow = dlnow;
		g_async_queue_push (gd->queue, (gpointer) gd);
	}
	return gd->destroyed;
}

gpointer session_update_game_list_thread (gpointer data) {
	CURL * curl = curl_easy_init ();
	long code = -1;
	DownloadedData * dd;
	struct games_thread_data * gd = (struct games_thread_data *) data;

	g_async_queue_ref (gd->queue);
	gd->dtotal = 0;
	gd->dnow = 0;
	if (curl) {
		curl_easy_setopt (curl, CURLOPT_NOPROGRESS, FALSE);
		curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION, session_update_progress);
		curl_easy_setopt (curl, CURLOPT_PROGRESSDATA, data);
		if ((((time_t) (-1)) != config->gl_last_upd) && !gd->force) {
			curl_easy_setopt (curl, CURLOPT_TIMEVALUE, config->gl_last_upd);
			curl_easy_setopt (curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
		}
		code = proxy_download_url (curl, proxy_get_default (config->proxies), gd->url, NULL, NULL, NULL, &dd);
		if (-1 == code) {
			 if (!gd->err) gd->err = "Connection to server failed.";
		} else gd->dd = dd;
	}
	gd->done = TRUE;
	gd->status = code;
	g_async_queue_push (gd->queue, (gpointer) gd);
	g_async_queue_unref (gd->queue);
	return NULL;
}

gpointer session_update_icons_thread (gpointer data) {
	CURL * curl = curl_easy_init ();
	long code = -1;
	DownloadedData * dd;
	struct games_thread_data * gd = (struct games_thread_data *) data;
	GList * i = g_list_first (gd->icons);
	Proxy * proxy = proxy_get_default (config->proxies);

	g_async_queue_ref (gd->queue);
	gd->dtotal = 0;
	gd->dnow = 0;
	if (curl) {
		guint n = g_list_length (i), k = 0;
		for (; i && !gd->destroyed; i = g_list_next (i)) {
			
			code = proxy_download_url (curl, proxy, (gchar *) i->data, NULL, NULL, NULL, &dd);
			if (-1 == code) {
				fprintf (stderr, "failed to download %s\n", (gchar *) i->data);
			} else {
				char * fname = gl_get_icon_filename ((gchar *) i->data);

				if (fname) {
					g_file_set_contents (fname, dd->buf, dd->size, NULL);
					g_free (fname);
				}
				discard_downloaded_data (dd);
			}
			gd->dnow = ++k;
			gd->dtotal = n;
			g_async_queue_push (gd->queue, (gpointer) gd);
		}
	}
	gd->done = TRUE;
	gd->status = code;
	g_async_queue_push (gd->queue, (gpointer) gd);
	g_async_queue_unref (gd->queue);
	return NULL;
}

void session_get_icon_urls (gpointer data, gpointer user_data) {
	GList ** gi = (GList **) user_data;
	GameListItem * gli = (GameListItem *) data;

	if (gli->game_icon) *gi = g_list_append (*gi, gli->game_icon);
	if (gli->sponsor_image) *gi = g_list_append (*gi, gli->sponsor_image);
	//if (gli->sponsor_flash) *gi = g_list_append (*gi, gli->sponsor_flash);
}

gboolean session_icons_update_ready (gpointer data) {
	struct games_thread_data * gd = (struct games_thread_data *) data;
	struct games_thread_data * rd = (struct games_thread_data *) g_async_queue_try_pop (gd->queue);
	gboolean done = gd->done;

	if (rd) {
		if (!rd->destroyed) {
			if (rd->dtotal) gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gd->progress_bar), rd->dnow / rd->dtotal);
			else gtk_progress_bar_pulse (GTK_PROGRESS_BAR (gd->progress_bar));
		}
		if (done) {
			if (!gd->destroyed) gtk_widget_destroy (gtk_widget_get_toplevel (gd->progress_bar));
			g_list_free (gd->icons);
			g_async_queue_unref (gd->queue);
			g_free (gd);
			gl_gamelist_free (config->gamelist);
			config->gamelist = NULL;
			gl_get_games (config->gamelistfile, &config->gamelist, NULL);
			sessions_create_new_char_intf ("radio_all");
		}
	}
	return !done;
}

static void session_update_progress_destroyed (GtkWidget * widget, gpointer data) {
	struct games_thread_data * gd = (struct games_thread_data *) data;

	gd->destroyed = TRUE;
}

void session_update_init_icons_downloading (char * glfile) {
	GList * icon_urls = NULL;

	if (config->gamelist) {
		gl_gamelist_free (config->gamelist);
		config->gamelist = NULL;
	}
	gl_get_games (config->gamelistfile, &config->gamelist, NULL);
	g_list_foreach (config->gamelist, session_get_icon_urls, &icon_urls);
	if (icon_urls) {
		GtkWidget * d;
		GtkWidget * l;
		GtkWidget * pb;
		GThread * th;
		struct games_thread_data * gd = g_new (struct games_thread_data, 1);
	
		d = gtk_dialog_new_with_buttons ("Downloading game icons", NULL, GTK_DIALOG_MODAL, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
		pb = gtk_progress_bar_new ();
		l = gtk_label_new ("Downloading game icons. Please wait.");
		gtk_misc_set_padding (GTK_MISC (l), 6, 6);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (d)->vbox), l);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (d)->vbox), pb);
		g_signal_connect (G_OBJECT (d), "destroy", G_CALLBACK (session_update_progress_destroyed), gd);
		gtk_widget_show_all (d);
		gd->progress_bar = pb;
		gd->destroyed = FALSE;
		gd->done = FALSE;
		gd->queue = g_async_queue_new ();
		gd->dd = NULL;
		gd->url = NULL;
		gd->dest = NULL;
		gd->err = NULL;
		gd->msg = "Loading game icons. Please wait.";
		gd->icons = icon_urls;
		gd->force = FALSE;
		if ((th = g_thread_create (session_update_icons_thread, gd, FALSE, NULL))) {
			g_idle_add (session_icons_update_ready, gd);
			if (GTK_RESPONSE_CANCEL == gtk_dialog_run (GTK_DIALOG (d))) {
				gtk_widget_destroy (d);
			}
		} else {
			g_error ("unable to create thread\n");
		}
	}
}

gboolean session_update_ready (gpointer data) {
	struct games_thread_data * gd = (struct games_thread_data *) data;
	struct games_thread_data * rd = (struct games_thread_data *) g_async_queue_try_pop (gd->queue);
	gboolean done = gd->done;

	if (rd) {
		if (!rd->destroyed) {
			if (rd->dtotal) gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gd->progress_bar), rd->dnow / rd->dtotal);
			else gtk_progress_bar_pulse (GTK_PROGRESS_BAR (gd->progress_bar));
		}
		if (done) {
			char msg [1024];
			GtkMessageDialog * md;
			gint tmpf, r;
			char * tmpn;

			if (!gd->destroyed) gtk_widget_destroy (gtk_widget_get_toplevel (gd->progress_bar));
				switch (gd->status) {
				case 304:
					md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
						NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						(gchar *) "Game list unchanged since last update. No download necessary. Download game list anyway?"
					));
					r = gtk_dialog_run (GTK_DIALOG (md));
					gtk_widget_destroy (GTK_WIDGET (md));
					if (GTK_RESPONSE_YES == r) session_update_init_gamelist_downloading (gd->url, gd->msg, gd->dest, TRUE);
					else sessions_create_new_char_intf ("radio_all");
				break;
				case 200:
					tmpf = g_file_open_tmp ("mmXXXXXX", &tmpn, NULL);
					if (-1 != tmpf) {
						gzFile gzf;
						size_t n = 0;
						char buf [10240];
						gint dstf = g_open (gd->dest, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	
						if (-1 != dstf) {
							write (tmpf, gd->dd->buf, gd->dd->size);
							lseek (tmpf, 0, SEEK_SET);
							gzf = gzdopen (tmpf, "rb");
							do {
								n = gzread (gzf, buf, 10240);
								if (-1 != n) write (dstf, buf, n);
							} while ((-1 != n) && (0 != n));
							close (dstf);
							config->gl_last_upd = time (NULL);
						} else {
							fprintf (stderr, "file creation failed:%s\n", gd->dest);
							close (tmpf);
						}
						g_remove (tmpn);
						g_free (tmpn);
						session_update_init_icons_downloading (get_configuration()->gamelistfile);
					} else {
						g_error ("tmp file creation failed\n");
					}
				break;
				default:
					if (gd->err) g_snprintf (msg, 1024, "%s", gd->err);
					else g_snprintf (msg, 1024, "Error retrieving game list. HTTP status is %li", gd->status);
					md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
						NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						(gchar *) "%s",
						msg
					));
					gtk_dialog_run (GTK_DIALOG (md));
					gtk_widget_destroy (GTK_WIDGET (md));
					sessions_create_new_char_intf ("radio_all");
				}
			if (-1 != gd->status) discard_downloaded_data (gd->dd);
			g_async_queue_unref (gd->queue);
			g_free (gd);
		}
	}
	return !done;
}

void session_update_init_gamelist_downloading (char * url, char * message, char * dest, gboolean force) {
	GtkWidget * d;
	GtkWidget * l;
	GtkWidget * pb;
	GThread * th;
	struct games_thread_data * gd = g_new (struct games_thread_data, 1);

	d = gtk_dialog_new_with_buttons ("Downloading game list", NULL, GTK_DIALOG_MODAL, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	pb = gtk_progress_bar_new ();
	l = gtk_label_new (message);
	gtk_misc_set_padding (GTK_MISC (l), 6, 6);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (d)->vbox), l);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (d)->vbox), pb);
	g_signal_connect (G_OBJECT (d), "destroy", G_CALLBACK (session_update_progress_destroyed), gd);
	gtk_widget_show_all (d);
	gd->progress_bar = pb;
	gd->destroyed = FALSE;
	gd->done = FALSE;
	gd->queue = g_async_queue_new ();
	gd->dd = NULL;
	gd->url = url;
	gd->dest = dest;
	gd->err = NULL;
	gd->msg = message;
	gd->force = force;
	gd->icons = NULL;
	if ((th = g_thread_create (session_update_game_list_thread, gd, FALSE, NULL))) {
		g_idle_add (session_update_ready, gd);
		if (GTK_RESPONSE_CANCEL == gtk_dialog_run (GTK_DIALOG (d))) {
			gtk_widget_destroy (d);
		}
	} else {
		g_error ("unable to create thread\n");
	}
}

void on_new_char_update_clicked (GtkButton * button, gpointer data) {
	GtkWidget * top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));

	gtk_widget_destroy (top);
	session_update_init_gamelist_downloading (get_configuration ()->gamelisturl, "Downloading game list. Please wait.", get_configuration()->gamelistfile, FALSE);
}

