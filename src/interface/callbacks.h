/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* callbacks.h:                                                            *
*                2004 Calvin Ellis  ( kyndig@mudmagic.com )               *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef CALLBACKS_H
#define CALLBACKS_H 1
#include <gtk/gtk.h>
#include <mudmagic.h>

GtkWidget *interface_get_widget(GtkWidget * wid, gchar * name);
GtkWidget *interface_create_object_by_name(gchar * name);
GtkWidget *interface_add_tab(GtkWidget * window, GtkWidget * tab);
void interface_remove_tab(GtkWidget * tab);

void interface_tab_refresh(GtkWidget * tab);

SESSION_STATE *interface_get_session(GtkWidget * wid);

// initialize ANSI state. Didn't find any better place for this, so it's here
void initialize_ansi(SESSION_STATE * session);

void on_notebook_page_changed(GtkNotebook * notebook,
            GtkNotebookPage * page, guint page_num,
            gpointer user_data);
void on_quit1_activate(GtkMenuItem * menuitem, gpointer user_data);

// tabs callbacks
void on_previous_tab1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_next_tab1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_move_tab_left1_activate(GtkMenuItem * menuitem,
        gpointer user_data);
void on_move_tab_right1_activate(GtkMenuItem * menuitem,
         gpointer user_data);
void on_detach_tab1_activate(GtkMenuItem * menuitem, gpointer user_data);

// toolbar callbacks
void on_tabs_menu_activated(GtkMenuItem * menuitem, gpointer user_data);
void on_move_tab_right1_activate(GtkMenuItem * menuitem,
         gpointer user_data);
void on_none1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_icons1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_text1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_both1_activate(GtkMenuItem * menuitem, gpointer user_data);

/*
 *help menu items
 */
void on_documentation1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_mudmagic_website1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_about1_activate(GtkMenuItem * menuitem, gpointer user_data);

/* 
 * Theme selection window 
 */
void on_theme_select1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_theme_reset_button_clicked(GtkMenuItem * menuitem, gpointer user_data);
void on_theme_cancel_button_enter(GtkMenuItem * menuitem, gpointer user_data);
void on_theme_ok_button_clicked(GtkMenuItem * menuitem, gpointer user_data);


// data related callbacks
void on_data_available(gpointer tab, gint fd, GdkInputCondition cond);
void on_input1_activate(GtkEntry * entry, gpointer user_data);
void on_button_send_clicked(GtkButton * button, gpointer user_data);
gchar *internal_key_to_string(gint state, gint key);
gboolean on_input_key_press_event(GtkWidget * widget, GdkEventKey * event,
          gpointer user_data);
gboolean on_input2_key_press_event(GtkWidget * widget, GdkEventKey * event,
           gpointer user_data);


// session related callbacks
void on_character1_menu_activated(GtkMenuItem * menuitem,
          gpointer user_data);
void on_new1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_quick_connect_1_activate(GtkMenuItem * menuitem,
         gpointer user_data);
void on_reconnect1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_toggle_ml_toggled(GtkToggleButton * togglebutton,
        gpointer user_data);
void on_session1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_open1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_new_char_create_clicked(GtkButton * button, gpointer user_data);
void on_new_char_update_clicked(GtkButton * button, gpointer data);
void on_saved_games_load_clicked(GtkButton * button, gpointer user_data);
void on_saved_games_new_clicked(GtkButton * button, gpointer user_data);
void on_button_reconnect_clicked(GtkButton * button, gpointer user_data);
void on_saved_games_delete_clicked(GtkButton * button, gpointer user_data);

//void on_button_conf_alias_remove_clicked(GtkButton * button,
//           gpointer user_data);
//void on_button_conf_alias_add_clicked(GtkButton * button,
//              gpointer user_data);

//void on_button_trigger_remove_clicked(GtkButton * button,
//              gpointer user_data);
//void on_button_trigger_add_clicked(GtkButton * button, gpointer user_data);
//void on_button_conf_trigger_remove_clicked(GtkButton * button,
//             gpointer user_data);
//void on_button_conf_trigger_add_clicked(GtkButton * button,
//          gpointer user_data);

//void on_button_conf_macro_add_clicked(GtkButton * button,
//              gpointer user_data);
//void on_button_conf_macro_remove_clicked(GtkButton * button,
//           gpointer user_data);
//void on_button_macro_add_clicked(GtkButton * button, gpointer user_data);
//void on_button_macro_remove_clicked(GtkButton * button,
//            gpointer user_data);

void on_button_statusvar_remove_clicked(GtkButton * button,
                                      gpointer user_data);
void on_button_statusvar_add_clicked(GtkButton * button, gpointer user_data);
void on_button_gauge_remove_clicked(GtkButton * button,
                                      gpointer user_data);
void on_button_gauge_add_clicked(GtkButton * button, gpointer user_data);

void on_configuration1_activate(GtkMenuItem * menuitem,
        gpointer user_data);

void on_button_macro_capture_clicked(gpointer entry, GtkButton * button);
gboolean on_entry_macro_expr_key_press_event(gpointer button,
               GdkEventKey * event,
               GtkWidget * widget);

void on_close1_activate(GtkMenuItem * menuitem, gpointer user_data);

// modules related callbacks
void on_modules1_activate(GtkMenuItem * menuitem, gpointer user_data);

// donwload window
void on_download_cancel_clicked(GtkButton * button, gpointer user_data);
gboolean on_window_download_delete_event(GtkWidget * widget,
           GdkEvent * event,
           gpointer user_data);

// split window
void on_output1_c_vscroll(GtkAdjustment * adjustment, gpointer tab);
void on_output2_c_vscroll(GtkAdjustment * adjustment, gpointer tab);
void on_output1_c_size_allocate(GtkWidget * widget,
        GtkAllocation * allocation,
        gpointer user_data);

// right click on a tab
gboolean on_eventbox_tab_button_press_event(GtkWidget * widget,
              GdkEventButton * event,
              gpointer user_data);

// restore window title
gboolean on_window_main_focus_in_event(GtkWidget * widget,
               GdkEventFocus * event,
               gpointer user_data);
             
gboolean on_window_main_focus_out_event(GtkWidget * widget,
               GdkEventFocus * event,
               gpointer user_data);

// intro eye-candy initialize/end
void on_intro_show(GtkWidget * drawable, gpointer data);
void on_intro_hide(GtkWidget * drawable, gpointer data);

// settings
void on_profile_menu_cb_toggled(GtkCheckMenuItem * checkmenuitem,
        gpointer data);
void on_button_browse_clicked(gpointer entry, GtkButton * button);
void on_macro_button_clicked(GtkWidget * button, gpointer data);

//void on_treeview_triggers_list_selection_changed(GtkTreeSelection *
//             selection, gpointer data);
//void on_treeview_macros_list_selection_changed(GtkTreeSelection *
//                 selection, gpointer data);
void on_treeview_statusvars_list_selection_changed(GtkTreeSelection *
    selection, gpointer data);
void on_treeview_gauges_list_selection_changed(GtkTreeSelection *
    selection, gpointer data);

// profile
//void on_aliases_1_activate(GtkMenuItem * menuitem, gpointer user_data);
//void on_triggers_1_activate(GtkMenuItem * menuitem, gpointer user_data);
//void on_macros_1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_profile_actions_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_status_variables_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_gauges_activate(GtkMenuItem * menuitem, gpointer user_data);
// global
void on_aliases_2_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_triggers_2_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_macros_2_activate(GtkMenuItem * menuitem, gpointer user_data);

void on_profile_menu_activated(GtkMenuItem * menuitem, gpointer user_data);
void on_preferences_1_activate(GtkMenuItem * menuitem, gpointer user_data);
void on_configuration_1_activate(GtkMenuItem * menuitem,
         gpointer user_data);
         
gboolean on_saved_games_treeview_button_press_event (GtkWidget *widget,
        GdkEventButton *event, gpointer func_data);

//gboolean
//on_run_atm_button_press_event (GtkWidget* source,
//        GdkEventButton* event, gpointer user_data);

gboolean gaugebar_expose (GtkWidget *widget, GdkEventExpose *event,
    gpointer data);

/* --- tools --- */
/* common */
void on_tools_menu_activated (GtkMenuItem * menuitem, gpointer user_data);

void on_tools_common_open (GtkButton * button, gpointer user_data);
void on_tools_common_save (GtkButton * button, gpointer user_data);
void on_tools_common_button_clear (GtkButton * button, gpointer user_data);
void on_tools_common_button_cancel (GtkButton * button, gpointer user_data);

// scripts testing
void on_scripts_testing_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_scripts_testing_button_ok (GtkButton * button, gpointer user_data);
// triggers and aliases testing
void on_ta_testing_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_ta_testing_button_ok (GtkButton * button, gpointer user_data);
// long text passing
void on_lt_passing_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_lt_passing_button_ok (GtkButton * button, gpointer user_data);
void on_tools_lt_passing_open (GtkButton * button, gpointer user_data);
void on_tools_lt_passing_save (GtkButton * button, gpointer user_data);
void on_tools_lt_passing_button_clear (GtkButton * button, gpointer user_data);
// viewing log
void on_log_view_activate (GtkMenuItem * menuitem, gpointer user_data);
// delayed commands
void on_delayed_cmd_activate (GtkMenuItem * menuitem, gpointer user_data);
void on_tools_delayed_commands_add (GtkButton * button, gpointer user_data);
void on_tools_delayed_commands_del (GtkButton * button, gpointer user_data);
// export/import settings
void on_tools_remote_storage (GtkMenuItem * menuitem, gpointer user_data);

#endif        // CALLBACKS_H
