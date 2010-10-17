/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* internal.c:                                                             *
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
#include <math.h>
#include <stdlib.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <mudmagic.h>
#include <log.h>
#include <module.h>
#include <interface.h>
#include <directories.h>
#include "cmdentry.h"
#include "sessions.h"

extern CONFIGURATION *config;

static gboolean
on_output_scroll(GtkWidget * widget, GdkEvent * event,
     GtkWidget * scrollbar)
{
  if (event->type != GDK_SCROLL)
    return FALSE;

  if (!GTK_WIDGET_REALIZED(scrollbar)) {
    g_warning
        ("Attempting to scroll the output, but no scrollbar found");
    return FALSE;
  }

  gtk_widget_event(scrollbar, event);
  return TRUE;
}

GtkWidget *interface_create_object_by_name(gchar * name)
{
  GladeXML *xml;
  GtkWidget *ret;
  gchar *glade_fn;  // glade file name
  if (!name)
    return NULL;
  glade_fn = g_build_filename( mudmagic_data_directory(), "interface", "interface.glade", NULL);
  xml = glade_xml_new(glade_fn, name, NULL);
  if (!xml) 
  {
    g_free(glade_fn);
    glade_fn = g_build_filename("..", "interface", "interface.glade", NULL);
    g_message("try with %s.", glade_fn);
    xml = glade_xml_new(glade_fn, name, NULL);
  }
  g_free(glade_fn);

  //glade_xml_signal_autoconnect( xml );
  ret = glade_xml_get_widget(xml, name);
  if (!ret)
    g_warning("object (%s) is NULL", name);

  // FIXME: Why is this commented out?
  // Won't that mean each time a new tab is opened all of the below is leaked?
  //if ( !strcmp( name, "window_main" )) {
  glade_xml_signal_connect_data(xml, "gtk_widget_destroy",
              (GCallback) gtk_widget_destroy,
              NULL);
  glade_xml_signal_connect_data(xml, "gtk_main_quit",
              (GCallback) gtk_main_quit, NULL);
  glade_xml_signal_connect_data(xml, "on_intro_show",
              (GCallback) on_intro_show, NULL);
  glade_xml_signal_connect_data(xml, "on_intro_hide",
              (GCallback) on_intro_hide, NULL);
  glade_xml_signal_connect_data(xml, "on_character1_menu_activated",
              (GCallback)
              on_character1_menu_activated, NULL);
  glade_xml_signal_connect_data(xml, "on_new1_activate",
              (GCallback) on_new1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_quick_connect_1_activate",
              (GCallback)
              on_quick_connect_1_activate, NULL);

  //theme window
  glade_xml_signal_connect_data(xml, "on_theme_select1_activate",
                                      (GCallback)
                                      on_theme_select1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_theme_reset_button_clicked",
                                      (GCallback)
                                      on_theme_reset_button_clicked, NULL);
  glade_xml_signal_connect_data(xml, "on_theme_cancel_button_enter",
                                      (GCallback)
                                      on_theme_cancel_button_enter, NULL);
  glade_xml_signal_connect_data(xml, "on_theme_ok_button_clicked",
                                      (GCallback)
                                      on_theme_ok_button_clicked, NULL);
  //end theme

  glade_xml_signal_connect_data(xml, "on_open1_activate",
              (GCallback) on_open1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_reconnect1_activate",
              (GCallback) on_reconnect1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_close1_activate",
              (GCallback) on_close1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_quit1_activate",
              (GCallback) on_quit1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_none1_activate",
              (GCallback) on_none1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_icons1_activate",
              (GCallback) on_icons1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_text1_activate",
              (GCallback) on_text1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_both1_activate",
              (GCallback) on_both1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_modules1_activate",
              (GCallback) on_modules1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_configuration_1_activate",
              (GCallback)
              on_configuration_1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_profile_menu_activated",
              (GCallback)
              on_profile_menu_activated, NULL);
  glade_xml_signal_connect_data(xml, "on_preferences_1_activate",
              (GCallback)
              on_preferences_1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_tabs_menu_activated",
              (GCallback) on_tabs_menu_activated,
              NULL);
  glade_xml_signal_connect_data(xml, "on_profile_menu_cb_toggled",
              (GCallback)
              on_profile_menu_cb_toggled, NULL);
  glade_xml_signal_connect_data(xml, "on_previous_tab1_activate",
              (GCallback)
              on_previous_tab1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_next_tab1_activate",
              (GCallback) on_next_tab1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_move_tab_left1_activate",
              (GCallback)
              on_move_tab_left1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_move_tab_right1_activate",
              (GCallback)
              on_move_tab_right1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_detach_tab1_activate",
              (GCallback) on_detach_tab1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_button_reconnect_clicked",
              (GCallback)
              on_button_reconnect_clicked, NULL);
  glade_xml_signal_connect_data(xml, "on_about1_activate",
              (GCallback) on_about1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_documentation1_activate",
              (GCallback)
              on_documentation1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_mudmagic_website1_activate",
              (GCallback)
              on_mudmagic_website1_activate, NULL);
  glade_xml_signal_connect_data(xml, "on_toggle_ml_toggled",
              (GCallback) on_toggle_ml_toggled,
              NULL);
  glade_xml_signal_connect_data(xml, "on_input_key_press_event",
              (GCallback) on_input_key_press_event,
              NULL);
  glade_xml_signal_connect_data(xml, "on_input2_key_press_event",
              (GCallback)
              on_input2_key_press_event, NULL);
  glade_xml_signal_connect_data(xml, "on_button_send_clicked",
              (GCallback) on_button_send_clicked,
              NULL);
  glade_xml_signal_connect_data(xml, "on_new_char_create_clicked",
              (GCallback)
              on_new_char_create_clicked, NULL);
  glade_xml_signal_connect_data(xml, "on_new_char_update_clicked",
              (GCallback)
              on_new_char_update_clicked, NULL);
  glade_xml_signal_connect_data(xml, "on_saved_games_delete_clicked",
              (GCallback)
              on_saved_games_delete_clicked, NULL);
  glade_xml_signal_connect_data(xml, "on_saved_games_load_clicked",
              (GCallback)
              on_saved_games_load_clicked, NULL);

  glade_xml_signal_connect_data(xml, "on_saved_games_new_clicked",
              (GCallback)
              on_saved_games_new_clicked, NULL);
  glade_xml_signal_connect_data(xml, "on_button_browse_clicked",
              (GCallback) on_button_browse_clicked,
              NULL);
/*
  glade_xml_signal_connect_data(xml, "on_button_trigger_add_clicked",
              (GCallback)
              on_button_trigger_add_clicked, NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_trigger_remove_clicked",
              (GCallback)
              on_button_trigger_remove_clicked,
              NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_conf_trigger_add_clicked",
              (GCallback)
              on_button_conf_trigger_add_clicked,
              NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_conf_trigger_remove_clicked",
              (GCallback)
              on_button_conf_trigger_remove_clicked,
              NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_conf_alias_add_clicked",
              (GCallback)
              on_button_conf_alias_add_clicked,
              NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_conf_alias_remove_clicked",
              (GCallback)
              on_button_conf_alias_remove_clicked,
              NULL);
*/
  glade_xml_signal_connect_data(xml,
              "on_entry_macro_expr_key_press_event",
              (GCallback)
              on_entry_macro_expr_key_press_event,
              NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_macro_capture_clicked",
              (GCallback)
              on_button_macro_capture_clicked,
              NULL);
/*
  glade_xml_signal_connect_data(xml, "on_button_macro_add_clicked",
              (GCallback)
              on_button_macro_add_clicked, NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_conf_macro_add_clicked",
              (GCallback)
              on_button_conf_macro_add_clicked,
              NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_macro_remove_clicked",
              (GCallback)
              on_button_macro_remove_clicked,
              NULL);
  glade_xml_signal_connect_data(xml,
              "on_button_conf_macro_remove_clicked",
              (GCallback)
              on_button_conf_macro_remove_clicked,
              NULL);
*/
 glade_xml_signal_connect_data(xml, "on_button_statusvar_add_clicked",
              (GCallback)
              on_button_statusvar_add_clicked, NULL);
 glade_xml_signal_connect_data(xml, "on_button_statusvar_remove_clicked",
              (GCallback)
              on_button_statusvar_remove_clicked, NULL);
 glade_xml_signal_connect_data(xml, "on_button_gauge_add_clicked",
              (GCallback)
              on_button_gauge_add_clicked, NULL);
 glade_xml_signal_connect_data(xml, "on_button_gauge_remove_clicked",
              (GCallback)
              on_button_gauge_remove_clicked, NULL);
 glade_xml_signal_connect_data(xml, "on_output1_c_size_allocate",
              (GCallback)
              on_output1_c_size_allocate, NULL);
  glade_xml_signal_connect_data(xml, "on_download_cancel_clicked",
              (GCallback)
              on_download_cancel_clicked, NULL);
  glade_xml_signal_connect_data(xml,
              "on_window_download_delete_event",
              (GCallback)
              on_window_download_delete_event,
              NULL);
  glade_xml_signal_connect_data(xml, "on_window_main_focus_in_event",
              (GCallback)
              on_window_main_focus_in_event, NULL);
  glade_xml_signal_connect_data(xml, "on_window_main_focus_out_event",
              (GCallback)
              on_window_main_focus_out_event, NULL);
  glade_xml_signal_connect_data(xml, "on_profile_actions_activate",
              (GCallback) on_profile_actions_activate,
              NULL);
/*
  glade_xml_signal_connect_data(xml, "on_aliases_1_activate",
              (GCallback) on_aliases_1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_triggers_1_activate",
              (GCallback) on_triggers_1_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_macros_1_activate",
              (GCallback) on_macros_1_activate,
              NULL);
*/
  glade_xml_signal_connect_data(xml, "on_status_variables_activate",
              (GCallback) on_status_variables_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_gauges_activate",
              (GCallback) on_gauges_activate,
              NULL);
/*
  glade_xml_signal_connect_data(xml, "on_aliases_2_activate",
              (GCallback) on_aliases_2_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_triggers_2_activate",
              (GCallback) on_triggers_2_activate,
              NULL);
  glade_xml_signal_connect_data(xml, "on_macros_2_activate",
              (GCallback) on_macros_2_activate,
              NULL);
*/
  glade_xml_signal_connect_data(xml, "on_cb_cmd_save_history_toggled",
              (GCallback) on_cb_cmd_save_history_toggled,
              NULL);
  glade_xml_signal_connect_data(xml, "on_saved_games_treeview_button_press_event",
                          G_CALLBACK (on_saved_games_treeview_button_press_event),
              NULL);

/* tools actions */

  glade_xml_signal_connect_data(xml, "on_tools_menu_activated",
              (GCallback) on_tools_menu_activated, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_common_button_clear",
              (GCallback) on_tools_common_button_clear, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_common_button_cancel",
              (GCallback) on_tools_common_button_cancel, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_common_open",
              (GCallback) on_tools_common_open, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_common_save",
              (GCallback) on_tools_common_save, NULL);



  glade_xml_signal_connect_data(xml, "on_log_view_activate",
              (GCallback) on_log_view_activate, NULL);


  glade_xml_signal_connect_data(xml, "on_scripts_testing_activate",
              (GCallback) on_scripts_testing_activate, NULL);

  glade_xml_signal_connect_data(xml, "on_scripts_testing_button_ok",
              (GCallback) on_scripts_testing_button_ok, NULL);


  glade_xml_signal_connect_data(xml, "on_ta_testing_activate",
              (GCallback) on_ta_testing_activate, NULL);

  glade_xml_signal_connect_data(xml, "on_ta_testing_button_ok",
              (GCallback) on_ta_testing_button_ok, NULL);

  glade_xml_signal_connect_data(xml, "on_lt_passing_activate",
              (GCallback) on_lt_passing_activate, NULL);

  glade_xml_signal_connect_data(xml, "on_lt_passing_button_ok",
              (GCallback) on_lt_passing_button_ok, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_lt_passing_open",
              (GCallback) on_tools_lt_passing_open, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_lt_passing_save",
              (GCallback) on_tools_lt_passing_save, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_lt_passing_button_clear",
              (GCallback) on_tools_lt_passing_button_clear, NULL);

  glade_xml_signal_connect_data(xml, "on_delayed_cmd_activate",
              (GCallback) on_delayed_cmd_activate, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_delayed_commands_add",
              (GCallback) on_tools_delayed_commands_add, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_delayed_commands_del",
              (GCallback) on_tools_delayed_commands_del, NULL);

  glade_xml_signal_connect_data(xml, "on_tools_remote_storage",
              (GCallback) on_tools_remote_storage, NULL);


  glade_xml_signal_connect_data (xml,  "gaugebar_expose",
    G_CALLBACK (gaugebar_expose), NULL);

/*  if ( !strcmp( name, "window_main" ))
        {
            GtkWidget* st = NULL;
            GdkPixbuf* icon = NULL;
            g_print ("Creating main window\n");

            st = interface_get_widget (ret, "statusbar_main");
            g_assert (NULL);

            icon = 
        }
*/  if (!strcmp(name, "session_tab")) {
    GtkWidget *out1, *out2;
    GtkTextBuffer *buffer;
    out1 = interface_get_widget(ret, "output1");
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(out1));
    out2 = interface_get_widget(ret, "output2");
    // make them use the same buffer
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(out2),
           gtk_text_view_get_buffer
           (GTK_TEXT_VIEW(out1))
        );

    // add the combo box by hand to fix a libglade windows problem
    GtkWidget *entry;
    GtkWidget *wid;

    wid = interface_get_widget(ret, "input");
    if (!wid)
      g_error("Can NOT find input container.");

    entry = cmd_entry_create ();
    gtk_widget_show(entry);

    g_object_set_data(G_OBJECT(wid), "input1_entry", entry);
    g_object_set_data(G_OBJECT(ret), "input1_entry", entry);

    gtk_box_pack_start(GTK_BOX(wid), entry, FALSE, FALSE, 0);

    // make scrolling work
    GtkAdjustment *vadj, *hadj;
    GtkWidget *textw;
    GtkRange *scroll_bar;
    textw = interface_get_widget(ret, "output1");
    scroll_bar =
        (GtkRange *) interface_get_widget(ret,
                  "output1_scrollbar");
    hadj =
        (GtkAdjustment *) gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    vadj = gtk_range_get_adjustment(scroll_bar);
    gtk_widget_set_scroll_adjustments(textw, hadj, vadj);

    // add event handler for vertical scroll
    g_signal_connect((gpointer) vadj, "value-changed",
         G_CALLBACK(on_output1_c_vscroll), ret);

    // Forward output windows scroll events to the scrollbar
    g_signal_connect(out1, "event",
         G_CALLBACK(on_output_scroll), scroll_bar);
    g_signal_connect(out2, "event",
         G_CALLBACK(on_output_scroll), scroll_bar);
  }
  //g_object_unref( G_OBJECT(xml) );
  return ret;
}

GtkWidget *interface_get_widget(GtkWidget * wid, gchar * name)
{
  GtkWidget *ret;
  g_return_val_if_fail(wid != NULL, NULL);

  if (!strcmp(name, "input1_entry")) {  // a hack to handle Combo on win
    GtkWidget *input = NULL;
    // get container
    input =
        glade_xml_get_widget(glade_get_widget_tree(wid),
           "input");
    g_return_val_if_fail(input, NULL);
    ret = g_object_get_data(G_OBJECT(input), "input1_entry");
    if (!ret)
      g_warning("input1 not found.");
  } else {
    ret =
        glade_xml_get_widget(glade_get_widget_tree(wid), name);
    if (!ret) {
      g_print
          ("[interface_get_widget] %s not found (from %s)\n",
           name, wid->name);
    }
  }
  return ret;
}



GtkWidget *interface_add_tab(GtkWidget * window, GtkWidget * tab)
{
  if (window) {
    GtkWidget *notebook = NULL;
    /* try to get the notebook object */
    notebook =
        (GtkWidget *) g_object_get_data(G_OBJECT(window),
                "notebook");
#ifdef DEBUG
    if (!notebook) {
      g_message("Not notebook found !");
    }
#endif
    if (!notebook) {  // don't have a notebook ? then create one
      GtkWidget *vbox = NULL;

      gtk_widget_hide(interface_get_widget
          (window, "intro"));
      // create the notebook
      notebook = gtk_notebook_new();

      g_signal_connect_after(G_OBJECT(notebook),
                 "switch_page",
                 (GCallback)
                 on_notebook_page_changed,
                 window);

      gtk_widget_set_name(notebook, "notebook");
      gtk_widget_show(notebook);

      vbox = interface_get_widget(window, "vbox_main");
#ifdef DEBUG
      if (!vbox) {
        g_print("Not vbox found !\n");
      }
#endif
      gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE,
             TRUE, 0);

      gtk_box_reorder_child(GTK_BOX(vbox), notebook, 2);
      /* link notebook with main window ... */
      g_object_set_data(G_OBJECT(window), "notebook",
            notebook);
    }

    if (tab) {
    g_message ("Created notebook!\n");

      gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
             tab, NULL);

    } else {
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
             (tab =
              interface_create_object_by_name
              ("session_tab")), NULL);
    }

    // for one tab hide the label
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) < 2) {
      gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook),
               FALSE);
    } else {
      gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook),
               TRUE);
    }
    // go to the last tab
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), -1);

  } else {
    g_warning("[add_tab] window recieved is NULL\n");
  }
  return tab;
}

void interface_remove_tab(GtkWidget * tab)
{
  GtkNotebook *notebook;
  SESSION_STATE *session;
  g_return_if_fail(tab != NULL);
  g_return_if_fail(0 ==
       strcmp(gtk_widget_get_name(tab), "session_tab"));
  // get parent notebook
  notebook = GTK_NOTEBOOK(gtk_widget_get_ancestor(tab, GTK_TYPE_NOTEBOOK));
  g_return_if_fail(notebook != NULL);

  // get session
  session = g_object_get_data(G_OBJECT(tab), "session");
  g_return_if_fail(session != NULL);

  // close log file if there is the case
  if (session->log_file) {
    log_close_logfile(session->log_file);
    session->log_file = NULL;
  }
  // let the modules do whatevere they need with the session
  module_call_all_session_close(config->modules, session);

  // remove session from sessions list
  config->sessions = g_list_remove_all(config->sessions, session);

  // don't listen any more for data from server
  if (session->input_event_id != -1) {
    gtk_input_remove(session->input_event_id);
    session->input_event_id = -1;
  }
  // this also call telnet_free which will close the connection
  session_delete(session);

  // now remove the tab from notebook
  gtk_notebook_remove_page(notebook,
         gtk_notebook_page_num(notebook, tab)
      );
  if (gtk_notebook_get_n_pages(notebook) < 2) {
    gtk_notebook_set_show_tabs(notebook, FALSE);
  } else {
    gtk_notebook_set_show_tabs(notebook, TRUE);
  }
  
}



//return the session state associate with the tab which contains the wid
SESSION_STATE *interface_get_session(GtkWidget * wid)
{
  GtkWidget *tab;

  tab = interface_get_widget(wid, "session_tab");
  g_return_val_if_fail(tab != NULL, NULL);

  return (Session*) g_object_get_data(G_OBJECT(tab), "session");
}

void on_tabs_menu_activated(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *top = NULL;
  GtkWidget *notebook = NULL;
  GtkWidget *item = NULL;
  gint n, p;

  top = gtk_widget_get_toplevel(GTK_WIDGET(menuitem));
  if (!top)
    return;
  notebook = g_object_get_data(G_OBJECT(top), "notebook");
  if (notebook) {
    n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    p = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  } else {
    p = -1;   // make all the bellow tests to fail :)
    n = -2;
  }

  item =
      interface_get_widget((GtkWidget *) menuitem, "previous_tab1");
  if (p > 0) {
    gtk_widget_set_sensitive(item, TRUE);
  } else {
    gtk_widget_set_sensitive(item, FALSE);
  }

  item = interface_get_widget((GtkWidget *) menuitem, "next_tab1");
  if (p < n - 1) {
    gtk_widget_set_sensitive(item, TRUE);
  } else {
    gtk_widget_set_sensitive(item, FALSE);
  }

  item =
      interface_get_widget((GtkWidget *) menuitem, "move_tab_left1");
  if (p > 0) {
    gtk_widget_set_sensitive(item, TRUE);
  } else {
    gtk_widget_set_sensitive(item, FALSE);
  }

  item =
      interface_get_widget((GtkWidget *) menuitem,
         "move_tab_right1");
  if (p < n - 1) {
    gtk_widget_set_sensitive(item, TRUE);
  } else {
    gtk_widget_set_sensitive(item, FALSE);
  }

  item = interface_get_widget((GtkWidget *) menuitem, "detach_tab1");
  if (n > 1) {
    gtk_widget_set_sensitive(item, TRUE);
  } else {
    gtk_widget_set_sensitive(item, FALSE);
  }
}

void on_notebook_page_changed(GtkNotebook * notebook,
            GtkNotebookPage * page, guint page_num,
            gpointer user_data)
{
  GtkWidget *tab;
  GtkWidget *label;
  gint p;
  SESSION_STATE *session;

  p = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  tab = gtk_notebook_get_nth_page(notebook, p);

  label = gtk_notebook_get_tab_label(notebook, tab);
  if (label) {
    gtk_widget_modify_fg(label, GTK_STATE_ACTIVE, NULL);
  }

  session = g_object_get_data(G_OBJECT(tab), "session");
  if (session == NULL)
    return;         // no session attached yet
        cmd_entry_set_focus ();

        module_call_all_session_changed (get_configuration()->modules,
                                            session);

}

void on_quit1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  gtk_main_quit();
}

extern void internal_set_tab_label(GtkWidget * notebook, GtkWidget * tab);
void on_detach_tab1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *t1, *t2;
  if ((t1 = interface_get_active_window())) {
    GtkWidget *notebook;
    GtkWidget *tab;

    notebook =
        (GtkWidget *) g_object_get_data(G_OBJECT(t1),
                "notebook");
    g_return_if_fail(notebook != NULL);

    // don't detach a sigle tab
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) < 2) {
      return;
    }
    // get the tab ...
    tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
            gtk_notebook_get_current_page
            (GTK_NOTEBOOK(notebook))
        );

    if ((t2 = interface_add_window())) {
      gtk_widget_ref(tab);
      // ... remove it from current window ...
      gtk_notebook_remove_page(GTK_NOTEBOOK(notebook),
             gtk_notebook_get_current_page
             (GTK_NOTEBOOK(notebook))
          );
      // ... and put it in new created window
      interface_add_tab(t2, tab);
      gtk_widget_unref(tab);

      if (gtk_notebook_get_n_pages
          (GTK_NOTEBOOK(notebook)) < 2) {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK
                 (notebook),
                 FALSE);
      } else {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK
                 (notebook),
                 TRUE);
      }
      // update tab label in new window
      notebook =
          (GtkWidget *) g_object_get_data(G_OBJECT(t2),
                  "notebook");
      g_return_if_fail(notebook != NULL);
      internal_set_tab_label(notebook, tab);
    } else {
      g_warning
          ("[on_detach_tab1_activate] no window create.");
      return;
    }

  } else {
    g_warning("[on_detach_tab1_activate] no window active.");
    return;
  }
}


void on_previous_tab1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *wid;
  wid =
      (GtkWidget *)
      g_object_get_data(G_OBJECT(interface_get_active_window()),
            "notebook");
  if (wid)
    gtk_notebook_prev_page(GTK_NOTEBOOK(wid));
}

void on_next_tab1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *wid;
  wid =
      (GtkWidget *)
      g_object_get_data(G_OBJECT(interface_get_active_window()),
            "notebook");
  if (wid)
    gtk_notebook_next_page(GTK_NOTEBOOK(wid));
}

void on_move_tab_left1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *wid;
  GtkWidget *tab;
  gint8 n, t;
  wid =
      (GtkWidget *)
      g_object_get_data(G_OBJECT(interface_get_active_window()),
            "notebook");
  t = gtk_notebook_get_current_page(GTK_NOTEBOOK(wid));
  tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(wid), t);
  n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(wid));
  if (t > 0) {
    gtk_notebook_reorder_child(GTK_NOTEBOOK(wid), tab, t - 1);
  }

}

void on_move_tab_right1_activate(GtkMenuItem * menuitem,
         gpointer user_data)
{
  GtkWidget *wid;
  GtkWidget *tab;
  gint8 n, t;
  wid =
      (GtkWidget *)
      g_object_get_data(G_OBJECT(interface_get_active_window()),
            "notebook");
  t = gtk_notebook_get_current_page(GTK_NOTEBOOK(wid));
  tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(wid), t);
  n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(wid));
  gtk_notebook_reorder_child(GTK_NOTEBOOK(wid), tab, t + 1);
}



void on_none1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *toolbar;
  toolbar =
      interface_get_widget(GTK_WIDGET(menuitem), "toolbar_main");
  g_return_if_fail(toolbar != NULL);
  gtk_widget_hide(toolbar);
}

void on_icons1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *toolbar;
  toolbar =
      interface_get_widget(GTK_WIDGET(menuitem), "toolbar_main");
  g_return_if_fail(toolbar != NULL);
  gtk_widget_show(toolbar);

  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
}

void on_text1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *toolbar;
  toolbar =
      interface_get_widget(GTK_WIDGET(menuitem), "toolbar_main");
  g_return_if_fail(toolbar != NULL);
  gtk_widget_show(toolbar);

  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
}

void on_both1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *toolbar;
  toolbar =
      interface_get_widget(GTK_WIDGET(menuitem), "toolbar_main");
  g_return_if_fail(toolbar != NULL);
  gtk_widget_show(toolbar);

  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
}

/***************
 *Help windows
 ***************/
void on_about1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *wid;
        GtkWidget *label;
        wid = interface_create_object_by_name("dialog_about");
        g_return_if_fail(wid != NULL);
        gtk_dialog_run(GTK_DIALOG(wid));
        gtk_widget_destroy(wid);
}

void on_documentation1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  gchar *doc_file;
  char buf [512];
  char * argv [3] = {config->help_browser, buf, NULL};
  GError * err = NULL;

  doc_file = g_build_filename(mudmagic_data_directory(), "doc", "index.html", NULL);

  if (!g_file_test(doc_file, G_FILE_TEST_EXISTS)) 
  {
    g_message("%s not found", doc_file);
    g_free(doc_file);
    doc_file = g_build_filename("../../doc", "index.html", NULL);
    g_message("try with %s", doc_file);
  }

  //interface_display_file("Documentation", doc_file);
  g_snprintf (buf, 1024, "%s", doc_file);
  if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err)) {
	g_warning("Error starting external browser: %s\n", err->message);
	g_error_free (err);
  }
  g_free(doc_file);
}

void on_mudmagic_website1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  gchar *doc_file;
  char buf [512];
  char * argv [3] = {config->help_browser, buf, NULL};
  GError * err = NULL;

  g_snprintf(buf, 1024, "http://www.mudmagic.com/mud-client/boards");
  if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err)) {
        g_warning("Error starting external browser: %s\n", err->message);
        g_error_free (err);
  }
}

/***************
 *End Help windows
 ***************/

void on_download_cancel_clicked(GtkButton * button, gpointer user_data)
{
  GtkWidget *win;

  win = gtk_widget_get_toplevel(GTK_WIDGET(button));
  g_return_if_fail(win != NULL);

  g_object_set_data(G_OBJECT(win), "canceled", (gpointer) TRUE);

  //gtk_widget_destroy( win );
}

gboolean on_window_download_delete_event(GtkWidget * widget,
           GdkEvent * event,
           gpointer user_data)
{
  g_object_set_data(G_OBJECT(widget), "canceled", (gpointer) TRUE);
  return TRUE;
}

void on_output1_c_vscroll(GtkAdjustment * adjustment, gpointer tab)
{
  //GtkWidget *vpaned;
  GtkWidget *wid, *wid1;
//      GtkAdjustment *vadj;
//      gint position;

  wid = interface_get_widget(GTK_WIDGET(tab), "output2");
  wid1 = interface_get_widget(GTK_WIDGET(tab), "output1");
  g_return_if_fail(wid != NULL);
  g_return_if_fail(wid1 != NULL);


  if (adjustment->value + adjustment->page_size != adjustment->upper) {
    if (GTK_WIDGET_VISIBLE(wid) == FALSE) {
      GtkRequisition r;
      GtkAllocation a;
      gtk_widget_size_request(wid1, &r);
      gtk_widget_show(wid);
      a.width = r.width;
      a.height = r.height / 4;
      a.x = 0;
      a.y = r.height * 3 / 4;
      gtk_widget_size_allocate(wid, &a);

//                      gtk_adjustment_set_value( vadj, gtk_adjustment_get_value(adjustment) );
//                      gtk_adjustment_value_changed( vadj );
    }
  } else
    gtk_widget_hide(wid);
}

void on_output1_c_size_allocate(GtkWidget * widget,
        GtkAllocation * allocation,
        gpointer user_data)
{
  SESSION_STATE *session;
  gint oldw, oldh, w, h;
  session = interface_get_session(widget);
  g_return_if_fail(session != NULL);
  if (session->telnet->naws) {
    interface_get_output_size(session, &w, &h);
    oldw =
        GPOINTER_TO_INT(g_object_get_data
            (G_OBJECT(widget), "width"));
    oldh =
        GPOINTER_TO_INT(g_object_get_data
            (G_OBJECT(widget), "height"));
    g_message("Blaat: w=%d h=%d oldw=%d oldh=%d\n", w, h, oldw,
        oldh);
    if (w != oldw || h != oldh) {
      telnet_send_window_size(session->telnet, w, h);
      g_object_set_data(G_OBJECT(widget), "width",
            GINT_TO_POINTER(w));
      g_object_set_data(G_OBJECT(widget), "height",
            GINT_TO_POINTER(h));
    }
  }
}

void on_tab_command_activate(GtkMenuItem * menuitem, gpointer tab)
{
  SESSION_STATE *session;
  g_return_if_fail(tab != NULL);
  session = g_object_get_data(G_OBJECT(tab), "session");
  g_return_if_fail(session != NULL);
  if (strcmp(GTK_WIDGET(menuitem)->name, "tab_close") == 0) {
    interface_remove_tab(tab);
    return;
  }
  if (strcmp(GTK_WIDGET(menuitem)->name, "tab_disconnect") == 0) {
    interface_tab_disconnect(tab);
    return;
  }
  if (strcmp(GTK_WIDGET(menuitem)->name, "tab_reconnect") == 0) {
    interface_tab_disconnect(tab);
    interface_tab_connect(tab);
    return;
  }
}
gboolean on_eventbox_tab_button_press_event(GtkWidget * widget,
              GdkEventButton * event,
              gpointer tab)
{
  GtkWidget *wid;
  GtkWidget *item;

  if (event->button == 3) {
    // create a submenu
    wid = interface_create_object_by_name("menu_tab");
    g_return_val_if_fail(wid != NULL, FALSE);
    // set handlers
    item = interface_get_widget(wid, "tab_close");
    g_return_val_if_fail(item != NULL, FALSE);
    g_signal_connect((gpointer) item, "activate",
         G_CALLBACK(on_tab_command_activate), tab);
    item = interface_get_widget(wid, "tab_reconnect");
    g_signal_connect((gpointer) item, "activate",
         G_CALLBACK(on_tab_command_activate), tab);
    g_return_val_if_fail(item != NULL, FALSE);
    item = interface_get_widget(wid, "tab_disconnect");
    g_signal_connect((gpointer) item, "activate",
         G_CALLBACK(on_tab_command_activate), tab);
    g_return_val_if_fail(item != NULL, FALSE);


    gtk_menu_popup(GTK_MENU(wid), NULL, NULL, NULL, NULL,
             event->button, event->time);
  } else {
    GtkWidget *notebook, *current_tab, *label;
    gint n;
    notebook = gtk_widget_get_ancestor(tab, GTK_TYPE_NOTEBOOK);
    g_return_val_if_fail(notebook != NULL, TRUE);
    n = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), tab);
    if (n != -1) {
      gtk_notebook_set_current_page(GTK_NOTEBOOK
                  (notebook), n);
    current_tab =
        gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);

    if (current_tab) {
      label =
          gtk_notebook_get_tab_label(GTK_NOTEBOOK
                   (notebook),
                   GTK_WIDGET(current_tab));
      if (label)
      {
        GtkWidget* icon;
        icon = g_object_get_data (G_OBJECT (label), "label_icon");
        gtk_image_set_from_stock (GTK_IMAGE (icon),
                GTK_STOCK_YES,
                GTK_ICON_SIZE_MENU); 
      }
    }
    }
  }
  return TRUE;
}

//handles the changing of the icon colors when text is changed and 
//no focus on window
gboolean on_window_main_focus_in_event(GtkWidget * widget,
               GdkEventFocus * event,
               gpointer user_data)
{
  gchar *icon;
  gtk_window_set_title(GTK_WINDOW(widget), "MudMagic");
  icon = g_build_filename(mudmagic_data_directory(), "interface", "mudmagic.xpm", NULL);
  gtk_window_set_icon_from_file(GTK_WINDOW(widget), icon, NULL);
  g_free(icon);
  return FALSE;
}

gboolean on_window_main_focus_out_event(GtkWidget * widget,
               GdkEventFocus * event,
               gpointer user_data)
{
  return FALSE;
}


#define FRAME_DELAY 100
#define BACKGROUND_NAME "background.xpm"

static const char *image_names[] = {
  "apple-red.png",
  "mudmagic.png",
  "nymph.png",
  "gnome-foot.png",
  "gnome-gmush.png",
  "version.png",
  "gnome-gsame.png",
  "gnu-keys.png"
};
static const char * featured_img_file = "mmenterstar.png";

#define N_IMAGES G_N_ELEMENTS (image_names)

/* Current frame */
static GdkPixmap *frame = NULL;

/* Background tiled image */
static GdkPixmap *background = NULL;

/* Images */
static GdkPixbuf *images[N_IMAGES];

/* featured game icon placed in center */
static GdkPixbuf * featured = NULL;
int featured_width2 = 0;
int featured_height2 = 0;


/* Loads the images for the demo and returns whether the operation succeeded */
static gboolean load_pixbufs(GtkWidget * window)
{
  gint i;
  char *filename;

  if (background)
    return TRUE;  /* already loaded earlier */

  filename = g_build_filename(mudmagic_data_directory(), "interface", BACKGROUND_NAME, NULL);

  if (!filename) {
    g_warning("Background image not found");
    return FALSE;
  }

  background =
      gdk_pixmap_create_from_xpm(window->window, NULL, NULL,
               filename);
  g_free(filename);

  if (!background) {
    g_warning("No background");
    return FALSE;
  }

	filename = g_build_filename (mudmagic_data_directory (), "interface", featured_img_file, NULL);
    if (!filename) {
		g_warning ("No file %s", featured_img_file);
		return FALSE;
	}
	featured = gdk_pixbuf_new_from_file (filename, NULL);
	g_free(filename);
	if (!featured) {
		g_warning ("No image 'featured'");
		return FALSE;
	}
	featured_width2 = gdk_pixbuf_get_width (featured) / 2;
	featured_height2 = gdk_pixbuf_get_height (featured) / 2;


  for (i = 0; i < N_IMAGES; i++) {
    filename =
        g_build_filename(mudmagic_data_directory(), "interface", image_names[i],
             NULL);
    if (!filename) {
      g_warning("No file %s", image_names[i]);
      return FALSE;
    }

    images[i] = gdk_pixbuf_new_from_file(filename, NULL);
    g_free(filename);

    if (!images[i]) {
      g_warning("No images-i");
      return FALSE;
    }
  }

  return TRUE;
}

#define CYCLE_LEN 60

static int frame_num;

static void redraw_frame(GtkWidget * da)
{
  double f;
  int i;
  double xmid, ymid;
  double radius;
  gint width, height;
  gint pw, ph;
  static GdkGC *tiled_gc = 0;

  f = (double) (frame_num % CYCLE_LEN) / CYCLE_LEN;

  gdk_drawable_get_size(da->window, &width, &height);

  if (frame) {
    //destroy old frame if window size changed
    gdk_drawable_get_size(frame, &pw, &ph);
    if ((pw != width) || (ph != height)) {
      g_object_unref(frame);
      frame = 0;
    }
  }
  if (frame == 0) {
    GdkColormap *cm = gdk_colormap_get_system();
    frame =
        gdk_pixmap_new(0, width, height, cm->visual->depth);
    gdk_drawable_set_colormap(frame, cm);
  }

  xmid = width / 2.0;
  ymid = height / 2.0;

  if (!tiled_gc) {
    tiled_gc = gdk_gc_new(frame);
    gdk_gc_set_tile(tiled_gc, background);
    gdk_gc_set_fill(tiled_gc, GDK_TILED);
  }

  gdk_draw_rectangle(frame, tiled_gc, TRUE, 0, 0, width, height);

	gdk_draw_pixbuf (
		frame, 0, featured, 0, 0, 
		xmid - featured_width2, ymid - featured_height2, -1, -1,
		GDK_RGB_DITHER_NORMAL, 0, 0
	);

  radius = MIN(xmid, ymid) / 2.0;

  for (i = 0; i < N_IMAGES; i++) {
    double ang;
    int xpos, ypos;
    int iw, ih;
    double r;
    GdkRectangle r1, r2, dest;
    double k;

    ang = 2.0 * G_PI * (double) i / N_IMAGES - f * 2.0 * G_PI;

    iw = gdk_pixbuf_get_width(images[i]);
    ih = gdk_pixbuf_get_height(images[i]);

    r = radius + (radius / 3.0) * sin(f * 2.0 * G_PI);

    xpos = floor(xmid + r * cos(ang) - iw / 2.0 + 0.5);
    ypos = floor(ymid + r * sin(ang) - ih / 2.0 + 0.5);

/*
    k = (i & 1) ? sin (f * 2.0 * G_PI) : cos (f * 2.0 * G_PI);
    k = 2.0 * k * k;
    k = MAX (0.25, k);
*/
    k = 1;


    r1.x = xpos;
    r1.y = ypos;
    r1.width = iw * k;
    r1.height = ih * k;

    r2.x = 0;
    r2.y = 0;
    r2.width = width;
    r2.height = height;

    if (gdk_rectangle_intersect(&r1, &r2, &dest))
      gdk_draw_pixbuf(frame, 0, images[i], 0, 0, dest.x,
          dest.y, dest.width, dest.height,
          GDK_RGB_DITHER_NORMAL, 0, 0);
/*
      gdk_pixbuf_composite (images[i],
                  frame,
                  dest.x, dest.y,
                  dest.width, dest.height,
                  xpos, ypos,
                  k, k,
                  GDK_INTERP_NEAREST,
                  ((i & 1)
                  ? MAX (127, fabs (255 * sin (f * 2.0 * G_PI)))
                  : MAX (127, fabs (255 * cos (f * 2.0 * G_PI))) ) );
*/
  }
}

/* Expose callback for the drawing area */
static gint
on_intro_expose_event(GtkWidget * widget, GdkEventExpose * event,
          gpointer data)
{
  static GdkGC *gc = 0;
  if (!gc)
    gc = gdk_gc_new(widget->window);

  gint width = 0, height = 0, pw, ph;

  if (!frame)
    return FALSE;

  gdk_drawable_get_size(widget->window, &width, &height);

  // Recreate the pixbuf if the size of the drawable has changed while the redraw from
  // timeout sat in the event queue
  gdk_drawable_get_size(frame, &pw, &ph);
  if ((pw != width) || (ph != height))
    redraw_frame(widget);

  // gdk_draw_rectangle(widget->window, tiled_gc, TRUE, 0, 0, width, height);

  gdk_draw_drawable(widget->window, gc, frame, event->area.x,
        event->area.y, event->area.x, event->area.y,
        event->area.width, event->area.height);

/*  gdk_draw_pixbuf (widget->window,
           tiled_gc,
           frame,
           0, 0,
           event->area.x, event->area.y,
           event->area.width, event->area.height,
           GDK_RGB_DITHER_NORMAL,
           event->area.x, event->area.y); */

  return TRUE;
}

/* Timeout handler to regenerate the frame */
static gint on_intro_timeout(GtkWidget * da)
{
  int width, height, xmid, ymid, radius;
  static char first_draw = 1;

  if (da == NULL)
    return FALSE;

  redraw_frame(da);

  gdk_drawable_get_size(da->window, &width, &height);
  xmid = width / 2;
  ymid = height / 2;

  radius = MIN(xmid, ymid);

  if (first_draw) {
    //draw everything on first draw
    first_draw = 0;
    gtk_widget_queue_draw(da);
  } else
    //only repaint changing parts on subsequent draws
    gtk_widget_queue_draw_area(da, xmid - radius,
             ymid - radius, 2 * radius,
             2 * radius);
  frame_num++;
  return TRUE;
}

static guint timeout_id;

static gboolean intro_event_after (GtkWidget * wid, GdkEvent  * ev) {
	GdkEventButton *event;

	if (ev->type != GDK_BUTTON_RELEASE) return FALSE;
	event = (GdkEventButton *) ev;
	if (event->button != 1) return FALSE;
	if (
		(featured_width2 > abs (wid->allocation.width / 2 - (int) event->x)) &&
		(featured_height2 > abs (wid->allocation.height / 2 - (int) event->y))
	) {
		sessions_create_new_char_intf ("radio_featured");
	}
	return TRUE;
}

void on_intro_show(GtkWidget * drawable, gpointer data) {
	GtkWidget * eb, * top;

	if (load_pixbufs(drawable)) {
		top = gtk_widget_get_toplevel (drawable);
		eb = interface_get_widget (top, "eventbox_intro");
		gtk_widget_show_all (eb);
		g_signal_connect (drawable, "expose_event", G_CALLBACK(on_intro_expose_event), NULL);
		g_signal_connect (eb, "event-after", G_CALLBACK (intro_event_after), NULL);
		timeout_id = g_timeout_add (FRAME_DELAY, (GtkFunction) on_intro_timeout, drawable);
	} else {
		g_message ("Unable to load the pixbufs, dropping to a default entry");
	}
}

void on_intro_hide(GtkWidget * drawable, gpointer data)
{
	GtkWidget * eb, * top;
	top = gtk_widget_get_toplevel (drawable);
	eb = interface_get_widget (top, "eventbox_intro");
	gtk_widget_hide_all (eb);
  gtk_timeout_remove(timeout_id);
  timeout_id = 0;
}
