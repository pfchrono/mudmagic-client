/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* interface.h:                                                            *
*                2004  Calvin Ellis    ( kyndig@mudmagic.com )            *
*                2005  Mart Raudsepp   ( leio@users.sf.net   )            *
*                2005  Tomas Mecir     ( kmuddy@kmuddy.net   )            *
*                2005  Shlykov Vasiliy ( vash@zmail.ru       )            *
*                                                                         *
***************************************************************************/
/**************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef __INTERFACE_H__MUDMAGIC
#define __INTERFACE_H__MUDMAGIC
#include <gtk/gtk.h>
#include <mudmagic.h>
#include <mudaccel.h>
#include "callbacks.h"
#include "statusvars.h"
#include "gauges.h"

// length of a string array
// because Gtk+ 2.4 doesn't has g_strv_length
int strv_length(gchar ** str_array);

GtkWidget *interface_get_active_tab();
GtkWidget *interface_get_active_window();
SESSION_STATE *interface_get_active_session();
GtkWidget *interface_get_main_toolbar (void);

void interface_display_message(gchar * message);
void interface_display_file(gchar * title, gchar * filename);

GtkWidget *interface_add_window();
void interface_remove_window(GtkWidget * window);

// return the right area from a tab( can be used by a module )
GtkWidget *interface_tab_get_area_right(GtkWidget * tab);

// download progress window related functions
struct _HttpHelper* httphelper_new (const gchar* title);
void httphelper_free (struct _HttpHelper* hh);

//gpointer interface_download_new(const gchar * title, const gchar * message);
//void interface_download_update(gpointer win, gsize current, gsize total);
//void interface_download_free(gpointer win);
//gboolean interface_download_iscanceled(gpointer win);

// tab disconnect/connect/functions
void interface_tab_connect(GtkWidget * tab);
void interface_tab_disconnect(GtkWidget * tab);

// hide/show user input
void interface_input_shadow(SESSION_STATE * session, gboolean shadow);

void interface_get_output_size(SESSION_STATE * session, gint * width,
             gint * height);

void interface_triggers_edit(GList ** triggers, const gchar* title, const gchar* body);
void interface_macros_edit(GList ** macros, const gchar* title, const gchar* body);
void interface_statusvars_edit(SVLIST *statusvars, const gchar* title);
void interface_gauges_edit(GAUGELIST *gauges, const gchar* title);

// send a command
void send_command(SESSION_STATE * session, char *buff, gsize len);

// add an image, used by MXP handler
void interface_image_add(GtkWidget * tab, GtkTextIter * iter,
       GdkPixbuf * pixbuf);
// get iterator pointing to end of current data - used to add image later on
GtkTextIter interface_get_current_position(SESSION_STATE * session);

void on_cb_cmd_save_history_toggled (GtkToggleButton *togglebutton, gpointer user_data);

void interface_show_script_errors (ATM* at, const gchar* usermsg);
void interface_show_gerrors (GList* list, const gchar* usermsg);
void interface_show_broken_installation (void);
void interface_show_error (MudError* error, const gchar* usermsg);
#define interface_show_errors interface_show_gerrors

gint interface_remove_empty_slot (const gchar* slot);

gint interface_messagebox (gint type, gint buttons, const gchar* fmt, ...);
//#define GET_REAL_ENTRY(obj) gtk_entry_completion_get_entry(GTK_ENTRY_COMPLETION(obj))

void interface_add_global_accel_group (MudAccelGroup* accel);
void interface_remove_global_accel_group (MudAccelGroup* accel);

void interface_open_local_macros (const gchar* body);
void interface_open_local_aliases (const gchar* body);
void interface_open_local_triggers (const gchar* body);
void interface_open_local_statusvars ();
void interface_open_local_gauges ();

void interface_open_global_macros (const gchar* body);
void interface_open_global_aliases (const gchar* body);
void interface_open_global_triggers (const gchar* body);

void interface_run_atm (Session* session, ATM* atm,
        const gchar* backrefs[], gsize nbackrefs);

void interface_modules_init (GList* modules);

#endif
// INTERFACE_H
