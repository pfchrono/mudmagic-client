/***************************************************************************
 *  Mud Magic Client                                                       *
 *  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 * tools.c:                                                                *
 *                2006  Victor Vorodyukhin ( victor.scorpion@gmail.com )   *
 *                   *
 ***************************************************************************/
/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#define _XOPEN_SOURCE
#include <gtk/gtk.h>
#include <glib/giochannel.h>
#include <mudmagic.h>
#include <interface.h>
#include <alias_triggers.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <time.h>
#include <glib/gprintf.h>
#include "tools.h"

#ifdef HAVE_WINDOWS
/*
 * To get prototypes for the following POSIXish functions, you have to
 * include the indicated non-POSIX headers. The functions are defined
 * in OLDNAMES.LIB (MSVC) or -lmoldname-msvc (mingw32).
 *
 * getcwd: <direct.h> (MSVC), <io.h> (mingw32)
 * getpid: <process.h>
 * access: <io.h>
 * unlink: <stdio.h> or <io.h>
 * open, read, write, lseek, close: <io.h>
 * rmdir: <io.h>
 */
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define closesocket close

#endif

#define SCRIPT_LANG_BASIC	(1)
#define SCRIPT_LANG_PYTHON	(2)

#define DELAYED_CMD_LOG		(1)
#define MUDCFG_HOME_DIR         ".mudmagic"

#define TRY_ASSIGN(VAR,ITEM) VAR = ITEM; g_return_if_fail (NULL != VAR);
#ifndef HAVE_WINDOWS
#define USE_IO_CHANNEL 1
#endif

#define RS_EXPORT_URL "www.mudmagic.com/mud-client/toolbox/export"
#define RS_IMPORT_URL "www.mudmagic.com/mud-client/toolbox/import"
#define RS_REMOVE_URL "www.mudmagic.com/mud-client/toolbox/remove"
#define RS_GET_SAVES_URL "www.mudmagic.com/mud-client/toolbox/get_saves"

extern CONFIGURATION * config;

struct _serv_msg_data {
	GtkWindow * win;
	GtkTextBuffer * buf;
#ifdef USE_IO_CHANNEL
	GIOChannel * chan;
#else
	guint input_event_id;
#endif
	gchar * msg;
	gboolean done;
	int fds [2];
};

struct _log_entry {
	SESSION_STATE * session;
	gchar * file_name;
	GtkWindow * window;
	gboolean repeat;
};

struct _saved_game_info {
	char * name;
	char * mud;
	time_t saved;
	gboolean remote;
	gboolean local;
};

typedef struct _saved_game_info SavedGameInfo;
typedef struct _serv_msg_data serv_msg_data;
typedef struct _log_entry log_entry;

/* constansts corresponding to remote storage action modes */
#define RS_ACT_GET_MESSAGE (1)
#define RS_ACT_PERFORM (2)

/*
 common functions
*/

void on_tools_menu_activated (GtkMenuItem * menuitem, gpointer user_data) {
	SESSION_STATE * session;
	GtkWidget * ta_item, * lt_item, * vl_item, * dc_item;
	
	TRY_ASSIGN (ta_item, GTK_WIDGET (interface_get_widget ((GtkWidget *) menuitem, "ta_testing")));
	TRY_ASSIGN (lt_item, GTK_WIDGET (interface_get_widget ((GtkWidget *) menuitem, "lt_passing")));
	TRY_ASSIGN (vl_item, GTK_WIDGET (interface_get_widget ((GtkWidget *) menuitem, "log_view")));
	TRY_ASSIGN (dc_item, GTK_WIDGET (interface_get_widget ((GtkWidget *) menuitem, "delayed_cmd")));
	
	session = interface_get_active_session ();

	if (session) {
		gtk_widget_set_sensitive(ta_item, TRUE);
		gtk_widget_set_sensitive(lt_item, TRUE);
		gtk_widget_set_sensitive(dc_item, TRUE);
		gtk_widget_set_sensitive(vl_item, TRUE);
	} else {
		gtk_widget_set_sensitive(ta_item, FALSE);
		gtk_widget_set_sensitive(lt_item, FALSE);
		gtk_widget_set_sensitive(dc_item, FALSE);
		gtk_widget_set_sensitive(vl_item, FALSE);
	}
}

void on_tools_common_button_clear (GtkButton * button, gpointer user_data) {
	GtkTextView * tv;
	
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (
		GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button))), "textview_tools_text")));
	gtk_text_buffer_set_text (gtk_text_view_get_buffer (tv), "", 0);
}

void on_tools_common_button_cancel (GtkButton * button, gpointer user_data) {
	gtk_widget_destroy (GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button))));
}

void on_tools_common_open (GtkButton * button, gpointer user_data) {
	GtkWidget *dialog;
	GtkWindow * window;
	GtkLabel * l_fname;

	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (l_fname, GTK_LABEL (interface_get_widget (GTK_WIDGET (window), "label_tools_fname_hid")));
	dialog = gtk_file_chooser_dialog_new ("Open File",
		window,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		gsize fs;
		gchar * fc;
		GError * err = NULL;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		if (g_file_get_contents (filename, &fc, &fs, &err)) {
			GtkTextView * tv;

			TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
			gtk_text_buffer_set_text (gtk_text_view_get_buffer (tv), fc, -1);
			gtk_label_set_text (l_fname, filename);
			g_free (fc);
		} else {
			GtkMessageDialog * md;

			md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				(gchar *) "%s",
				err->message
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
		}
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

void on_tools_common_save (GtkButton * button, gpointer user_data) {
	GtkWidget * dialog;
	GtkWindow * window;
	GtkLabel * l_fname;
	const gchar * c_fname;

	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (l_fname, GTK_LABEL (interface_get_widget (GTK_WIDGET (window), "label_tools_fname_hid")));
	dialog = gtk_file_chooser_dialog_new ("Save File",
		window,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL);
#ifdef GTK_2_8_9_PLUS
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
#endif
	c_fname = gtk_label_get_text (l_fname);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), c_fname);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		gchar * fc;
		GError * err = NULL;
		GtkTextView * tv;
		GtkTextBuffer * buf;
		GtkTextIter start, end;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
		TRY_ASSIGN (buf, gtk_text_view_get_buffer (tv));
		gtk_text_buffer_get_start_iter (buf, &start);
		gtk_text_buffer_get_end_iter (buf, &end);
		fc = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
		if (g_file_set_contents (filename, fc, -1, &err)) {

			gtk_label_set_text (l_fname, filename);
		} else {
			GtkMessageDialog * md;

			md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				(gchar *) "%s",
				err->message
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
		}
		g_free (fc);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

static void append_errs (gpointer data, gpointer user_data) {
	char ** x = (char **) user_data;
	char * z;

	z = g_strjoin ("\n", (gchar *) data, *x, NULL);
	g_free (*x);
	*x = z;
}

int tools_socket_pair (int sp [2]) {
    struct sockaddr_in sa;
    int r, p;

    /* creating listening socket */
    sp [0] = socket (PF_INET, SOCK_STREAM, 0);
    if (-1 == sp [0]) return -1;
    sa.sin_family = AF_INET;
#ifdef HAVE_WINDOWS
    sa.sin_addr.S_un.S_addr = inet_addr ("127.0.0.1");
#else
    sa.sin_addr.s_addr = inet_addr ("127.0.0.1");
#endif
    /* looking for a free port in dynamic ports range */
    p = 49152;
    do {
        sa.sin_port = htons (p++);
        r = bind (sp [0], (struct sockaddr *) &sa, sizeof (sa));
    } while ((-1 == r) && (65535 > sa.sin_port));
    if (r) return -1;
    if (-1 == listen (sp [0], 0)) return -1;
    /* creating client socket */
    sp [1] = socket (PF_INET, SOCK_STREAM, 0);
    if (-1 == sp [1]) {
        closesocket (sp [0]);
        return -1;
    }
    if (-1 == connect (sp [1], (struct sockaddr *) &sa, sizeof (sa))) {
        closesocket (sp [0]);
        closesocket (sp [1]);
        return -1;
    }
    /* accepting connection */
    p = accept (sp [0], NULL, 0);
    if (-1 == p) {       
        closesocket (sp [0]);
        closesocket (sp [1]);
        return -1;
    } else {
        closesocket (sp [0]);
        sp [0] = p;
    }
    return 0;
}

static void serv_sim_destroy (GtkWidget * widget, gpointer data) {
	GtkWidget * w;
	serv_msg_data * smg = (serv_msg_data *) data;

	w = gtk_widget_get_toplevel (widget);
	smg->win = NULL;
	smg->buf = NULL;
	gtk_widget_destroy (GTK_WIDGET (w));
	if ((w == widget) && smg->done) {
		g_free (smg->msg);
		g_free (smg);
	}
}

void serv_sim_apply_msg (serv_msg_data * smg, gchar * buf) {
	GtkTextView * tv;

	if (smg->msg) {
		gchar * x = smg->msg;
		smg->msg = g_strconcat (x, buf, NULL);
		g_free (x);
	} else {
		smg->msg = g_strdup (buf);
	}
	if (!smg->win) {
		TRY_ASSIGN (smg->win, GTK_WINDOW (interface_create_object_by_name ("window_tools_serv_msg")));
		TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (smg->win), "textview_tools_text")));
		TRY_ASSIGN (smg->buf, gtk_text_view_get_buffer (tv));
		g_signal_connect (G_OBJECT (smg->win), "destroy", G_CALLBACK (serv_sim_destroy), smg);
		g_signal_connect (G_OBJECT (interface_get_widget (GTK_WIDGET (smg->win), "button_tools_serv_msg_close")), 
			"clicked", G_CALLBACK (serv_sim_destroy), smg);
	}
	gtk_text_buffer_set_text (smg->buf, smg->msg, -1);
}

gboolean serv_sim_data (GIOChannel * source, GIOCondition condition, gpointer data) {
	gchar * buf;
	GError * err = NULL;
	gsize br;
	serv_msg_data * smg = (serv_msg_data *) data;
	gboolean r = TRUE;

	g_io_channel_read_line (source, &buf, &br, NULL, &err);
	if (buf) {
		serv_sim_apply_msg (smg, buf);
		g_free (buf);
	} else {
		r = FALSE;
	}
	return r;
}

gboolean serv_sim_hup (GIOChannel * source, GIOCondition condition, gpointer data) {
	return FALSE;
}

void tools_on_test_data_available (gpointer smg, gint fd, GdkInputCondition cond) {
	char buf [128];
	int n;

	memset (buf, 0, 128);
	n = recv (fd, buf, 128, 0);
	if (n) serv_sim_apply_msg (smg, buf);
	else close (fd);
}

serv_msg_data * init_serv_sim (SESSION_STATE * session) {
	serv_msg_data * smg = g_new (serv_msg_data, 1);

	tools_socket_pair (smg->fds);
	smg->win = NULL;
	smg->buf = NULL;
	smg->msg = NULL;
	smg->done = FALSE;
#ifdef USE_IO_CHANNEL
	smg->chan = g_io_channel_unix_new (smg->fds [0]);
	g_io_add_watch (smg->chan, G_IO_IN, serv_sim_data, smg);
	g_io_add_watch (smg->chan, G_IO_HUP, serv_sim_hup, smg);
	g_io_add_watch (smg->chan, G_IO_ERR, serv_sim_hup, smg);
#else
    smg->input_event_id = gtk_input_add_full (smg->fds [0], GDK_INPUT_READ, (GdkInputFunction) tools_on_test_data_available, NULL, smg, NULL);
#endif
	session->telnet = telnet_new ();
	session->telnet->fd = smg->fds [1];
	return smg;
}

/*
	scripts testing
*/

void on_scripts_testing_activate (GtkMenuItem * menuitem, gpointer user_data) {
	GtkWindow * window;
	GtkComboBox * lang;
	GtkTextView * tv;
	int i;

	TRY_ASSIGN (window, GTK_WINDOW (interface_create_object_by_name ("window_tools_scripts_testing")));
	TRY_ASSIGN (lang, GTK_COMBO_BOX (interface_get_widget (GTK_WIDGET (window), "combo_tools_scripts_testing_lang")));
	for (i = 0; i < ATMLanguageCount; i++) {
		gtk_combo_box_append_text (lang, Languages [i].name);
	}
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	gtk_widget_grab_focus (GTK_WIDGET (tv));
	gtk_combo_box_set_active (lang, 0);
}

static void run_script (char * scr, int lang, const gchar * alias, const gchar * cmdline) {
	ATM * atm;
	SESSION_STATE * session;
	int r;
	serv_msg_data * smg;
	gboolean running = TRUE;

	atm = atm_new ();
	session = session_new ();
	session->local_echo = 0;

	smg = init_serv_sim (session);

	if (alias && cmdline) {
		GList * cmdlist = NULL;
		gchar * text = g_strdup (cmdline);
		int text_size = strlen (text);

		atm->raiser = g_strdup (alias);
		atm->text = g_strdup (scr);
		atm->name = g_strdup ("script testing");
		atm->action = ATM_ACTION_SCRIPT;
		cmdlist = g_list_append (cmdlist, atm);
		if (!atm_find_fire (session, text, text_size, cmdlist, TRUE, &r)) {
			GtkMessageDialog * md;
			md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				(gchar *) "%s",
				"Command line passed doesn't match alias expression."
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
			running = FALSE;
		}
		g_list_free (cmdlist);
		g_free (text);
	} else {
		atm_init_macro (atm, "test", scr, lang, NULL, NULL, ATM_ACTION_SCRIPT);
		r = atm_execute (session, atm, NULL, 0);
	}
	if (running) {
		if (r) {
			gchar * s = "\n____________________\nScript checks OK";
			send_command (session, s, strlen (s));
		} else {
			gchar * g;
			g = "\n____________________";
			send_command (session, g, strlen (g));
			if (atm->errors) {
				char * buf = NULL;
				GList * er = g_list_reverse (atm->errors);
	
				g_list_foreach (er, append_errs, &buf);
				send_command (session, buf, strlen (buf));
				g_free (buf);
			} else {
				gchar * s = "<unknown error>";
				send_command (session, s, strlen (s));
			}
			g = "Script checks FAILED";
			send_command (session, g, strlen (g));
		}
	}
	session_delete (session);
	atm_free (atm);
	smg->done = TRUE;
}

void on_scripts_testing_button_ok (GtkButton * button, gpointer user_data) {
	GtkWindow * window;
	GtkTextView * tv;
	GtkTextBuffer * buf;
	GtkTextIter start, end;
	GtkComboBox * lang;
	int l;
	
	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	TRY_ASSIGN (buf, gtk_text_view_get_buffer (tv));
	gtk_text_buffer_get_start_iter (buf, &start);
	gtk_text_buffer_get_end_iter (buf, &end);
	TRY_ASSIGN (lang, GTK_COMBO_BOX (interface_get_widget (GTK_WIDGET (window), "combo_tools_scripts_testing_lang")));
	l = gtk_combo_box_get_active (lang);
	if (-1 == l) {
		GtkMessageDialog * md;
		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "%s",
			"No Script language selected!"
		));
		gtk_dialog_run (GTK_DIALOG (md));
		gtk_widget_destroy (GTK_WIDGET (md));
	} else {
		gchar * text;
		GtkEntry * e_alias, * e_line;
		const gchar * alias, * line;

		TRY_ASSIGN (e_alias, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_alias_name")));
		TRY_ASSIGN (e_line, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_cmdline_passed")));
		alias = gtk_entry_get_text (e_alias);
		line = gtk_entry_get_text (e_line);
		if (!strlen (alias)) alias = NULL;
		if (!strlen (line)) line = NULL;
		text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
		run_script (text, Languages [l].id, alias, line);
		g_free (text);
	}
}

/*
	aliases and triggers testing
*/

void on_ta_testing_activate (GtkMenuItem * menuitem, gpointer user_data) {
	GtkWindow * window;
	GtkTextView * tv;

	TRY_ASSIGN (window, GTK_WINDOW (interface_create_object_by_name ("window_tools_ta_testing")));
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	gtk_widget_grab_focus (GTK_WIDGET (tv));
}

void on_ta_testing_button_ok (GtkButton * button, gpointer user_data) {
	GtkWindow * window;
	GtkTextView * tv;
	GtkTextBuffer * buf;
	int text_size, r;
	gchar * text;
	GtkTextIter start, end;
	SESSION_STATE * session, * test_session;
	ATM * match;
	serv_msg_data * smg;
	
	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	TRY_ASSIGN (buf, gtk_text_view_get_buffer (tv));
	gtk_text_buffer_get_start_iter (buf, &start);
	gtk_text_buffer_get_end_iter (buf, &end);
	text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

	session = interface_get_active_session ();
	g_return_if_fail (NULL != session);

	test_session = session_new ();
	g_return_if_fail (NULL != test_session);

	test_session = session_new ();
	test_session->local_echo = 0;
	smg = init_serv_sim (test_session);

	text_size = strlen (text);

	(void) ((match = atm_find_fire (test_session, text, text_size, session->aliases, TRUE, &r)) ||
	(match = atm_find_fire (test_session, text, text_size, config->aliases, TRUE, &r)) ||
	(match = atm_find_fire (test_session, text, text_size, session->triggers, TRUE, &r)) ||
	(match = atm_find_fire (test_session, text, text_size, config->triggers, TRUE, &r)));

	session_delete (test_session);
	close (smg->fds [1]);
	smg->done = TRUE;

	if (!match) {
		gtk_widget_destroy (GTK_WIDGET (smg->win));
	}

	g_free (text);
}

/*
	long text passing
*/

void take_top_line (GtkTextBuffer * buf, GtkEntry * ent) {
	GtkTextIter l1, l2;
	gchar * text;

	gtk_text_buffer_get_iter_at_line (buf, &l1, 0);
	l2 = l1;
	gtk_text_iter_forward_to_line_end (&l2);
	text = gtk_text_buffer_get_text (buf, &l1, &l2, FALSE);
	gtk_entry_set_text (ent, text);
	g_free (text);
	gtk_text_iter_forward_char (&l2);
	gtk_text_buffer_delete (buf, &l1, &l2);
}

void give_top_line (GtkTextBuffer * buf, GtkEntry * ent) {
	GtkTextIter l;
	const gchar * text;

	text = gtk_entry_get_text (ent);
	gtk_text_buffer_get_start_iter (buf, &l);
	gtk_text_buffer_insert (buf, &l, text, -1);
	gtk_text_buffer_insert (buf, &l, "\n", -1);
}

void on_tools_lt_passing_save (GtkButton * button, gpointer user_data) {
	GtkWindow * window;
	GtkTextView * tv;
	GtkTextBuffer * buf;
	GtkEntry * e_pref, * e_suf;


	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	TRY_ASSIGN (buf, gtk_text_view_get_buffer (tv));
	TRY_ASSIGN (e_pref, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_tools_lt_passing_pref")));
	TRY_ASSIGN (e_suf, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_tools_lt_passing_suf")));

	give_top_line (buf, e_suf);
	give_top_line (buf, e_pref);

	on_tools_common_save (button, user_data);

	take_top_line (buf, e_pref);
	take_top_line (buf, e_suf);
}

void on_tools_lt_passing_open (GtkButton * button, gpointer user_data) {
	GtkWindow * window;
	GtkTextView * tv;
	GtkTextBuffer * buf;
	GtkEntry * e_pref, * e_suf;

	on_tools_common_open (button, user_data);
	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	TRY_ASSIGN (buf, gtk_text_view_get_buffer (tv));
	TRY_ASSIGN (e_pref, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_tools_lt_passing_pref")));
	TRY_ASSIGN (e_suf, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_tools_lt_passing_suf")));
	take_top_line (buf, e_pref);
	take_top_line (buf, e_suf);
}

void on_tools_lt_passing_button_clear (GtkButton * button, gpointer user_data) {
	GtkTextView * tv;
	GtkEntry * e_pref, * e_suf;
	GtkWindow * window;
	
	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	TRY_ASSIGN (e_pref, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_tools_lt_passing_pref")));
	TRY_ASSIGN (e_suf, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_tools_lt_passing_suf")));
	gtk_text_buffer_set_text (gtk_text_view_get_buffer (tv), "", 0);
	gtk_entry_set_text (e_pref, "");
	gtk_entry_set_text (e_suf, "");
}

void on_lt_passing_activate (GtkMenuItem * menuitem, gpointer user_data) {
	GtkWindow * window;
	GtkTextView * tv;

	TRY_ASSIGN (window, GTK_WINDOW (interface_create_object_by_name ("window_tools_lt_passing")));
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	gtk_widget_grab_focus (GTK_WIDGET (tv));
}

void on_lt_passing_button_ok (GtkButton * button, gpointer user_data) {
	GtkWindow * window;
	GtkTextView * tv;
	GtkTextBuffer * buf;
	gchar * text;
	GtkTextIter l1, l2;
	gint n, i;
	gboolean echo, prev_echo;
	GtkToggleButton * cbe;
	GtkEntry * e_pref, * e_suf;
	const gchar * pref, * suf;
	SESSION_STATE * session;
	
	session = interface_get_active_session ();
	g_return_if_fail (NULL != session);
	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_text")));
	TRY_ASSIGN (buf, gtk_text_view_get_buffer (tv));
	TRY_ASSIGN (cbe, GTK_TOGGLE_BUTTON (interface_get_widget (GTK_WIDGET (window), "cb_tools_lt_passing_echo")));
	TRY_ASSIGN (e_pref, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_tools_lt_passing_pref")));
	TRY_ASSIGN (e_suf, GTK_ENTRY (interface_get_widget (GTK_WIDGET (window), "entry_tools_lt_passing_suf")));
	pref = gtk_entry_get_text (e_pref);
	suf = gtk_entry_get_text (e_suf);
	echo = gtk_toggle_button_get_active (cbe);
	
	prev_echo = session->local_echo;
	session->local_echo = echo;
	n = gtk_text_buffer_get_line_count (buf);
	for (i = 0; i < n; i++) {
		char * send;
		gtk_text_buffer_get_iter_at_line (buf, &l1, i);
		l2 = l1;
		gtk_text_iter_forward_to_line_end (&l2);
		text = gtk_text_buffer_get_text (buf, &l1, &l2, FALSE);
		send = g_strjoin (" ", pref, text, suf, NULL);
		send_command (session, send, strlen (send));
		g_free (text);
		g_free (send);
	}

	session->local_echo = prev_echo;
}

/*
	log viewer
*/

void destroy_log_view (GtkWidget * w, log_entry * e) {
	GtkWidget * top = gtk_widget_get_toplevel (w);
	gtk_widget_destroy (GTK_WIDGET (top));
	if (w == top) {
		e->repeat = FALSE;
	}
}

void on_tools_log_view_save (GtkWidget * w, log_entry * e) {
	GtkWidget * dialog;
	GtkWindow * window;
	const gchar * c_fname;

	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (w)));
	dialog = gtk_file_chooser_dialog_new ("Save File",
		window,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	NULL);
#ifdef GTK_2_8_9_PLUS
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
#endif
	c_fname = "";
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), c_fname);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		gchar * fc;
		GError * err = NULL;
		GtkTextView * tv;
		GtkTextBuffer * buf;
		GtkTextIter start, end;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		TRY_ASSIGN (tv, GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (window), "textview_tools_log_view")));
		TRY_ASSIGN (buf, gtk_text_view_get_buffer (tv));
		gtk_text_buffer_get_start_iter (buf, &start);
		gtk_text_buffer_get_end_iter (buf, &end);
		fc = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
		if (!g_file_set_contents (filename, fc, -1, &err)) {
			GtkMessageDialog * md;

			md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				(gchar *) "%s",
				err->message
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
		}
		g_free (fc);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

gboolean refresh_log_view_source (log_entry * e) {
	SESSION_STATE * s;
	gsize fs;
	gchar * fc;
	GError * err = NULL;

	if (e->repeat) {
		// check the session still valid
		s = g_list_find (config->sessions, e->session) ? e->session : NULL;
		if (!s) e->session = NULL;
	
		if (s && s->logging) {
			// suspend logging
			fclose (s->log_file);
		}
		if (g_file_get_contents (e->file_name, &fc, &fs, &err)) {
			GtkTextView * tv;
			GtkTextBuffer * buf;
			GtkTextIter end;
	
			tv = GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (e->window), "textview_tools_log_view"));
			buf = gtk_text_view_get_buffer (tv);
			gtk_text_buffer_set_text (buf, fc, -1);
			gtk_text_buffer_get_end_iter (buf, &end);
			gtk_text_view_scroll_to_iter (tv, &end, 0, TRUE, 0, 1.0);
			g_free (fc);
		} else {
			GtkMessageDialog * md;
	
			md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				(gchar *) "%s",
				err->message
			));
			gtk_dialog_run (GTK_DIALOG (md));
			gtk_widget_destroy (GTK_WIDGET (md));
		}
	
		if (s && s->logging) {
			// resume logging
			s->log_file = fopen (e->file_name, "a");
		}
	} else {
		g_free (e->file_name);
		g_free (e);
	}
	return e->repeat;
}

void on_log_view_activate (GtkMenuItem * menuitem, gpointer user_data) {
	GtkWindow * window;
	log_entry * e;
	SESSION_STATE * session;

	session = interface_get_active_session ();
	g_return_if_fail (NULL != session);
	g_return_if_fail (NULL != session->slot);

	e = g_new (log_entry, 1);
	e->session = session;
	e->file_name = g_build_path (G_DIR_SEPARATOR_S, session->slot, "log.txt", NULL);
	if (g_file_test (e->file_name, G_FILE_TEST_EXISTS)) {
		TRY_ASSIGN (window, GTK_WINDOW (interface_create_object_by_name ("window_tools_view_log")));
		g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy_log_view), e);
		g_signal_connect (G_OBJECT (interface_get_widget (GTK_WIDGET (window), "button_tools_log_view_close")), 
				"clicked", G_CALLBACK (destroy_log_view), e);
		g_signal_connect (G_OBJECT (interface_get_widget (GTK_WIDGET (window), "button_tools_log_view_save")), 
				"clicked", G_CALLBACK (on_tools_log_view_save), e);
		e->window = window;
		e->repeat = TRUE;
		refresh_log_view_source (e);
		g_timeout_add (1000, (GSourceFunc) refresh_log_view_source, (gpointer) e);
	} else {
		GtkMessageDialog * md;

		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "No Log file exists yet. Select 'Profile->Logging' from main menu to log mesages first."
		));
		gtk_dialog_run (GTK_DIALOG (md));
		gtk_widget_destroy (GTK_WIDGET (md));
		g_free (e->file_name);
		g_free (e);
	}
}

/*
	delayed commands
*/

void on_tools_delayed_commands_selection_changed (GtkTreeSelection * sel, gpointer user_data) {
	GtkWidget * top;
	GtkWidget * b_pause, * b_resume;
	GtkTreeView * tv = gtk_tree_selection_get_tree_view (sel);

	top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (tv)));
	TRY_ASSIGN (b_pause, interface_get_widget (GTK_WIDGET (top), "button_tools_delayed_commands_pause"));
	TRY_ASSIGN (b_resume, interface_get_widget (GTK_WIDGET (top), "button_tools_delayed_commands_resume"));
	if (sel) {
		GtkTreeModel * model;
		GList * rows;
		gboolean paused = FALSE, running = FALSE;
		GList * i;

		model = gtk_tree_view_get_model (tv);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		for (i = g_list_first (rows); i; i = g_list_next (i)) {
			delayed_cmd * c;
			GtkTreeIter iter;

			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) i->data);
			gtk_tree_model_get (model, &iter, 3, &c, -1);
			paused = paused || c->paused;
			running = running || !c->paused;
		}
		if (paused) {
			gtk_widget_set_sensitive (b_resume, TRUE);
		} else {
			gtk_widget_set_sensitive (b_resume, FALSE);
		}
		if (running) {
			gtk_widget_set_sensitive (b_pause, TRUE);
		} else {
			gtk_widget_set_sensitive (b_pause, FALSE);
		}
		g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (rows);
	} else {
		g_printf ("no selection\n");
	}
}

void delayed_cmd_log_event (delayed_cmd * c, SESSION_STATE * s) {
	char t [32];
	gulong ms, e;
	time_t now = time (NULL);
	
	strftime (t, 32, "%Y-%m-%d %H:%M:%S", localtime (&now));
	e = (gulong) g_timer_elapsed (c->timer, &ms);
}

gboolean on_tools_delayed_commands_timer (delayed_cmd * cmd) {
	gboolean r = cmd->repeat;
	SESSION_STATE * s;

	s = g_list_find (config->sessions, cmd->session) ? cmd->session : NULL;
	if (!s) cmd->stop = TRUE;

	if (cmd->stop) {
		g_free (cmd->command);
		g_timer_destroy (cmd->timer);
		r = FALSE;
		if (s) s->timers = g_list_remove (s->timers, cmd);
		g_free (cmd);
	} else if (cmd->paused) {
		g_timer_destroy (cmd->timer);
		r = FALSE;
	} else {
#ifdef DELAYED_CMD_LOG
		delayed_cmd_log_event (cmd, s);
#endif
		g_timer_start (cmd->timer);
		send_command (s, cmd->command, strlen (cmd->command));
	}
	return r;
}

void tools_delayed_command_apply (delayed_cmd * cmd) {
	cmd->timer = g_timer_new ();
	g_timer_start (cmd->timer);
	g_timeout_add (cmd->interval * 1000, (GSourceFunc) on_tools_delayed_commands_timer, cmd);
}

void tools_delayed_commands_show (GtkWindow * w, gpointer data) {
	GList * i;
	GtkTreeView * tv;
	GtkListStore * store;
	GtkTreeIter iter;
	time_t now = time (NULL);
	const char * units [3] = {"s", "m", "h"};
	SESSION_STATE * session;

	session = interface_get_active_session ();
	g_return_if_fail (NULL != session);

	TRY_ASSIGN (tv, GTK_TREE_VIEW (interface_get_widget (GTK_WIDGET (w), "treeview_tools_delayed_commands")));
	store = (GtkListStore *) gtk_tree_view_get_model (tv);
	gtk_list_store_clear (store);

	for (i = g_list_first (session->timers); i; i = g_list_next (i)) {
		delayed_cmd * c = (delayed_cmd *) i->data;

		if (!c->stop) {
			char buf [64], intvl [64];
			gulong ms;
			time_t v;
			gulong i_val = c->interval;
			gint i_un = 0;

			if (c->repeat) {
				if (! (i_val % 60)) {
					i_val /= 60;
					i_un ++;
					if (! (i_val % 60)) {
						i_val /= 60;
						i_un ++;
					}
				}
				g_snprintf (intvl, 64, "%i%s", (int) i_val, units [i_un]);
			} else {
				intvl [0] = 0;
			}
			if (c->paused) {
				g_snprintf (buf, 64, "Paused");
			} else {
				v = now + c->interval - (glong) g_timer_elapsed (c->timer, &ms);
				strftime (buf, 64, "%T", localtime (&v));
			}
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, c->command, 1, buf, 2, intvl, 3, c, -1);
		}
	}
}

void on_tools_delayed_commands_add (GtkButton * button, gpointer data) {
	GtkDialog * cmd;
	GtkComboBox * cb;
	SESSION_STATE * session;

	TRY_ASSIGN (cmd, GTK_DIALOG (interface_create_object_by_name ("dialog_tools_delayed_command_new")));
	TRY_ASSIGN (cb, GTK_COMBO_BOX (interface_get_widget (GTK_WIDGET (cmd), "combobox_units")));
	gtk_combo_box_set_active (cb, 1);
	if (gtk_dialog_run (cmd) == GTK_RESPONSE_OK) {
		GtkWindow * window;
		const gchar * c = gtk_entry_get_text (GTK_ENTRY (interface_get_widget (GTK_WIDGET (cmd), "entry_command")));
		gint i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (interface_get_widget (GTK_WIDGET (cmd), "spinbutton_interval")));
		gint u = gtk_combo_box_get_active (GTK_COMBO_BOX (interface_get_widget (GTK_WIDGET (cmd), "combobox_units")));
		gboolean r = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (interface_get_widget (GTK_WIDGET (cmd), "checkbutton_repeat")));
		delayed_cmd * cmd;

		window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
		session = interface_get_active_session ();
		cmd = g_new (delayed_cmd, 1);
		cmd->command = g_strdup (c);
		cmd->interval = (0 == u) ? i : ((1 == u) ? 60 * i : 3600 * i);
		cmd->paused = FALSE;
		cmd->stop = FALSE;
		cmd->repeat = r;
		cmd->session = session;
		session->timers = g_list_append (session->timers, cmd);
		tools_delayed_command_apply (cmd);
		tools_delayed_commands_show (window, NULL);
	}
	gtk_widget_destroy (GTK_WIDGET (cmd));
}

void on_tools_delayed_commands_del (GtkButton * button, gpointer data) {
	GtkWindow * window;
	GtkTreeView * tv;
	GtkTreeSelection * sel;
	GtkTreeModel * model;
	GList * rows, * i;
	GtkTreeIter iter;
	GtkMessageDialog * md;
	
	window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (tv, GTK_TREE_VIEW (interface_get_widget (GTK_WIDGET (window), "treeview_tools_delayed_commands")));
	sel = gtk_tree_view_get_selection (tv);
	if (sel) {
		model = gtk_tree_view_get_model (tv);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			(gchar *) "You are going to remove %i delayed commands, are you sure?",
			gtk_tree_selection_count_selected_rows (sel)
		));
		if (GTK_RESPONSE_YES == gtk_dialog_run (GTK_DIALOG (md))) {
			for (i = g_list_first (rows); i; i = g_list_next (i)) {
				delayed_cmd * c;
	
				gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) i->data);
				gtk_tree_model_get (model, &iter, 3, &c, -1);
				c->stop = TRUE;
			}
			tools_delayed_commands_show (window, NULL);
		}
		g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (rows);
	} else {
		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "No command selected. Select command to delete first."
		));
		gtk_dialog_run (GTK_DIALOG (md));
	}
	gtk_widget_destroy (GTK_WIDGET (md));
}

void on_tools_delayed_commands_pause (GtkButton * button, gpointer data) {
	GtkTreeSelection * sel;
	GtkWidget * top;
	GtkTreeView * tv;

	top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	TRY_ASSIGN (tv, GTK_TREE_VIEW (interface_get_widget (GTK_WIDGET (top), "treeview_tools_delayed_commands")));
	sel = gtk_tree_view_get_selection (tv);
	if (sel) {
		GtkTreeModel * model;
		GList * rows;
		GList * i;
		char buf [64];
		GtkListStore * store;
		GtkWidget * b_pause, * b_resume;

		TRY_ASSIGN (b_pause, interface_get_widget (GTK_WIDGET (top), "button_tools_delayed_commands_pause"));
		TRY_ASSIGN (b_resume, interface_get_widget (GTK_WIDGET (top), "button_tools_delayed_commands_resume"));
		model = gtk_tree_view_get_model (tv);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		store = (GtkListStore *) gtk_tree_view_get_model (tv);
		for (i = g_list_first (rows); i; i = g_list_next (i)) {
			delayed_cmd * c;
			GtkTreeIter iter;

			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) i->data);
			gtk_tree_model_get (model, &iter, 3, &c, -1);
			if (data) {
				// resume button clicked
				time_t v;
				time_t now = time (NULL);

				c->paused = FALSE;
				v = now + c->interval;
				strftime (buf, 64, "%T", localtime (&v));
				tools_delayed_command_apply (c);
			} else {
				// pause button clicked
				c->paused = TRUE;
				g_snprintf (buf, 64, "Paused");
			}

			gtk_list_store_set (store, &iter, 1, buf, -1);

		}
		if (data) {
			gtk_widget_set_sensitive (b_pause, TRUE);
			gtk_widget_set_sensitive (b_resume, FALSE);
		} else {
			gtk_widget_set_sensitive (b_pause, FALSE);
			gtk_widget_set_sensitive (b_resume, TRUE);
		}
		g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (rows);
	} else {
		g_printf ("no selection\n");
	}
}

void on_delayed_cmd_activate (GtkMenuItem * menuitem, gpointer user_data) {
	GtkWindow * window;
	GtkTreeView * tv;
	GtkTreeViewColumn * c;
	GtkListStore * store;
	GtkWidget * b_pause, * b_resume;

	GtkCellRenderer * r = gtk_cell_renderer_text_new ();

	TRY_ASSIGN (window, GTK_WINDOW (interface_create_object_by_name ("window_tools_delayed_commands")));
	TRY_ASSIGN (tv, GTK_TREE_VIEW (interface_get_widget (GTK_WIDGET (window), "treeview_tools_delayed_commands")));
	TRY_ASSIGN (b_pause, interface_get_widget (GTK_WIDGET (window), "button_tools_delayed_commands_pause"));
	TRY_ASSIGN (b_resume, interface_get_widget (GTK_WIDGET (window), "button_tools_delayed_commands_resume"));

	gtk_widget_set_sensitive(b_pause, FALSE);
	gtk_widget_set_sensitive(b_resume, FALSE);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (tv), GTK_SELECTION_MULTIPLE);

	store = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model (tv, GTK_TREE_MODEL (store));

	c = gtk_tree_view_column_new_with_attributes ("command", r, "text", 0, NULL);
	gtk_tree_view_append_column (tv, c);
	
	c = gtk_tree_view_column_new_with_attributes ("run at", r, "text", 1, NULL);
	gtk_tree_view_append_column (tv, c);
	
	c = gtk_tree_view_column_new_with_attributes ("interval", r, "text", 2, NULL);
	gtk_tree_view_append_column (tv, c);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (tv)), "changed", G_CALLBACK (on_tools_delayed_commands_selection_changed), NULL);
	g_signal_connect (G_OBJECT (b_pause), "clicked", G_CALLBACK (on_tools_delayed_commands_pause), NULL);
	g_signal_connect (G_OBJECT (b_resume), "clicked", G_CALLBACK (on_tools_delayed_commands_pause), (gpointer) 1);
	tools_delayed_commands_show (window, NULL);
}

/*
	export/import settings
*/

GList * remote_storage_concatenate_lists (GList * locals, GList * remotes) {
	GList * ret = NULL;
	GList * j;
	gboolean found;
	SavedGameInfo * l, * r = NULL;

	while (locals) {
		l = (SavedGameInfo *) locals->data;
		found = FALSE;
		for (j = g_list_first (remotes); j && !found; j = g_list_next (j)) {
			r = (SavedGameInfo *) j->data;
			found = !(g_ascii_strcasecmp (l->name, r->name) || g_ascii_strcasecmp (l->mud, r->mud));
		}
		locals = g_list_remove (locals, l);
		if (found) {
			r->local = TRUE;
			r->remote = TRUE;
			remotes = g_list_remove (remotes, r);
			ret = g_list_append (ret, r);
			g_free (l);
		} else {
			l->local = TRUE;
			l->remote = FALSE;
			ret = g_list_append (ret, l);
		}
	}
	while (remotes) {
		r = (SavedGameInfo *) remotes->data;
		r->local = FALSE;
		r->remote = TRUE;
		remotes = g_list_remove (remotes, r);
		ret = g_list_append (ret, r);
	}
	return g_list_first (ret);
}

GList * remote_storage_read_local_game_list () {
	GList * games = NULL;
	GDir * dir;
	gchar * t, * name, * game_name, * entry;
	gboolean exist;

	if (g_file_test (config->savedir, G_FILE_TEST_IS_DIR) && (dir = g_dir_open (config->savedir, 0, NULL))) {
		while ((entry = (gchar *) g_dir_read_name (dir))) {
			t = g_build_path (G_DIR_SEPARATOR_S, config->savedir, entry, NULL);
			exist = session_saved_get_name (t, &name, &game_name, NULL);
			if (exist && (name || game_name)) {
				SavedGameInfo * g = g_new0 (SavedGameInfo, 1);

				if (name) g->name = name;
				else g->name = g_strdup ("N/A");
				if (game_name) g->mud = game_name;
				else g->mud = g_strdup ("N/A");
				g->saved = (time_t) 0;
				games = g_list_append (games, g);
			}
			g_free(t);
			name = NULL;
			game_name = NULL;
		}
	}
	return games;
}

struct _read_game_list_thread_data {
	char * acct_user;
	char * acct_passwd;
	Proxy * proxy;
	GAsyncQueue * queue;
	GtkWidget * d;
	GtkWidget * pb;
	GTimer * timer;
	gboolean cancelled;
	gboolean destroyed;
};

struct _read_game_list_harvested_data {
	char * err;
	GList * games;
};

struct _remote_storage_action {
	char * conf_message;
	char * (* action) (CURL *, char *, char *, char *);
	char * actname;
	char must_close;
};

struct _rs_perform_thread_data {
	GList * games;
	char * (* action) (CURL *, char *, char *, char *);
	GAsyncQueue * queue;
	Proxy * proxy;
	GtkDialog * status_dlg;
	GtkWidget * list_view;
	gboolean progress_destroyed;
	gboolean list_view_destroyed;
	gboolean done;
	char * actname;
	char * acct_user;
	char * acct_passwd;
};

struct _rs_perform_thread_status {
	char * game;
	char * mud;
	char * status;
	char * actname;
	char * slot;
	gboolean done;
};

struct _game_defn {
	char * game;
	char * mud;
	char * slot;
};

typedef struct _remote_storage_action RemoteStorageAction;
typedef struct _read_game_list_thread_data ReadGameListThreadData;
typedef struct _read_game_list_harvested_data ReadGameListHarvestedData;
typedef struct _rs_perform_thread_data RSPerformThreadData;
typedef struct _rs_perform_thread_status RSPerformThreadStatus;
typedef struct _game_defn GameDefn;

gpointer tools_remote_storage_get_remote_games_list_thread (gpointer data) {
	ReadGameListThreadData * td = (ReadGameListThreadData *) data;
	ReadGameListHarvestedData * h = g_new (ReadGameListHarvestedData, 1);
	Proxy * proxy = proxy_get_default (config->proxies);
	long code;
	DownloadedData * dd;

	h->games = NULL;
	h->err = NULL;
	g_async_queue_ref (td->queue);
	code = proxy_download_url (NULL, proxy, RS_GET_SAVES_URL, td->acct_user, td->acct_passwd, NULL, &dd);
	if (-1 == code) {
		h->err = "Connection to remote storage failed.";
	} else {
		if (!(td->cancelled || td->destroyed)) {
			if (200 == code) {
				xmlDocPtr doc;

				dd->buf = g_realloc (dd->buf, dd->size + 1);
				dd->buf [dd->size] = 0;
				// parsing xml input
				doc = xmlReadMemory (dd->buf, dd->size, "noname.xml", NULL, 0);
				if (doc) {
					xmlNodePtr root = xmlDocGetRootElement (doc);
					if (root && (XML_ELEMENT_NODE == root->type) && !g_ascii_strcasecmp ((gchar *) root->name, "saved-games")) {
						xmlNodePtr cur_node = NULL;
						for (cur_node = root->children; cur_node && ! h->err; cur_node = cur_node->next) {
							if (XML_ELEMENT_NODE == cur_node->type) {
								xmlChar * name, * saved, * mud;
								SavedGameInfo * g = g_new0 (SavedGameInfo, 1);
	
								g->name = NULL;
								g->mud = NULL;
								name = xmlGetProp (cur_node, (xmlChar *) "name");
								saved = xmlGetProp (cur_node, (xmlChar *) "saving_date");
								mud = xmlGetProp (cur_node, (xmlChar *) "mud_name");
								if (name) g->name = g_strdup ((gchar *) name);
								else h->err = "Server returns malformed saved games list.";
								if (mud) g->mud = g_strdup ((gchar *) mud);
								else h->err = "Server returns malformed saved games list.";
								if (saved) {
									if (((time_t) -1) == (g->saved = curl_getdate ((char *) saved, NULL))) h->err = "date conversion error";
								} else h->err = "Server returns malformed saved games list.";
								xmlFree (name);
								xmlFree (saved);
								xmlFree (mud);
								if (!h->err) h->games = g_list_append (h->games, g);
								else g_free (g);
							}
						}
					} else {
						h->err = "Server returns malformed saved games list.";
					}
					xmlFreeDoc (doc);
				} else {
					h->err = "Server returns malformed saved games list.";
				}
				xmlCleanupParser ();
			} else {
				if (401 == code) h->err = "Unauthorized";
				else h->err = "Unable to retrive saved games list from server.";
			}
		}
		g_free (dd->buf);
		g_free (dd);
	}
	if (h->err) {
		while (h->games) {
			SavedGameInfo * g = (SavedGameInfo *) h->games;
			g_free (g->name);
			g_free (g->mud);
			h->games = g_list_remove (h->games, g);
			g_free (g);
		}
	}
	g_async_queue_push (td->queue, (gpointer) h);
	g_async_queue_unref (td->queue);
	return NULL;
}

gboolean tools_remote_storage_get_acct_info (char ** user, char ** passwd) {
	gboolean r;
	GtkDialog * d = GTK_DIALOG (interface_create_object_by_name ("dialog_acct_settings"));
	GtkEntry * e_user = GTK_ENTRY (interface_get_widget (GTK_WIDGET (d), "entry_mudmagic_user"));
	GtkEntry * e_passwd = GTK_ENTRY (interface_get_widget (GTK_WIDGET (d), "entry_mudmagic_passwd"));

	if (*user) gtk_entry_set_text (e_user, *user);
	if (*passwd) gtk_entry_set_text (e_passwd, *passwd);
	if (!(r = (GTK_RESPONSE_OK != gtk_dialog_run (d)))) {
		if (*user) g_free (*user);
		if (*passwd) g_free (*passwd);
		* user = g_strdup (gtk_entry_get_text (e_user));
		* passwd = g_strdup (gtk_entry_get_text (e_passwd));
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (interface_get_widget (GTK_WIDGET (d), "check_keep_info")))) {
			if (config->acct_user) g_free (config->acct_user);
			if (config->acct_passwd) g_free (config->acct_passwd);
			config->acct_user = g_strdup (* user);
			config->acct_passwd = g_strdup (* passwd);
		}
	}
	gtk_widget_destroy (GTK_WIDGET (d));

	return r;
}

void on_tools_remote_storage_selection_changed (GtkTreeSelection * sel, gpointer user_data) {
	GtkWidget * top;
	GtkWidget * b_export, * b_import, * b_erase;
	GtkTreeView * tv = gtk_tree_selection_get_tree_view (sel);

	top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (tv)));
	TRY_ASSIGN (b_erase, interface_get_widget (GTK_WIDGET (top), "toolbutton_remove"));
	TRY_ASSIGN (b_export, interface_get_widget (GTK_WIDGET (top), "toolbutton_export"));
	TRY_ASSIGN (b_import, interface_get_widget (GTK_WIDGET (top), "toolbutton_import"));
	if (sel) {
		GtkTreeModel * model;
		GList * rows;
		gboolean local = FALSE, remote = FALSE;
		GList * i;

		model = gtk_tree_view_get_model (tv);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		for (i = g_list_first (rows); i; i = g_list_next (i)) {
			SavedGameInfo * g;
			GtkTreeIter iter;

			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) i->data);
			gtk_tree_model_get (model, &iter, 4, &g, -1);
			local = local || g->local;
			remote = remote || g->remote;
		}
		if (local) {
			gtk_widget_set_sensitive (b_export, TRUE);
		} else {
			gtk_widget_set_sensitive (b_export, FALSE);
		}
		if (remote) {
			gtk_widget_set_sensitive (b_import, TRUE);
			gtk_widget_set_sensitive (b_erase, TRUE);
		} else {
			gtk_widget_set_sensitive (b_import, FALSE);
			gtk_widget_set_sensitive (b_erase, FALSE);
		}
		g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (rows);
	} else {
		g_printf ("no selection\n");
	}
}

#define SET_STRING_ITEM(A) \
	xmlChar * content = xmlNodeGetContent (n); \
	if (s->A) g_free (s->A); \
	s->A = g_strdup ((gchar *)content); \
	xmlFree (content); \
	return NULL;

#define SET_INT_ITEM(A) \
	xmlChar * content = xmlNodeGetContent (n); \
	s->A = atoi ((const char *) content); \
	xmlFree (content); \
	return NULL;

char * sr_session_item_host (xmlNodePtr n, SESSION_STATE * s) { SET_STRING_ITEM (game_host); }
char * sr_session_item_port (xmlNodePtr n, SESSION_STATE * s) { SET_INT_ITEM (game_port); }
char * sr_session_item_font (xmlNodePtr n, SESSION_STATE * s) { SET_STRING_ITEM (font); }
char * sr_session_item_bg_color (xmlNodePtr n, SESSION_STATE * s) { SET_STRING_ITEM (bg_color); }
char * sr_session_item_fg_color (xmlNodePtr n, SESSION_STATE * s) { SET_STRING_ITEM (fg_color); }
char * sr_session_item_ufg_color (xmlNodePtr n, SESSION_STATE * s) { SET_STRING_ITEM (ufg_color); }
char * sr_session_item_single_line (xmlNodePtr n, SESSION_STATE * s) { SET_INT_ITEM (single_line); }
char * sr_session_item_local_echo (xmlNodePtr n, SESSION_STATE * s) { SET_INT_ITEM (local_echo); }
char * sr_session_item_logging (xmlNodePtr n, SESSION_STATE * s) { SET_INT_ITEM (logging); }
char * sr_session_item_sound (xmlNodePtr n, SESSION_STATE * s) { SET_INT_ITEM (sound); }
char * sr_session_item_proxy (xmlNodePtr n, SESSION_STATE * s) { SET_STRING_ITEM (proxy); }

char * rs_session_item_atm (xmlNodePtr n, GList ** atms, SESSION_STATE * s) {
	ATM * atm, * prev = NULL;
	GList * i;
	xmlChar * c;
	xmlNodePtr ch;

	for (ch = n->children; ch; ch = ch->next) if (XML_ELEMENT_NODE == ch->type) {
		int type, action, disabled, lang = -1, found;
		char * name, * text = NULL, * source = NULL, * raiser;

		atm = atm_new ();
		atm->config = config;
		atm->session = s;
		c = xmlGetProp (ch, (xmlChar *) "type"); 
		type = atoi(c); 
		xmlFree (c);
		c = xmlGetProp (ch, (xmlChar *) "action"); 
		action = atoi(c); 
		xmlFree (c);
		c = xmlGetProp (ch, (xmlChar *) "disabled"); 
		disabled = atoi(c); 
		xmlFree (c);

		name = xmlGetProp (ch, "name");
		raiser = xmlGetProp (ch, "raiser");

		if (ATM_ACTION_SCRIPT == action) {
			c = xmlGetProp (ch, (xmlChar *) "lang"); 
			lang = atoi(c); 
			xmlFree (c);
			text = xmlNodeGetContent (ch);
		} else {
			source = xmlGetProp (ch, "source");
		}
		atm_init (atm, type, name, text, lang, source, raiser, action, disabled);
		xmlFree (name);
		if (text) xmlFree (text);
		if (source) xmlFree (source);
		xmlFree (raiser);
		found = 0;
		for (i = g_list_first (* atms); i && !found; i = g_list_next (i)) {
			prev = (ATM *) (i->data);
			found = !g_ascii_strcasecmp (atm->name, prev->name);
		}
		if (found) {
			* atms = g_list_first (g_list_remove (g_list_first (* atms), prev));
			atm_free (prev);
		}
		* atms = g_list_append (* atms, atm);
	}
	return NULL;
}

char * sr_session_item_macroses (xmlNodePtr n, SESSION_STATE * s) { return rs_session_item_atm (n, &s->macros, s); }
char * sr_session_item_aliases (xmlNodePtr n, SESSION_STATE * s) { return rs_session_item_atm (n, &s->aliases, s); }
char * sr_session_item_triggers (xmlNodePtr n, SESSION_STATE * s) { return rs_session_item_atm (n, &s->triggers, s); }

char * sr_session_item_delayed_commands (xmlNodePtr n, SESSION_STATE * s) {
	xmlNodePtr ch;
	delayed_cmd * c, * prev = NULL;
	GList * i;

	for (ch = n->children; ch; ch = ch->next) if (XML_ELEMENT_NODE == ch->type) {
		xmlChar * x;

		c = g_new (delayed_cmd, 1);
		c->stop = FALSE;
		c->repeat = TRUE;
		c->paused = TRUE;
		c->session = s;
		x = xmlGetProp (ch, "command");
		c->command = g_strdup (x);
		xmlFree (x);
		x = xmlGetProp (ch, "interval");
		c->interval = atoi (x);
		xmlFree (x);
		for (i = g_list_first (s->timers); i && !prev; i = g_list_next (i)) {
			delayed_cmd * a = (delayed_cmd *) i->data;

			if ((c->interval == a->interval) && !g_ascii_strcasecmp (a->command, c->command)) prev = a;
		}
		if (prev) {
			g_free (c->command);
			g_free (c);
		} else s->timers = g_list_append (s->timers, c);
	}
	return NULL;
}

char * sr_session_item_variables (xmlNodePtr n, SESSION_STATE * s) {
	xmlNodePtr ch;

	varlist_destroy (s->variables);
	s->variables = varlist_new (s);
	for (ch = n->children; ch; ch = ch->next) if (XML_ELEMENT_NODE == ch->type) {
		xmlChar * k, * v;

		k = xmlGetProp (ch, "key");
		v = xmlGetProp (ch, "val");
		varlist_set_value (s->variables, k, v);
		xmlFree (k);
		xmlFree (v);
	}	
	return NULL;
}

char * sr_session_item_statusvars (xmlNodePtr n, SESSION_STATE * s) {
	xmlNodePtr ch;

	svlist_destroy (s->svlist);
	s->svlist = svlist_new (s);
	s->svlist->loading = TRUE;
	for (ch = n->children; ch; ch = ch->next) if (XML_ELEMENT_NODE == ch->type) {
		xmlChar * var, * maxval, * percentage;

		var = xmlGetProp (ch, "var");
		maxval = xmlGetProp (ch, "maxval");
		percentage = xmlGetProp (ch, "percentage");
		svlist_set_statusvar (s->svlist, var, maxval, atoi (percentage));
		xmlFree (var);
		xmlFree (maxval);
		xmlFree (percentage);
	}	
	return NULL;
}

char * sr_session_item_gauges (xmlNodePtr n, SESSION_STATE * s) {
	xmlNodePtr ch;

	gaugelist_destroy (s->gaugelist);
	s->gaugelist = gaugelist_new (s);
	for (ch = n->children; ch; ch = ch->next) if (XML_ELEMENT_NODE == ch->type) {
		xmlChar * var, * maxval, * col_red, * col_green, * col_blue;
	    GdkColor color;
    	color.pixel = 0;

		var = xmlGetProp (ch, "var");
		maxval = xmlGetProp (ch, "maxval");
		col_red = xmlGetProp (ch, "col_red");
		col_green = xmlGetProp (ch, "col_green");
		col_blue = xmlGetProp (ch, "col_blue");
		color.red = atoi (col_red);
		color.green = atoi (col_green);
		color.blue = atoi (col_blue);
		gaugelist_set_gauge (s->gaugelist, var, maxval, color);
		xmlFree (var);
		xmlFree (maxval);
		xmlFree (col_red);
		xmlFree (col_green);
		xmlFree (col_blue);
	}	
	return NULL;
}

char * sr_session_item_owindows (xmlNodePtr n, SESSION_STATE * s) {
	xmlNodePtr ch;

	owindowlist_destroy (s->windowlist);
	s->windowlist = owindowlist_new (s);
	for (ch = n->children; ch; ch = ch->next) if (XML_ELEMENT_NODE == ch->type) {
		xmlChar * name, * title, * top, * left, * width, * height;

		name = xmlGetProp (ch, "name");
		title = xmlGetProp (ch, "title");
		top = xmlGetProp (ch, "top");
		left = xmlGetProp (ch, "left");
		width = xmlGetProp (ch, "width");
		height = xmlGetProp (ch, "height");
		owindowlist_set_owindow (
			s->windowlist,
			name,
			title,
			atoi (left),
			atoi (top),
			atoi (width),
			atoi (height)
		);
		xmlFree (name);
		xmlFree (title);
		xmlFree (top);
		xmlFree (left);
		xmlFree (width);
		xmlFree (height);
	}	
	return NULL;
}

struct _rs_import_session_item {
	char * name;
	char * (* action) (xmlNodePtr, SESSION_STATE *);
};

typedef struct _rs_import_session_item RSImportSessionItem;

const int SessionItemsCount = 19;
const  RSImportSessionItem SessionItems [] = {
	{"host", sr_session_item_host},
	{"port", sr_session_item_port},
	{"font", sr_session_item_font},
	{"bg_color", sr_session_item_bg_color},
	{"fg_color", sr_session_item_fg_color},
	{"ufg_color", sr_session_item_ufg_color},
	{"single_line", sr_session_item_single_line},
	{"local_echo", sr_session_item_local_echo},
	{"logging", sr_session_item_logging},
	{"sound", sr_session_item_sound},
	{"proxy", sr_session_item_proxy},
	{"delayed_commands", sr_session_item_delayed_commands},
	{"macroses", sr_session_item_macroses},
	{"aliases", sr_session_item_aliases},
	{"triggers", sr_session_item_triggers},
	{"variables", sr_session_item_variables},
	{"statusvars", sr_session_item_statusvars},
	{"gauges", sr_session_item_gauges},
	{"owindows", sr_session_item_owindows}
};

char * rs_imort_make_session (SESSION_STATE * s, char * buf, int bufsize) {
	xmlDocPtr doc;
	char * ret = NULL;

	doc = xmlReadMemory (buf, bufsize, "noname.xml", NULL, 0);
	if (doc) {
		xmlNodePtr root = xmlDocGetRootElement (doc);

		if (root && (XML_ELEMENT_NODE == root->type) && !g_ascii_strcasecmp (root->name, "game")) {
			xmlNodePtr cur_node = NULL;

			for (cur_node = root->children; cur_node && !ret ; cur_node = cur_node->next) {
				if (XML_ELEMENT_NODE == cur_node->type) {
					int found = 0;
					int i;

					for (i = 0; (i < SessionItemsCount) && !found; i++) {
						found = !g_ascii_strcasecmp (cur_node->name, SessionItems [i].name);
						if (found) ret = SessionItems [i].action (cur_node, s);
					}
					if (!found) {
						fprintf (stderr, "unknown XML node: %s\n", cur_node->name);
						ret = "Invalid XML document";
					}
				}
			}
		} else ret = "Invalid XML document";
	} else return "Invalid XML document";
	xmlFreeDoc (doc);
	return ret;
}

char * tools_remote_storage_action_import (CURL * curl, char * game, char * mud, char * slot) {
	char * ret;
	struct curl_httppost * post = NULL;
	struct curl_httppost * last = NULL;
	char * buf;
	uLong bufsize;
	uLong res;
	DownloadedData * data;
	long code;
	char * engame, * enmud;
	xmlDocPtr doc = xmlNewDoc (BAD_CAST "1.0");

	engame = xmlEncodeEntitiesReentrant (doc, BAD_CAST game);
	enmud = xmlEncodeEntitiesReentrant (doc, BAD_CAST mud);
	curl_formadd (&post, &last, CURLFORM_PTRNAME, "game", CURLFORM_COPYCONTENTS, engame, CURLFORM_END);
	curl_formadd (&post, &last, CURLFORM_PTRNAME, "mud", CURLFORM_COPYCONTENTS, enmud, CURLFORM_END);
	xmlFree (engame);
	xmlFree (enmud);
	xmlFreeDoc (doc);
	code = proxy_download_url (curl, NULL, RS_IMPORT_URL, NULL, NULL, post, &data);
	if (-1 == code) {
		ret = "Connection to remote storage failed.";
	} else {
		if (200 == code) {
			bufsize = 10 * data->size;
			buf = g_malloc (bufsize);
			res = 0;
			do {
				if (Z_BUF_ERROR == res) bufsize *= 2, buf = g_realloc (buf, bufsize);
				res = uncompress (buf, &bufsize, data->buf, data->size);
			} while (Z_BUF_ERROR == res);
			if (Z_OK == res) {
				SESSION_STATE * s = session_new ();

				if (slot) {
					s->slot = g_strdup (slot);
					session_load (s, slot);
				}
				else s->slot = session_get_free_slot (config);
				ret = rs_imort_make_session (s, buf, bufsize);
				if (ret) {
					g_free (s->slot);
					s->slot = NULL;
					session_delete (s);
				} else {
					s->name = g_strdup (game);
					s->game_name = g_strdup (mud);
					session_delete (s);	
					ret = "Success.";
				}
			} else ret = "Uncompression failed";
			g_free (buf);
		} else ret = "Failed.";
		discard_downloaded_data (data);
	}
	curl_formfree (post); 
	return ret;
}

#define XML_ADD_CHILD_STRING(A,B) \
	ch = xmlNewNode (NULL, BAD_CAST A); \
	xmlNodeSetContent (ch, BAD_CAST B); \
	xmlAddChild (n, ch);

#define XML_ADD_CHILD_INT(A,B) \
	g_snprintf (buf, 1024, "%u", B); \
	ch = xmlNewNode (NULL, BAD_CAST A); \
	xmlNodeSetContent (ch, BAD_CAST buf); \
	xmlAddChild (n, ch);

xmlNodePtr rs_export_get_delayed_cmds (SESSION_STATE * session) {
	xmlNodePtr n, ch;
	GList * i;
	char buf [1024];

	n = xmlNewNode (NULL, BAD_CAST "delayed_commands");
	for (i = g_list_first (session->timers); i; i = g_list_next (i)) {
		delayed_cmd * c = (delayed_cmd *) i->data;

		ch = xmlNewNode (NULL, BAD_CAST "delayed_command");
		g_snprintf (buf, 1024, "%u", c->interval);
		xmlSetProp (ch, "command", c->command);
		xmlSetProp (ch, "interval", buf);
		xmlAddChild (n, ch);
	}
	return n;
}

xmlNodePtr rs_export_get_atm (char * name, GList * atms) {
	xmlNodePtr n, ch;
	GList * i;
	char buf [1024];
	int j, m;

	n = xmlNewNode (NULL, BAD_CAST name);
	for (i = g_list_first (atms); i; i = g_list_next (i)) {
		ATM * atm = (ATM *) i->data;

		ch = xmlNewNode (NULL, BAD_CAST "atm");
		xmlSetProp (ch, "name", atm->name);
		g_snprintf (buf, 1024, "%u", atm->action);
		xmlSetProp (ch, "action", buf);
		g_snprintf (buf, 1024, "%u", atm->type);
		xmlSetProp (ch, "type", buf);
		xmlSetProp (ch, "raiser", atm->raiser);
		g_snprintf (buf, 1024, "%u", atm->disabled);
		xmlSetProp (ch, "disabled", buf);
		if (ATM_ACTION_SCRIPT == atm->action) {
			for (m = -1, j = 0; (-1 == m) && (j < ATMLanguageCount); j++)
				if (Languages [j].id == atm->lang) m = j;
			xmlSetProp (ch, "lang", (-1 == m) ? "Unknwon" : Languages [m].name);
			if (!atm->text) atm_load_script (atm);
			xmlNodeSetContent (ch, atm->text);
		} else {
			xmlSetProp (ch, "source", atm->source);
		}
		xmlAddChild (n, ch);
	}
	return n;
}

gboolean rs_export_add_var_entry (gpointer * key, gpointer * value, gpointer * data) {
	gchar * k = (gchar *) key;
	gchar * v = ((VARIABLE *) value)->value;
	xmlNodePtr n, ch;

	n = (xmlNodePtr) data;
	ch = xmlNewNode (NULL, BAD_CAST "variable");
	xmlSetProp (ch, "key", k);
	xmlSetProp (ch, "val", v);
	xmlAddChild (n, ch);
	return FALSE;
}

xmlNodePtr rs_export_get_variables (VARLIST * vars) {
	xmlNodePtr n = xmlNewNode (NULL, BAD_CAST "variables");
	
	g_tree_foreach (vars->tree, (GTraverseFunc) rs_export_add_var_entry, n);
	return n;
}

gboolean rs_export_add_svar_entry (gpointer * data, gpointer * user_data) {
	xmlNodePtr n, ch;
	STATUSVAR * g = ((STATUSVAR *) data);
	gchar buf [1024];

	n = (xmlNodePtr) user_data;
	ch = xmlNewNode (NULL, BAD_CAST "statvariable");
	xmlSetProp (ch, "var", g->variable);
	xmlSetProp (ch, "maxval", g->maxvariable);
	g_snprintf (buf, 1024, "%d", g->percentage ? 1 : 0);
	xmlSetProp (ch, "percentage", buf);
	xmlAddChild (n, ch);
	return FALSE;
}

xmlNodePtr rs_export_get_statvars (SVLIST * vars) {
	xmlNodePtr n = xmlNewNode (NULL, BAD_CAST "statusvars");

	g_list_foreach (vars->list, (GFunc) rs_export_add_svar_entry, n);
	return n;
}

gboolean rs_export_add_gauge_entry (gpointer * data, gpointer * user_data) {
	xmlNodePtr n, ch;
	GAUGE * g = ((GAUGE *) data);
	gchar buf [1024];

	n = (xmlNodePtr) user_data;
	ch = xmlNewNode (NULL, BAD_CAST "gauge");
	xmlSetProp (ch, "var", g->variable);
	xmlSetProp (ch, "maxval", g->maxvariable);
	g_snprintf (buf, 1024, "%d", g->color.red);
	xmlSetProp (ch, "col_red", buf);
	g_snprintf (buf, 1024, "%d", g->color.green);
	xmlSetProp (ch, "col_green", buf);
	g_snprintf (buf, 1024, "%d", g->color.blue);
	xmlSetProp (ch, "col_blue", buf);
	xmlAddChild (n, ch);
	return FALSE;
}

xmlNodePtr rs_export_get_gauges (GAUGELIST * g) {
	xmlNodePtr n = xmlNewNode (NULL, BAD_CAST "gauges");

	g_list_foreach (g->list, (GFunc) rs_export_add_gauge_entry, n);
	return n;
}

gboolean rs_export_add_owindow_entry (gpointer * data, gpointer * user_data) {
	xmlNodePtr n, ch;
	OWINDOW * o = ((OWINDOW *) data);
	gchar buf [1024];

	n = (xmlNodePtr) user_data;
	ch = xmlNewNode (NULL, BAD_CAST "owindow");
	xmlSetProp (ch, "name", o->name);
	xmlSetProp (ch, "title", o->title);
	g_snprintf (buf, 1024, "%d", o->left); xmlSetProp (ch, "left", buf);
	g_snprintf (buf, 1024, "%d", o->top); xmlSetProp (ch, "top", buf);
	g_snprintf (buf, 1024, "%d", o->width); xmlSetProp (ch, "width", buf);
	g_snprintf (buf, 1024, "%d", o->height); xmlSetProp (ch, "height", buf);
	xmlAddChild (n, ch);
	return FALSE;
}

xmlNodePtr rs_export_get_owindows (OWINDOWLIST * ol) {
	xmlNodePtr n = xmlNewNode (NULL, BAD_CAST "owindows");

	g_list_foreach (ol->list, (GFunc) rs_export_add_owindow_entry, n);
	return n;
}

char * rs_export_get_data_to_save (char * game, char * mud, char ** body, int * body_size) {
	SESSION_STATE * session;
	char * r = NULL;
	GDir * dir;
	GError * error = NULL;
	gchar * entry = NULL;
	gchar * t = NULL;
	gchar * name = NULL;
	gchar * game_name = NULL;
	gboolean found = FALSE;
	gboolean exist;
	xmlNodePtr n, ch;
	xmlDocPtr doc;
	xmlChar *xmlbuff;
	int buffersize;
	char buf [1024];
	uLongf dstlen;

	dir = g_dir_open (config->savedir, 0, &error);
	if (!dir) return "Unable to open savedir";
	while ((!found) && (entry = (gchar *) g_dir_read_name (dir))) {
		t = g_build_path(G_DIR_SEPARATOR_S, config->savedir, entry, NULL);
		exist = session_saved_get_name (t, &name, &game_name, NULL);
		if (exist && ((name != NULL) || (game_name != NULL))) {
			found = !(g_ascii_strcasecmp (name, game) || g_ascii_strcasecmp (game_name, mud));
		}
		g_free (name);
		g_free (game_name);
		name = NULL;
		game_name = NULL;
		if (!found) g_free (t);
	}
	if (!found) return "Saving directory not found";
	session = session_new ();
	session->slot = t;
	session_load (session, session->slot);
	if (session->gerrors) {
		session_delete (session);
		return "Can't load session.";
	}
	doc = xmlNewDoc (BAD_CAST "1.0");
	n = xmlNewNode (NULL, BAD_CAST "game");
	XML_ADD_CHILD_STRING ("host", session->game_host);
	XML_ADD_CHILD_INT ("port", session->game_port);
	XML_ADD_CHILD_STRING ("font", session->font);
	XML_ADD_CHILD_STRING ("bg_color", session->bg_color);
	XML_ADD_CHILD_STRING ("fg_color", session->fg_color);
	XML_ADD_CHILD_STRING ("ufg_color", session->ufg_color);
	XML_ADD_CHILD_INT ("single_line", session->single_line);
	XML_ADD_CHILD_INT ("local_echo", session->local_echo);
	XML_ADD_CHILD_INT ("logging", session->logging);
	XML_ADD_CHILD_INT ("sound", session->sound);
	XML_ADD_CHILD_STRING ("proxy", session->proxy);
	xmlAddChild (n, rs_export_get_delayed_cmds (session));
	xmlAddChild (n, rs_export_get_atm ("macroses", session->macros));
	xmlAddChild (n, rs_export_get_atm ("aliases", session->aliases));
	xmlAddChild (n, rs_export_get_atm ("triggers", session->triggers));
	xmlAddChild (n, rs_export_get_variables (session->variables));
	xmlAddChild (n, rs_export_get_statvars (session->svlist));
	xmlAddChild (n, rs_export_get_gauges (session->gaugelist));
	xmlAddChild (n, rs_export_get_owindows (session->windowlist));
	xmlDocSetRootElement (doc, n);
	xmlDocDumpFormatMemory (doc, &xmlbuff, &buffersize, 1);
	xmlFreeDoc (doc);
	dstlen = 3 * compressBound (buffersize);
	* body = g_malloc (dstlen);
	if (Z_OK == compress (* body, &dstlen, xmlbuff, buffersize)) {
		* body_size = dstlen;
	} else {
		g_free (* body);
		* body_size = 0;
		* body = NULL;
		r = "Compression failed";
	}
	xmlFree (xmlbuff);
	return r;
}

char * tools_remote_storage_action_export (CURL * curl, char * game, char * mud, char * slot) {
	struct curl_httppost * post = NULL;
	struct curl_httppost * last = NULL;
	char * ret;
	char * body;
	int body_size;
	long code;
	DownloadedData * data;
	char * engame, * enmud;

	ret = rs_export_get_data_to_save (game, mud, &body, &body_size);
	if (!ret) {
		xmlDocPtr doc = xmlNewDoc (BAD_CAST "1.0");

		engame = xmlEncodeEntitiesReentrant (doc, BAD_CAST game);
		enmud = xmlEncodeEntitiesReentrant (doc, BAD_CAST mud);
		curl_formadd (&post, &last, CURLFORM_PTRNAME, "game", CURLFORM_COPYCONTENTS, engame, CURLFORM_END);
		curl_formadd (&post, &last, CURLFORM_PTRNAME, "mud", CURLFORM_COPYCONTENTS, enmud, CURLFORM_END);
		xmlFree (engame);
		xmlFree (enmud);
		xmlFreeDoc (doc);
		curl_formadd (
			&post,
			&last,
			CURLFORM_PTRNAME, "body",
			CURLFORM_BUFFER, "body",
			CURLFORM_BUFFERPTR, body,
			CURLFORM_BUFFERLENGTH, body_size,
			CURLFORM_END
		);
		code = proxy_download_url (curl, NULL, RS_EXPORT_URL, NULL, NULL, post, &data);
		if (-1 == code) {
			ret = "Connection to remote storage failed.";
		} else {
			if (202 == code) ret = "Success.";
			else ret = "Failed.";
		}
		if (body) g_free (body);
		curl_formfree (post); 
		discard_downloaded_data (data);
	}
	return ret;
}

char * tools_remote_storage_action_remove (CURL * curl, char * game, char * mud, char * slot) {
	char * ret;
	struct curl_httppost * post = NULL;
	struct curl_httppost * last = NULL;
	DownloadedData * data;
	long code;
	char * engame, * enmud;
	xmlDocPtr doc = xmlNewDoc (BAD_CAST "1.0");

	engame = xmlEncodeEntitiesReentrant (doc, BAD_CAST game);
	enmud = xmlEncodeEntitiesReentrant (doc, BAD_CAST mud);
	curl_formadd (&post, &last, CURLFORM_PTRNAME, "game", CURLFORM_COPYCONTENTS, engame, CURLFORM_END);
	curl_formadd (&post, &last, CURLFORM_PTRNAME, "mud", CURLFORM_COPYCONTENTS, enmud, CURLFORM_END);
	xmlFree (engame);
	xmlFree (enmud);
	xmlFreeDoc (doc);
	code = proxy_download_url (curl, NULL, RS_REMOVE_URL, NULL, NULL, post, &data);
	if (-1 == code) {
		ret = "Connection to remote storage failed.";
	} else {
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &code);
		if (202 == code) ret = "Success.";
		else ret = "Failed.";
		discard_downloaded_data (data);
	}
	curl_formfree (post); 
	return ret;
}

RemoteStorageAction action_perform_import = {
	"Local game setting will be replaced by saved one on server. Are you sure?",
	tools_remote_storage_action_import,
	"Importing",
	1
};

RemoteStorageAction action_perform_export = {
	"Saved game setting will be replaced by local ones. Are you sure?",
	tools_remote_storage_action_export,
	"Exporting",
	1
};

RemoteStorageAction action_perform_remove = {
	"Saved game setting will be removed. Local game settings will remain intact. Are you sure?",
	tools_remote_storage_action_remove,
	"Removing",
	0
};

void rs_init_remotes_harvesting (char *, char *);

void rs_cleanup_perform_thread_data (RSPerformThreadData * d) {

	if (d->done && d->progress_destroyed) {
		if (d->list_view_destroyed) {
			g_free (d->acct_user);
			g_free (d->acct_passwd);
			g_free (d);
		} else {
			// refresh information in game list
			rs_init_remotes_harvesting (g_strdup (d->acct_user), g_strdup (d->acct_passwd));
			gtk_widget_destroy (d->list_view);
		}
	}
}

gboolean remote_games_perform_ready (gpointer data) {
	RSPerformThreadData * d = (RSPerformThreadData *) data;
	RSPerformThreadStatus * ts = (RSPerformThreadStatus *) g_async_queue_try_pop (d->queue);
	gboolean done = FALSE;

	if (ts) {
		done = ts->done;
		if (!d->progress_destroyed) {
			GtkTextView * t = GTK_TEXT_VIEW (interface_get_widget (GTK_WIDGET (d->status_dlg), "textview_status"));
			GtkTextBuffer * b = gtk_text_view_get_buffer (t);
			GtkTextMark * m;
			GtkTextIter i;
			char buf [1024];

			gtk_text_buffer_get_end_iter (b, &i);
			if (done) {
				g_snprintf (buf, 1024, "%s\n", ts->status);
				gtk_text_buffer_insert (b, &i, buf, -1);
			} else {
				if (ts->status) g_snprintf (buf, 1024, "%s\n", ts->status);
				else g_snprintf (buf, 1024, "%s '%s' (%s): ", ts->actname, ts->game, ts->mud);
				gtk_text_buffer_insert (b, &i, buf, -1);
			}
			m = gtk_text_buffer_get_mark (b, "the_end");
			if (!m) m = gtk_text_buffer_create_mark (b, "the_end", &i, FALSE);
			else gtk_text_buffer_move_mark (b, m, &i);
			gtk_text_view_scroll_to_mark (t, m, 0, FALSE, 0, 1.0);
		}
		if (done) {
			g_async_queue_unref (d->queue);
			if (d->progress_destroyed) {
				GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
					NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					(gchar *) "%s selected games finished.",
					ts->actname
				));
				gtk_dialog_run (GTK_DIALOG (md));
				gtk_widget_destroy (GTK_WIDGET (md));
			}
			d->done = TRUE;
			rs_cleanup_perform_thread_data (d);
		}
		if (ts->game) g_free (ts->game);
		if (ts->mud) g_free (ts->mud);
		if (ts->status) g_free (ts->status);
		if (ts->slot) g_free (ts->slot);
		g_free (ts);
	}
	return !done;
}

gpointer remote_storage_perform_thread (gpointer data) {
	RSPerformThreadData * d = (RSPerformThreadData *) data;
	GList * i = d->games;
	RSPerformThreadStatus * s;
	CURL * curl = curl_easy_init ();
	char errbuf [CURL_ERROR_SIZE];
	char * rs;
	char userpwd [1024];

	g_async_queue_ref (d->queue);
	if (curl) {
		if (d->proxy && g_ascii_strcasecmp (d->proxy->name, "None") && g_ascii_strcasecmp (d->proxy->name, "MudMagic")) {
			char buf [1024];

			curl_easy_setopt (curl, CURLOPT_PROXYPORT, d->proxy->port);
			curl_easy_setopt (curl, CURLOPT_PROXY, d->proxy->host);
			if (d->proxy->user && strlen (d->proxy->user)) {
				g_snprintf (buf, 1024, "%s:%s", d->proxy->user, d->proxy->passwd);
				curl_easy_setopt (curl, CURLOPT_PROXYUSERPWD, buf);
			}
		}
		curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errbuf);
		g_snprintf (userpwd, 1024, "%s:%s", d->acct_user, d->acct_passwd);
		curl_easy_setopt (curl, CURLOPT_USERPWD, userpwd);
		while (i) {
			GameDefn * gd = (GameDefn *) i->data;

			s = g_new (RSPerformThreadStatus, 1);
			s->game = g_strdup (gd->game);
			s->mud = g_strdup (gd->mud);
			s->slot = g_strdup (gd->slot);
			s->status = NULL;
			s->done = FALSE;
			s->actname = d->actname;
			g_async_queue_push (d->queue, (gpointer) s);
			s = g_new (RSPerformThreadStatus, 1);
			i = g_list_remove (i, gd);
			s->game = g_strdup (gd->game);
			s->mud = g_strdup (gd->mud);
			s->slot = g_strdup (gd->slot);
			s->status = g_strdup (d->action (curl, s->game, s->mud, s->slot));
			s->done = FALSE;
			s->actname = d->actname;
			g_free (gd->game);
			g_free (gd->mud);
			g_free (gd->slot);
			g_free (gd);
			g_async_queue_push (d->queue, (gpointer) s);
		}
		curl_easy_cleanup (curl);
		rs = g_strdup ("All done.");
	} else {
		rs = g_strdup ("libCURL initialization failed!");
	}
	s = g_new (RSPerformThreadStatus, 1);
	s->done = TRUE;
	s->game = NULL;
	s->mud = NULL;
	s->slot = NULL;
	s->status = rs;
	s->actname = d->actname;
	g_async_queue_push (d->queue, (gpointer) s);
	g_async_queue_unref (d->queue);
	return NULL;
}

static void rs_perform_progress_destroyed (GtkWidget * widget, gpointer data) {
	RSPerformThreadData * thd = (RSPerformThreadData *) data;
	thd->progress_destroyed = TRUE;
	rs_cleanup_perform_thread_data (thd);
}

static void rs_perform_list_view_destroyed (GtkWidget * widget, gpointer data) {
	RSPerformThreadData * thd = (RSPerformThreadData *) data;
	thd->list_view_destroyed = TRUE;
	rs_cleanup_perform_thread_data (thd);
}

SESSION_STATE * rs_get_running_session (char * game, char * mud) {
	GList * i;
	SESSION_STATE * ret = NULL;

	for (i = g_list_first (config->sessions); i && !ret; i = g_list_next (i)) {
		SESSION_STATE * s = (SESSION_STATE *) i->data;

		if (!(g_ascii_strcasecmp (s->name, game) || g_ascii_strcasecmp (s->game_name, mud))) ret = s;
	}
	return ret;
}

void rs_session_close (SESSION_STATE * s) {
	GList * it;

	for (it = g_list_first (config->windows); it; it = g_list_next (it)) {
		GtkWindow * w = GTK_WINDOW (it->data);
		GtkNotebook * nb = GTK_NOTEBOOK (g_object_get_data (G_OBJECT(w), "notebook"));

		if (nb) {
			int n = gtk_notebook_get_n_pages (nb);
			int i;
			for (i = 0; i < n; i++) {
				GtkWidget * page = gtk_notebook_get_nth_page (nb, i);
				SESSION_STATE * session = g_object_get_data (G_OBJECT(page), "session");
				if (s == session) interface_remove_tab (page);
			}
		}
	}
}

char * rs_get_game_slot (char * game, char * mud) {
	char * ret = NULL;
	GDir * dir;
	char  * entry, * nname, * nmud, * t;
	gboolean exist;

	if (!g_file_test (config->savedir, G_FILE_TEST_IS_DIR)) return NULL;
	dir = g_dir_open (config->savedir, 0, NULL);
	if (!dir) return NULL;
	while ((entry = (gchar *) g_dir_read_name (dir)) && !ret) {
		t = g_build_path (G_DIR_SEPARATOR_S, config->savedir, entry, NULL);
		exist = session_saved_get_name(t, &nname, &nmud, NULL);
		if (exist && (nname != NULL || nmud != NULL)) {
			if (!(g_ascii_strcasecmp (nname, game) || g_ascii_strcasecmp (nmud, mud))) ret = t;
		} else g_free (t);
		g_free (nname);
		g_free (nmud);
	}
	g_free (dir);
	return ret;
}

void on_tools_remote_storage_perform (GtkButton * button, gpointer user_data) {
	GtkWidget * top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (GTK_WIDGET (top), "treeview_remote_storage"));
	GtkTreeSelection * sel = gtk_tree_view_get_selection (tv);
	GtkTreeModel * model = gtk_tree_view_get_model (tv);
	GThread * thread;
	GError * gerr = NULL;
	RSPerformThreadData * d;

	if (sel) {
		GList * i, * games = NULL, * rows = gtk_tree_selection_get_selected_rows (sel, &model);
		RemoteStorageAction * rsa = (RemoteStorageAction *) user_data;
		GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			(gchar *) "%s",
			rsa->conf_message
		));
		gint res = gtk_dialog_run (GTK_DIALOG (md));
		GtkTreeIter iter;

		gtk_widget_destroy (GTK_WIDGET (md));
		if (GTK_RESPONSE_YES == res) {
			for (i = g_list_first (rows); i; i = g_list_next (i)) {
				char * game, * mud;
				GameDefn * gd = g_new (GameDefn, 1);
				SESSION_STATE * z;

				gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) i->data);
				gtk_tree_model_get (model, &iter, 0, &game, 1, &mud, -1);
				gd->game = g_strdup (game);
				gd->mud = g_strdup (mud);
				gd->slot = rs_get_game_slot (game, mud);
				if ((z = rs_get_running_session (game, mud)) && rsa->must_close) {
					GtkMessageDialog * md;
					md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
						NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_OK_CANCEL,
						(gchar *) "Game session %s (%s) is running. The game session must be closed first. Press OK to close the session or CANCEL to exclude the game from action list.",
						game, mud
					));
					if (gtk_dialog_run (GTK_DIALOG (md)) == GTK_RESPONSE_OK) {
						rs_session_close (z);
						games = g_list_append (games, gd);
					} else {
						g_free (gd->game);
						g_free (gd->mud);
						g_free (gd->slot);
						g_free (gd);
					}
					gtk_widget_destroy (GTK_WIDGET (md));
				} else games = g_list_append (games, gd);
			}
			d = g_new (RSPerformThreadData, 1);
			d->games = games;
			d->action = rsa->action;
			d->actname = rsa->actname;
			d->queue = g_async_queue_new ();
			d->proxy = proxy_get_default (config->proxies);
			d->progress_destroyed = FALSE;
			d->list_view_destroyed = FALSE;
			d->done = FALSE;
			d->list_view = top;
			d->acct_user = g_object_get_data (G_OBJECT (top), "acct_user");
			d->acct_passwd = g_object_get_data (G_OBJECT (top), "acct_passwd");
			if ((thread = g_thread_create (remote_storage_perform_thread, d, FALSE, &gerr))) {
				d->status_dlg = GTK_DIALOG (interface_create_object_by_name ("dialog_rs_perform_status"));
				g_signal_connect (G_OBJECT (interface_get_widget (GTK_WIDGET (d->status_dlg), "closebutton")), "clicked", G_CALLBACK (on_tools_common_button_cancel), NULL);
				g_signal_connect (G_OBJECT (d->status_dlg), "destroy", G_CALLBACK (rs_perform_progress_destroyed), d);
				g_signal_connect (G_OBJECT (d->list_view), "destroy", G_CALLBACK (rs_perform_list_view_destroyed), d);
				g_idle_add (remote_games_perform_ready, d);
			} else {
				g_error ("Unable to create thread");
			}
			g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (rows);
		}
	} else {
		g_printf ("no selection\n");
	}
}

static void tools_remote_storage_setup_view (GList * remote_games, char * acct_user, char * acct_passwd) {
	GtkCellRenderer * r = gtk_cell_renderer_text_new ();
	GtkListStore * store;
	GtkTreeViewColumn * c;
	GList * i, * games, * local_games;
	GtkTreeIter iter;
	SavedGameInfo * g;
	char exported [64];
	char * t, * n;
	GtkWindow * window;
	GtkTreeView * tv;
	GtkWidget * b_export, * b_import, * b_remove;

	local_games = remote_storage_read_local_game_list ();
	TRY_ASSIGN (window, GTK_WINDOW (interface_create_object_by_name ("window_remote_storage")));
	TRY_ASSIGN (tv, GTK_TREE_VIEW (interface_get_widget (GTK_WIDGET (window), "treeview_remote_storage")));
	g_object_set_data (G_OBJECT (window), "acct_user", acct_user);
	g_object_set_data (G_OBJECT (window), "acct_passwd", acct_passwd);
	store = gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model (tv, GTK_TREE_MODEL (store));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (tv), GTK_SELECTION_MULTIPLE);
	c = gtk_tree_view_column_new_with_attributes ("Name", r, "text", 0, NULL);
	gtk_tree_view_append_column (tv, c);
	c = gtk_tree_view_column_new_with_attributes ("Mud", r, "text", 1, NULL);
	gtk_tree_view_append_column (tv, c);
	c = gtk_tree_view_column_new_with_attributes ("Exported", r, "text", 2, NULL);
	gtk_tree_view_append_column (tv, c);
	c = gtk_tree_view_column_new_with_attributes ("Notes", r, "text", 3, NULL);
	gtk_tree_view_append_column (tv, c);
	games = remote_storage_concatenate_lists (local_games, remote_games);

	store = (GtkListStore *) gtk_tree_view_get_model (tv);
	for (i = g_list_first (games); i; i = g_list_next (i)) {
		g = (SavedGameInfo *) i->data;
		if (g->saved) {
			strftime (exported, 64, "%Y-%m-%d %H:%M:%S", localtime (&g->saved));
			t = exported;
		} else {
			t = "";
		}
		n = (!g->local) ? "not exists locally" : "";
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, g->name, 1, g->mud, 2, t, 3, n, 4, g, -1);
	}
	g_list_free (games);
	g_signal_connect (G_OBJECT (interface_get_widget (GTK_WIDGET (window), "button_close")), "clicked", G_CALLBACK (on_tools_common_button_cancel), NULL);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (tv)), "changed", G_CALLBACK (on_tools_remote_storage_selection_changed), NULL);
	b_import = interface_get_widget (GTK_WIDGET (window), "toolbutton_import");
	b_export = interface_get_widget (GTK_WIDGET (window), "toolbutton_export");
	b_remove = interface_get_widget (GTK_WIDGET (window), "toolbutton_remove");
	gtk_widget_set_sensitive (b_import, FALSE);
	gtk_widget_set_sensitive (b_export, FALSE);
	gtk_widget_set_sensitive (b_remove, FALSE);
	g_signal_connect (G_OBJECT (b_import), "clicked", G_CALLBACK (on_tools_remote_storage_perform), &action_perform_import);
	g_signal_connect (G_OBJECT (b_export), "clicked", G_CALLBACK (on_tools_remote_storage_perform), &action_perform_export);
	g_signal_connect (G_OBJECT (b_remove), "clicked", G_CALLBACK (on_tools_remote_storage_perform), &action_perform_remove);
}

gboolean remote_games_list_ready (gpointer data) {
	ReadGameListThreadData * thd = (ReadGameListThreadData *) data;
	ReadGameListHarvestedData * d = (ReadGameListHarvestedData *) g_async_queue_try_pop (thd->queue);

	if (!(thd->cancelled || thd->destroyed)) {
		if (0.1 < g_timer_elapsed (thd->timer, NULL)) {
			gtk_progress_bar_pulse (GTK_PROGRESS_BAR (thd->pb));
			g_timer_start (thd->timer);
		}
	}
	if (d) {
		if (thd->cancelled || thd->destroyed) {
			if (!thd->destroyed) gtk_widget_destroy (thd->d);
			while (d->games) {
				SavedGameInfo * g = (SavedGameInfo *) (d->games->data);
				g_free (g->name);
				g_free (g->mud);
				d->games = g_list_first (g_list_remove (d->games, g));
				g_free (g);
			}
			g_free (thd->acct_user);
			g_free (thd->acct_passwd);
		} else {
			gtk_widget_destroy (thd->d);
			if (d->err) {
				if (g_ascii_strcasecmp (d->err, "Unauthorized")) {
					GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
						NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						(gchar *) "%s",
						d->err
					));
					gtk_dialog_run (GTK_DIALOG (md));
					gtk_widget_destroy (GTK_WIDGET (md));
					g_free (thd->acct_user);
					g_free (thd->acct_passwd);
				} else {
					g_free (thd->acct_passwd);
					rs_init_remotes_harvesting (thd->acct_user, g_strdup (""));
				}
			} else {
				tools_remote_storage_setup_view (d->games, thd->acct_user, thd->acct_passwd);
			}
		}
		g_timer_destroy (thd->timer);
		g_async_queue_unref (thd->queue);
		g_free (thd);
		g_free (d);
	}
	return !d;
}

static void rs_get_game_list_progress_destroyed (GtkWidget * widget, gpointer data) {
	ReadGameListThreadData * thd = (ReadGameListThreadData *) data;
	thd->destroyed = TRUE;
}

void rs_init_remotes_harvesting (char * acct_user, char * acct_passwd) {
	gboolean rept;
	gboolean canc = FALSE;
	GError * gerr = NULL;
	GThread * rgl_thread;
	ReadGameListThreadData * th_data;
	GtkWidget * l;

	do {
		rept = FALSE;
		while ((!canc) && ((!acct_user) || (!acct_passwd) || (!strlen (acct_user)) || (!strlen (acct_passwd)))) {
			canc = tools_remote_storage_get_acct_info (&acct_user, &acct_passwd);
		} 
		if (!canc) {
			th_data = g_new (ReadGameListThreadData, 1);
			th_data->d = gtk_dialog_new_with_buttons ("Getting game list...", NULL, GTK_DIALOG_MODAL, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
			th_data->pb = gtk_progress_bar_new ();
			th_data->proxy = proxy_get_default (config->proxies);
			th_data->acct_user = acct_user;
			th_data->acct_passwd = acct_passwd;
			th_data->queue = g_async_queue_new ();
			th_data->timer = g_timer_new ();
			th_data->cancelled = FALSE;
			th_data->destroyed = FALSE;
			l = gtk_label_new ("Retrieving game list from server. Please wait.");
			gtk_misc_set_padding (GTK_MISC (l), 6, 6);
			gtk_container_add (GTK_CONTAINER (GTK_DIALOG (th_data->d)->vbox), l);
			gtk_container_add (GTK_CONTAINER (GTK_DIALOG (th_data->d)->vbox), th_data->pb);
			if ((rgl_thread = g_thread_create (tools_remote_storage_get_remote_games_list_thread, th_data, FALSE, &gerr))) {
				gtk_widget_show_all (th_data->d);
				g_idle_add (remote_games_list_ready, th_data);
				g_signal_connect (G_OBJECT (th_data->d), "destroy", G_CALLBACK (rs_get_game_list_progress_destroyed), th_data);
				if (GTK_RESPONSE_CANCEL == gtk_dialog_run (GTK_DIALOG (th_data->d))) {
					th_data->cancelled = TRUE;
					gtk_widget_destroy (th_data->d);
					th_data->d = NULL;
				}
			} else {
				g_error ("Unable to create thread");
			}
		}
	} while (rept);
}

void on_tools_remote_storage (GtkMenuItem * menuitem, gpointer user_data) {
	char * acct_user = g_strdup (config->acct_user);
	char * acct_passwd = g_strdup (config->acct_passwd);

	rs_init_remotes_harvesting (acct_user, acct_passwd);
}

