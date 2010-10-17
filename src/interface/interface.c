/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* interface.c:                                                            *
*                2004  Calvin Ellis    ( kyndig@mudmagic.com )            *
*                2005  Mart Raudsepp   ( leio@users.sf.net   )            *
*                2005  Tomas Mecir     ( kmuddy@kmuddy.net   )            *
*                2005  Shlykov Vasiliy ( vash@zmail.ru )                  *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <mudmagic.h>
#include <protocols.h>
#include "interface.h"
#include "cmdentry.h"
#include <module.h>
#include <utils.h>

extern CONFIGURATION *config;

EXPORT SESSION_STATE *interface_get_active_session()
{
  GtkWidget *tab;
  SESSION_STATE *session;
  tab = interface_get_active_tab();
  if (!tab)
  {
    //g_warning("there is no active tab.");
    return NULL;
  }
  session = g_object_get_data(G_OBJECT(tab), "session");
  if (!session) {
    g_warning("no session attached to current tab.");
  }
  return session;
}

EXPORT GtkWidget *interface_get_active_tab()
{
  GtkWidget *win;
  GtkWidget *notebook;
  GtkWidget *tab;
  win = interface_get_active_window();

        if (win == NULL)
            return NULL;
  //g_return_val_if_fail(win != NULL, NULL);
  notebook =
      (GtkWidget *) g_object_get_data(G_OBJECT(win), "notebook");
  // a warmer behavior
  if (notebook == NULL)
  {
    return NULL;
  }
  //g_return_val_if_fail( notebook != NULL, NULL );
  tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
          gtk_notebook_get_current_page
          (GTK_NOTEBOOK(notebook))
      );
  return tab;
}

EXPORT GtkWidget *interface_get_active_window()
{
        Configuration* cfg = get_configuration ();
        GList* it = NULL;

  g_assert (cfg);

        for (it = g_list_first (cfg->windows); it; it = g_list_next (it))
        {
                if (gtk_window_is_active (GTK_WINDOW (it->data)))
                    return GTK_WIDGET (it->data);
        }

  //g_critical ("Active window not found.");
        it = g_list_first (cfg->windows);
  return (it ? GTK_WIDGET (it->data) : NULL);
}

EXPORT GtkWidget*
interface_get_main_toolbar (void)
{
    GtkWidget* win = interface_get_active_window ();
    g_return_val_if_fail (win != NULL, NULL);

    return interface_get_widget (win, "toolbar_main");
}

EXPORT GtkWidget *interface_tab_get_area_right(GtkWidget * tab)
{
  if (!tab)
    return NULL;
  return interface_get_widget(tab, "area_right");
}

EXPORT void
interface_modules_init (GList* modules)
{
    GtkWidget *menubar;
    GtkWidget *toolbar;
    GtkWidget* win = (GtkWidget*) g_list_first (get_configuration()->windows)->data;

    menubar = interface_get_widget(win, "menubar_main");
    if (menubar)
  module_call_all_menu_modify(config->modules, menubar);

    toolbar = interface_get_widget(win, "toolbar_main");
    if (toolbar)
  module_call_all_toolbar_modify(config->modules, toolbar);
}

EXPORT GtkWidget *interface_add_window()
{
  GtkWidget *win;
  win = interface_create_object_by_name("window_main");
  g_return_val_if_fail(win != NULL, NULL);
  // add this window to windows list
  config->windows = g_list_append(config->windows, win);
  /* Synthesize show event on intro. Doesn't seem to do it if Visible=True in glade file */
  gtk_widget_show(interface_get_widget(win, "intro"));

  return win;
}

EXPORT void interface_remove_window(GtkWidget * window)
{
  if (window == NULL)
    window = interface_get_active_window();
  config->windows = g_list_remove_all(config->windows, window);
  gtk_widget_destroy(window);
  if (config->windows == NULL) {
    g_print("no more windows ... ending\n");
    gtk_main_quit();
  }
}


EXPORT gint
interface_messagebox (gint type, gint buttons, const gchar* fmt, ...)
{
  va_list    ap;
  gchar*     msg;
  GtkWidget* master,
           * dialog;
  gint       result;

  va_start (ap, fmt);
  msg = g_strdup_vprintf (fmt, ap);
  va_end (ap);

  master = interface_get_active_window ();

  dialog = gtk_message_dialog_new (GTK_WINDOW (master),
                                     GTK_DIALOG_MODAL,
                                     type,
                                     buttons,
                                     "%s",
                                     msg
                                    );
  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  g_free (msg);

  return result;
}

EXPORT void
interface_show_error (MudError* error, const gchar* usermsg)
{
  if (error == NULL)
      return;

  interface_messagebox (GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE,
      "%s\n%s",
      usermsg,
      mud_error_get_msg (error)
           );
}

EXPORT void
interface_show_gerrors (GList* list, const gchar* usermsg)
{
  GtkWidget* master,
     * dialog;

        gchar*     msg = "";

        if (list != NULL)
            msg = utils_join_gerrors (list, "\n");

  master = interface_get_active_window ();

  dialog = gtk_message_dialog_new (GTK_WINDOW (master),
           GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         "%s\n%s",
                                         usermsg,
                                         msg
          );
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

        if (strcmp (msg, ""))
            g_free (msg);
}

/**
 * interface_remove_empty_slot: Shows request for remove empty slot.
 *
 * @slot: Slot name.
 *
 * Return value: TRUE if user pressed 'Yes',
 *               FALSE if user pressed 'No'.
 **/
EXPORT gint
interface_remove_empty_slot (const gchar* slot)
{
  GtkWidget* master,
     * dialog;
  gint     result;

  master = interface_get_active_window ();

  dialog = gtk_message_dialog_new (GTK_WINDOW (master),
           GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_YES_NO,
                                         "Slot %s is empty.\n Remove it?",
                                         slot
          );

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return result == GTK_RESPONSE_YES;
}

EXPORT void
interface_show_broken_installation (void)
{
  GtkWidget* master,
     * dialog;

  master = interface_get_active_window ();

  dialog = gtk_message_dialog_new (GTK_WINDOW (master),
           GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         "Installation of MudMagic client is broken.\n"
                                         "Please close the apllication and reinstall package.\n"
          );

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

EXPORT void interface_display_message(gchar * message)
{
  GtkWidget *wid;
  GtkWidget *label;
  wid = interface_create_object_by_name("dialog_info");
  g_return_if_fail(wid != NULL);
  label = interface_get_widget(wid, "info_message");
  g_return_if_fail(label != NULL);
  gtk_label_set_text(GTK_LABEL(label), message);
  gtk_dialog_run(GTK_DIALOG(wid));
  gtk_widget_destroy(wid);

}


/*
creates a new download window.
title     - is the title of the window
message   - is the text under progress bar
returns a pointer to the window; should not be used directly
*/
EXPORT gpointer interface_download_new(const gchar * title, const gchar * message)
{
  GtkWidget *ret;
  GtkWidget *wid;
  ret = interface_create_object_by_name("window_download");
  g_return_val_if_fail(ret != NULL, NULL);
  if (title != NULL) {
    gtk_window_set_title(GTK_WINDOW(ret), title);
  }
  if (message != NULL) {
    wid = interface_get_widget(ret, "download_message");
    if (wid != NULL)
      gtk_label_set_text(GTK_LABEL(wid), message);
  }
  g_object_set_data(G_OBJECT(ret), "canceled", (gpointer) FALSE);

  while (g_main_context_iteration(NULL, FALSE));
  //g_object_ref( G_OBJECT(ret) );
  return ret;
}

/* updates a download window
  win  - a pointer to a download window
  current
  total
*/
EXPORT void interface_download_update(gpointer win, gsize current,
              gsize total)
{
  GtkWidget *wid;
  gchar *text;
  g_return_if_fail(win != NULL);
  g_return_if_fail(strcmp
       (gtk_widget_get_name(GTK_WIDGET(win)),
        "window_download") == 0);
  wid = interface_get_widget(win, "download_progressbar");
  g_return_if_fail(wid != NULL);
  if (total != -1) {
    text = g_strdup_printf("%d%% (Total %d KB)", current*100/total, total/1024);
    if (total != 0)
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
                  (wid),
                  (gdouble) current /
                  total);
  } else {
    text = g_strdup_printf("%d", current);
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(wid));
  }
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(wid), text);
  g_free(text);

  while (g_main_context_iteration(NULL, FALSE));
}

EXPORT void interface_download_free(gpointer win)
{
  gtk_widget_destroy(GTK_WIDGET(win));
  while (g_main_context_iteration(NULL, FALSE));
}

EXPORT gboolean interface_download_iscanceled(gpointer win)
{
  gboolean ret;
  g_return_val_if_fail(win != NULL, FALSE);
  g_return_val_if_fail(strcmp
           (gtk_widget_get_name(GTK_WIDGET(win)),
            "window_download") == 0, FALSE);
  ret = (gboolean) g_object_get_data(G_OBJECT(win), "canceled");
  return ret;
}

void HH_StartDownload (HttpHelper* hh)
{
    g_assert (hh);
    hh->user_data_1 = interface_download_new ("Download", hh->user_data_2);
}

void HH_EndDownload (HttpHelper* hh)
{
    g_assert (hh);
    if (!hh->user_data_1)
        return;

    interface_download_free (hh->user_data_1);
    hh->user_data_1 = NULL;
}

void HH_UpdateDownload (HttpHelper* hh, gsize current, gsize total)
{
    g_assert (hh);
    if (!hh->user_data_1)
        return;

    interface_download_update (hh->user_data_1, current, total);
}

gboolean HH_GetStatus (HttpHelper* hh)
{
    g_assert (hh);
    if (!hh->user_data_1)
        return FALSE;

    return interface_download_iscanceled (hh->user_data_1);
}

HttpHelper*
httphelper_new (const gchar* title)
{
    HttpHelper* hh = g_new0 (HttpHelper, 1);
    hh->user_data_1 = NULL; /* win handler */
    hh->user_data_2 = g_strdup (title); /* title */

    hh->start_cb  = &HH_StartDownload;
    hh->end_cb    = &HH_EndDownload;
    hh->update_cb = &HH_UpdateDownload;
    hh->status_cb = &HH_GetStatus;

    return hh;
}

void
httphelper_free (HttpHelper* hh)
{
    if (hh != NULL)
    {
  g_free (hh->user_data_2);
  g_free (hh);
    }
}

EXPORT void interface_tab_disconnect(GtkWidget * tab)
{
  SESSION_STATE *session;
  GtkWidget *wid;
  g_return_if_fail(tab != NULL);
  g_return_if_fail(0 ==
       strcmp(gtk_widget_get_name(tab), "session_tab"));
  session = g_object_get_data(G_OBJECT(tab), "session");
  g_return_if_fail(session != NULL);

  // don't listen any more for data from server
  if (session->input_event_id != -1) {
    gtk_input_remove(session->input_event_id);
    session->input_event_id = -1;
  }
  // close the socket if it isn't yet
  if (session->telnet->fd > 0) {
    network_connection_close(session->telnet->fd);
    session->telnet->fd = NO_CONNECTION;
  }
  // reseset telnet state
  telnet_reset(session->telnet);

  wid = g_object_get_data(G_OBJECT(session->tab), "input1_entry");
  g_return_if_fail(wid != NULL);
  if (! gtk_entry_get_visibility(GTK_ENTRY(wid)) ) {
    interface_input_shadow(session, FALSE);
    gtk_entry_set_text(GTK_ENTRY(wid), "");
  }

}

EXPORT void interface_tab_connect(GtkWidget * tab)
{
  GtkWidget *wid, *label;
  SESSION_STATE *session;
  gchar *text;
  gint response;

  g_return_if_fail(tab != NULL);
  g_return_if_fail(0 == strcmp(gtk_widget_get_name(tab), "session_tab"));
  session = g_object_get_data(G_OBJECT(tab), "session");
  g_return_if_fail(session != NULL);

  // connect
  while ( 1 ) {
	if (session->pconn) proxy_connection_close (session->pconn);
	session->pconn = proxy_connection_open(
      session->game_host, session->game_port, proxy_get_by_name (session->proxy, config->proxies)
    );
	if (session->pconn) session->telnet->fd = session->pconn->sock; else session->telnet->fd = NO_CONNECTION;
	
    if ( session->telnet->fd != NO_CONNECTION ) {
      session->input_event_id = gtk_input_add_full(
        session->telnet->fd, GDK_INPUT_READ,
        (GdkInputFunction)on_data_available, NULL, tab, NULL
      );
      break;
    } else {
                        interface_messagebox (GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                              network_errmsg (session->telnet->fd));

      wid =
          interface_create_object_by_name
          ("dialog_connection_fail");
      g_return_if_fail(wid);
      label =
          interface_get_widget(wid,
             "connection_fail_message");
      g_return_if_fail(label != NULL);
      text =
          g_strdup_printf
          ("Connection attempt failed to: %s:%d",
           session->game_host, session->game_port);
      gtk_label_set_text(GTK_LABEL(label), text);
      g_free(text);
      response = gtk_dialog_run(GTK_DIALOG(wid));
      gtk_widget_destroy(wid);
      if (response == 1)
        continue; // try again
      else {
        // bug: can't remove the tab if its the first one
        // interface_remove_tab(tab);
        break;
      }
    }
  }
}

// hide user input ( eg when server ask password )
void interface_input_shadow(SESSION_STATE * session, gboolean shadow)
{
  GtkWidget *wid;
  g_return_if_fail(session != NULL && session->tab != NULL);

  wid = g_object_get_data(G_OBJECT(session->tab), "input1_entry");
  g_return_if_fail(wid != NULL);

  gtk_entry_set_visibility(GTK_ENTRY(wid),
         !shadow);
  wid = interface_get_widget(session->tab, "input2");

  g_return_if_fail(wid != NULL);
  gtk_widget_set_sensitive(wid, !shadow);
}

// get output1 size in characters
EXPORT void interface_get_output_size(SESSION_STATE * session,
              gint * width, gint * height)
{
  PangoContext *context;
  PangoFontMetrics *metrics;
  PangoFontDescription *desc;
  GtkWidget *textview;  // output1 textview

  *width = *height = 0;
  g_return_if_fail(session != NULL && width != NULL
       && height != NULL);

  textview = interface_get_widget(session->tab, "output1");
  g_return_if_fail(textview != NULL);

  context = gtk_widget_get_pango_context(textview);
  desc = pango_context_get_font_description(context);
  metrics = pango_context_get_metrics(context, desc, NULL);


  *width = textview->allocation.width /
      PANGO_PIXELS(pango_font_metrics_get_approximate_digit_width
       (metrics));
  *height =
      textview->allocation.height /
      (PANGO_PIXELS(pango_font_metrics_get_ascent(metrics)) +
       PANGO_PIXELS(pango_font_metrics_get_descent(metrics)));
  pango_font_metrics_unref(metrics);

}

void interface_statusvars_edit(SVLIST *statusvars, const gchar* title)
{
  GtkWidget *dialog;

  GtkWidget *treeview;
  GtkListStore *store;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;

  GList *l;
  gchar *s;

  // create a statusvars dialog
  dialog = interface_create_object_by_name("dialog_statusvars");
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(statusvars != NULL);
  if (title != NULL) {
    gtk_window_set_title(GTK_WINDOW(dialog), title);
  }
  g_object_set_data(G_OBJECT(dialog), "statusvars_list", statusvars);


  // fill statusvar list
  treeview = interface_get_widget(dialog, "treeview_statusvars_list");
  g_return_if_fail(treeview != NULL);
  store = gtk_list_store_new(1, G_TYPE_STRING);
  l = statusvars->list;
  while (l != NULL) {
    gtk_list_store_append(store, &iter);
    s = ((STATUSVAR *) l->data)->variable;
    gtk_list_store_set(store, &iter, 0, s, -1);
    l = g_list_next(l);
  }
  gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),
                          GTK_TREE_MODEL(store));
  renderer = gtk_cell_renderer_text_new();
  column =
      gtk_tree_view_column_new_with_attributes("Status variable", renderer,
                                               "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(select), "changed",
                   G_CALLBACK
                       (on_treeview_statusvars_list_selection_changed),
                   dialog);
  // end - status variables list

  gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW (interface_get_active_window() ));
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void interface_gauges_edit(GAUGELIST *gauges, const gchar* title)
{
  GtkWidget *dialog;

  GtkWidget *treeview;
  GtkListStore *store;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;

  GList *l;
  gchar *s;

  // create a gauges dialog
  dialog = interface_create_object_by_name("dialog_gauges");
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(gauges != NULL);
  if (title != NULL) {
    gtk_window_set_title(GTK_WINDOW(dialog), title);
  }
  g_object_set_data(G_OBJECT(dialog), "gauges_list", gauges);


  // fill statusvar list
  treeview = interface_get_widget(dialog, "treeview_gauges_list");
  g_return_if_fail(treeview != NULL);
  store = gtk_list_store_new(1, G_TYPE_STRING);
  l = gauges->list;
  while (l != NULL) {
    gtk_list_store_append(store, &iter);
    s = ((GAUGE *) l->data)->variable;
    gtk_list_store_set(store, &iter, 0, s, -1);
    l = g_list_next(l);
  }
  gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),
                          GTK_TREE_MODEL(store));
  renderer = gtk_cell_renderer_text_new();
  column =
      gtk_tree_view_column_new_with_attributes("Gauge", renderer,
                                               "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(select), "changed",
                   G_CALLBACK
                       (on_treeview_gauges_list_selection_changed),
                   dialog);
  // end - gauges list

  gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW (interface_get_active_window() ));
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void interface_tab_refresh(GtkWidget * tab)
{
  GtkWidget *wid;
  PangoFontDescription *font_desc;
  SESSION_STATE *session;
  GdkColor color;

  GtkTextBuffer *buffer;
  GtkTextTag *tag;
  GtkTextTagTable *tagtable;

  ATM *script;
  GtkWidget *button;
  GList *l;

  session = g_object_get_data(G_OBJECT(tab), "session");
  if (session->single_line) {
    wid = interface_get_widget(tab, "input1_entry");
    gtk_widget_show(wid);
    cmd_entry_init (wid, &session->cmdline);
    gtk_widget_grab_focus(wid);
    gtk_widget_hide(interface_get_widget(tab, "input2_c"));
  } else {
    wid = interface_get_widget(tab, "input1_entry");
    gtk_widget_hide(wid);
    gtk_widget_show(interface_get_widget(tab, "input2_c"));
    gtk_widget_grab_focus(interface_get_widget(tab, "input2"));
  }
  font_desc = pango_font_description_from_string(session->font);

  wid = interface_get_widget(tab, "output1");

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wid));
  tagtable = gtk_text_buffer_get_tag_table(buffer);
  tag = gtk_text_tag_table_lookup(tagtable, "user_input_tag");
  if (tag) {
    //gtk_text_tag_table_remove ( tagtable, tag );
    g_object_set(tag, "foreground", session->ufg_color, NULL);

    //tag = gtk_text_buffer_create_tag (buffer, "user_input_tag",
    //  "foreground", session->ufg_color, NULL);
  }

  gdk_color_parse(session->bg_color, &color);

  gtk_widget_modify_base(wid, GTK_STATE_NORMAL, &color);
  gtk_widget_modify_bg(wid, GTK_STATE_NORMAL, &color);
  gdk_color_parse(session->fg_color, &color);
  gtk_widget_modify_text(wid, GTK_STATE_NORMAL, &color);
  gtk_widget_modify_fg(wid, GTK_STATE_NORMAL, &color);
  gtk_widget_modify_font(wid, font_desc);
  wid = interface_get_widget(tab, "output2");
  gdk_color_parse(session->bg_color, &color);
  gtk_widget_modify_base(wid, GTK_STATE_NORMAL, &color);
  gtk_widget_modify_bg(wid, GTK_STATE_NORMAL, &color);
  gdk_color_parse(session->fg_color, &color);
  gtk_widget_modify_text(wid, GTK_STATE_NORMAL, &color);
  gtk_widget_modify_fg(wid, GTK_STATE_NORMAL, &color);
  gtk_widget_modify_font(wid, font_desc);
  pango_font_description_free(font_desc);

  // add macro buttons

  wid = interface_get_widget(tab, "macros_toolbar");
  if( wid != NULL )
  {
    g_return_if_fail(wid != NULL);
  }

  l = gtk_container_get_children(GTK_CONTAINER(wid));
  while (l) {
    gtk_widget_destroy(GTK_WIDGET(l->data));
    l = g_list_next(l);
  }

  /* Mud specific macros */
  l = g_list_first (session->macros);
  while (l) {
    script = (ATM *) l->data;

    if ((script->name) && (strlen(script->name))) 
    {
      button = gtk_button_new_with_label(script->name);
      gtk_widget_set_name(button, script->name);
      gtk_container_add(GTK_CONTAINER(wid), button);
      gtk_widget_show(button);
      g_object_set_data(G_OBJECT(button), "session",
            session);

      g_signal_connect(button, "clicked",
           G_CALLBACK
           (on_macro_button_clicked),
           script);
    }
    l = g_list_next(l);
  }

  /* Global macros */
  l = g_list_first (config->macros);
  while (l) 
  {
    script = (ATM *) l->data;
    if ((script->name) && (strlen(script->name))) 
    {
        button =
            gtk_button_new_with_label(script->
                    name);
        gtk_container_add(GTK_CONTAINER(wid),
              button);
        gtk_widget_show(button);
        g_object_set_data(G_OBJECT(button), "session",
                                          session);

        g_signal_connect(button, "clicked",
             (GCallback)
             on_macro_button_clicked,
             script);
    }
    l = g_list_next(l);
  }

}


EXPORT void interface_display_file(gchar * title, gchar * filename)
{
  GtkWidget *dialog, *textview, *label;
  GtkTextBuffer *buffer;
  gchar *buff;

  g_return_if_fail(filename != NULL);

  dialog = interface_create_object_by_name("dialog_display_file");
  g_return_if_fail(dialog != NULL);

  label = interface_get_widget(dialog, "label_filename");
  if (label != NULL)
    gtk_label_set_text(GTK_LABEL(label), filename);

  if (title != NULL)
    gtk_window_set_title(GTK_WINDOW(dialog), title);

  if (g_file_get_contents(filename, &buff, NULL, NULL)) {
    textview =
        interface_get_widget(dialog, "textview_content");
    if (textview != NULL) {
      buffer =
          gtk_text_view_get_buffer(GTK_TEXT_VIEW
                 (textview));
      if (buffer != NULL)
        gtk_text_buffer_set_text(buffer, buff, -1);
    }
    g_free(buff);
  }
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

}


EXPORT void interface_add_global_accel_group (MudAccelGroup* accel)
{
    GtkWidget* win = interface_get_active_window ();

    g_return_if_fail (accel != NULL);
    if (win == NULL)
        return;

    gtk_window_add_accel_group (GTK_WINDOW (win), accel);
}

EXPORT void interface_remove_global_accel_group (MudAccelGroup* accel)
{
    GtkWidget* win = NULL;

     //check if we're closing client before getting active window
    if(accel == NULL)
  return;

    win = interface_get_active_window ();

    g_return_if_fail (accel != NULL);
    if (win == NULL)
        return;

    gtk_window_remove_accel_group (GTK_WINDOW (win), accel);
}

