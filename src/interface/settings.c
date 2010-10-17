/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* settings.c:                                                             *
*                2004  Calvin Ellis    ( kyndig@mudmagic.com )            *
*                2005  Shlykov Vasiliy ( vash@zmail.ru )                  *
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
#include <gtk/gtk.h>
#include <mudmagic.h>
#include <log.h>
#include <utils.h>
#include <alias_triggers.h>
#include "interface.h"
#include "cmdentry.h"
#include <statusvars.h>
#include <gauges.h>
#include "proxy.h"

typedef struct _ConfigAtmPage ConfigAtmPage;

struct _ConfigAtmPage
{
    GtkEntry*         raiser_entry;
    GtkEntry*         name_entry;
    GtkEntry*         filename_entry;
    GtkToggleButton*  external_button;
    GtkToggleButton*  internal_button;
    GtkTextView*      script_code;
    GtkTreeView*      atm_treeview;
};

extern CONFIGURATION *config;

void on_profile_menu_cb_toggled(GtkCheckMenuItem * checkmenuitem,
        gpointer data)
{
  SESSION_STATE *session;
  const gchar *name;
  gboolean value, save = FALSE;
  name = gtk_widget_get_name(GTK_WIDGET(checkmenuitem));
  value = gtk_check_menu_item_get_active(checkmenuitem);

  session = interface_get_active_session();
  g_return_if_fail(session != NULL);

  if (strcmp(name, "menuitem_echo") == 0) {
    if (value != session->local_echo)
      save = TRUE;
    session->local_echo = value;
  }
  if (strcmp(name, "menuitem_sound") == 0) {
    if (value != session->sound)
      save = TRUE;
    session->sound = value;
  }
  if (strcmp(name, "menuitem_logging") == 0) {
    if (value != session->logging)
      save = TRUE;
    session->logging = value;

    if (session->logging) {
      session->log_file =
          log_open_logfile(session->slot);
    } else {
      if (session->log_file) {
        log_close_logfile(session->log_file);
        session->log_file = NULL;
      }
    }
  }
  if (save)
    session_save(session);
}

void on_profile_menu_activated(GtkMenuItem * menuitem, gpointer user_data)
{
  SESSION_STATE *session;
  GtkWidget *menu, *item;

  menu = gtk_menu_item_get_submenu(menuitem);
  g_return_if_fail(menu);

  session = interface_get_active_session();
  if (session == NULL) {
    gtk_container_foreach(GTK_CONTAINER(menu),
              (GtkCallback)
              gtk_widget_set_sensitive,
              GINT_TO_POINTER(FALSE)
        );
    return;
  }

  gtk_container_foreach(GTK_CONTAINER(menu),
            (GtkCallback) gtk_widget_set_sensitive,
            GINT_TO_POINTER(TRUE)
      );

  item =
      interface_get_widget((GtkWidget *) menuitem, "menuitem_echo");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
               session->local_echo);

  item =
      interface_get_widget((GtkWidget *) menuitem, "menuitem_sound");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
               session->sound);

  item =
      interface_get_widget((GtkWidget *) menuitem,
         "menuitem_logging");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
               session->logging);
}

void interface_show_script_errors (ATM* at, const gchar* usermsg)
{
  GtkWidget* master,
     * dialog;

        gchar*     msg;

  g_assert (at);

        if (at->errors == NULL)
            return;

        msg = utils_join_strs (at->errors, "\n");

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

        g_free (msg);
}

/* user clicks on macro button of toolbar */
void on_macro_button_clicked(GtkWidget * button, gpointer data)
{
  SESSION_STATE *session;
  ATM           *script = data;

  session = g_object_get_data(G_OBJECT(button), "session");

        interface_run_atm (session, script, NULL, 0);
        cmd_entry_set_focus ();
}

void store_file_in_entry(gpointer entry, GtkWidget * button)
{
  GtkWidget *filed;

  filed = gtk_widget_get_ancestor(button, GTK_TYPE_WINDOW);
  gtk_entry_set_text(GTK_ENTRY(entry),
         gtk_file_selection_get_filename
         (GTK_FILE_SELECTION(filed))
      );
}

static GtkFileFilter** get_script_filters (gsize* length)
{
    static gboolean init = FALSE;

    static GtkFileFilter* filters[] =
    {
        NULL, // All script files
        NULL, // BASIC filter
        NULL, // Python filter
        NULL  // All files
    };

    static gsize filters_size = sizeof (filters) / sizeof (filters[0]);

    g_assert (length);

//    if (!init)
    {
        filters[0] = gtk_file_filter_new ();
        gtk_file_filter_set_name (filters[0], "All scripts files");
        gtk_file_filter_add_pattern (filters[0], "*.bas");
        gtk_file_filter_add_pattern (filters[0], "*.py");

        filters[1] = gtk_file_filter_new ();
        gtk_file_filter_set_name (filters[1], "BASIC scripts");
        gtk_file_filter_add_pattern (filters[1], "*.bas");

        filters[2] = gtk_file_filter_new ();
        gtk_file_filter_set_name (filters[2], "Python scripts");
        gtk_file_filter_add_pattern (filters[2], "*.py");

        filters[3] = gtk_file_filter_new ();
        gtk_file_filter_set_name (filters[3], "All files");
        gtk_file_filter_add_pattern (filters[3], "*");

        init = TRUE;
    }

    *length = filters_size;
    return filters;
}


// open a file chooser and put selected file in entry  
void on_button_browse_clicked(gpointer entry, GtkButton * button)
{
    GtkWidget*      dialog;
    GtkWidget*      top;
    gchar*          filename;
    gsize           i, flen;
    GtkFileFilter** filters;
    GtkFileChooser* chooser;
    Configuration*  cfg = get_configuration ();
    Session*        session = interface_get_active_session ();

    g_return_if_fail( entry != NULL );

    top = gtk_widget_get_toplevel (GTK_WIDGET (button));
    dialog = gtk_file_chooser_dialog_new (
                                            "Select Script File", 
                                            GTK_WINDOW(top),
                                            GTK_FILE_CHOOSER_ACTION_OPEN, 
                                            GTK_STOCK_CANCEL,
                                            GTK_RESPONSE_CANCEL, 
                                            GTK_STOCK_OPEN,
                                            GTK_RESPONSE_ACCEPT, 
                                            NULL
                                         );
    chooser = GTK_FILE_CHOOSER (dialog);

    if (cfg != NULL)
    {
        gtk_file_chooser_add_shortcut_folder (chooser, cfg->gamedir, NULL);
        gtk_file_chooser_set_current_folder (chooser, cfg->gamedir);
    }
    if (session != NULL)
    {
        gtk_file_chooser_add_shortcut_folder (chooser, session->slot, NULL);
        gtk_file_chooser_set_current_folder (chooser, session->slot);
    }

    filters = get_script_filters (&flen);
    for (i = 0; i < flen; i++)
    {
        g_assert (filters[i]);
        gtk_file_chooser_add_filter (chooser, filters[i]);
        if (i == 0)
            gtk_file_chooser_set_filter (chooser, filters[i]);
    }

    {
        const gchar* fname = gtk_entry_get_text (GTK_ENTRY (entry));
        if (strcmp (fname, ""))
        {
            gtk_file_chooser_set_filename (chooser, fname);
        }
    }

    if ( gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT )
    {
        filename = gtk_file_chooser_get_filename (chooser);
        gtk_entry_set_text (GTK_ENTRY (entry), filename);
        g_free (filename);
    }

    gtk_widget_destroy (dialog);
}

const gchar* config_minimize_script_path (gint atm_type, Session* session, const gchar* file)
{
    const gchar* sdir = atm_get_config_subdir (get_configuration(), atm_type);
    const gchar* basepath,
               * ret = NULL;
    gchar*       subpath;

    basepath = (session != NULL) ? session->slot : get_configuration()->gamedir;
    subpath = g_build_path (G_DIR_SEPARATOR_S, basepath, sdir, NULL);

    ret = utils_check_subpath (subpath, file);
    //g_print ("Base path: %s\nPath: %s\nResult: %s\n", subpath, file, ret);

    g_free (subpath);
    return ret;
}

void on_button_statusvar_add_clicked(GtkButton * button, gpointer user_data)
{
  GtkWidget*    dialog;
  Session*      session;
  SVLIST *svlist;
  GtkEntry *_var, *_maxvar;
  GtkCheckButton *chkpercent;
  gchar *var, *maxvar;
  gboolean percentage;

  dialog = gtk_widget_get_toplevel (GTK_WIDGET(button));
  g_return_if_fail (dialog != NULL);

  session = interface_get_active_session();
  g_return_if_fail (session != NULL);

  _var = GTK_ENTRY (interface_get_widget (dialog, "entry_statusvar_variable"));
  _maxvar = GTK_ENTRY (interface_get_widget (dialog, "entry_statusvar_maxvariable"));
  chkpercent = GTK_CHECK_BUTTON (interface_get_widget (dialog, "chkpercent"));
  
  var = (gchar *) gtk_entry_get_text (_var);
  maxvar = (gchar *) gtk_entry_get_text (_maxvar);
  percentage = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chkpercent));
  svlist = g_object_get_data (G_OBJECT (dialog), "statusvars_list");

  if (!strcmp (var, ""))
  {
    interface_display_message("Variable name can not be empty.");
    return;
  }

  if (!svlist_exists (svlist, var)) {
    GtkWidget *treeview;
    GtkListStore *store;
    GtkTreeIter  iter;
    treeview = interface_get_widget(dialog, "treeview_statusvars_list");
    store = (GtkListStore *) gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, var, -1);
  }

  svlist_set_statusvar (svlist, var, maxvar, percentage);
}

void on_button_statusvar_remove_clicked(GtkButton * button,
                                      gpointer user_data)
{
  GtkWidget *dialog;
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  SVLIST *statusvars;
  gchar *variable;

  dialog = gtk_widget_get_toplevel(GTK_WIDGET(button));
  g_return_if_fail(dialog != NULL);

  statusvars = g_object_get_data(G_OBJECT(dialog), "statusvars_list");
  g_return_if_fail(statusvars != NULL);

  treeview =
      interface_get_widget(GTK_WIDGET(button),
                           "treeview_statusvars_list");
  g_return_if_fail(treeview != NULL);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  g_return_if_fail(selection != NULL);

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, 0, &variable, -1);
    //g_message("delete %s", expr);
    svlist_remove_statusvar (statusvars, variable);
    g_free(variable);
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  } else {
    interface_display_message("No selection !!!");
  }
}


void on_treeview_statusvars_list_selection_changed(GtkTreeSelection *
    selection, gpointer data)
{
  SVLIST*         svlist;
  GtkWidget*      dialog;
  STATUSVAR*      statusvar = NULL;
  gchar*          variable;
  GtkTreeIter     iter;
  GtkTreeModel*   model;

  dialog = GTK_WIDGET (data);
  g_return_if_fail (dialog != NULL);

  svlist = g_object_get_data (G_OBJECT (dialog), "statusvars_list");
  g_return_if_fail (svlist != NULL);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    GtkEntry *_var, *_maxvar;
    GtkCheckButton *chkpercent;
    
    gtk_tree_model_get (model, &iter, 0, &variable, -1);
    statusvar = svlist_get_statusvar (svlist, variable);
    g_return_if_fail (statusvar != NULL);
    
    // and fill the dialog ...
    _var = GTK_ENTRY (interface_get_widget (dialog, "entry_statusvar_variable"));
    _maxvar = GTK_ENTRY (interface_get_widget (dialog, "entry_statusvar_maxvariable"));
    chkpercent = GTK_CHECK_BUTTON (interface_get_widget (dialog, "chkpercent"));
    
    gtk_entry_set_text (_var, statusvar->variable);
    gtk_entry_set_text (_maxvar, statusvar->maxvariable);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chkpercent),
        statusvar->percentage);

    g_free (variable);
  }
}

void on_button_gauge_add_clicked(GtkButton * button, gpointer user_data)
{
  GtkWidget*    dialog;
  Session*      session;
  GAUGELIST *gaugelist;
  GtkEntry *_var, *_maxvar;
  GtkColorSelection *colorsel;
  gchar *var, *maxvar;
  GdkColor color;

  dialog = gtk_widget_get_toplevel (GTK_WIDGET(button));
  g_return_if_fail (dialog != NULL);

  session = interface_get_active_session();
  g_return_if_fail (session != NULL);

  _var = GTK_ENTRY (interface_get_widget (dialog, "entry_gauge_variable"));
  _maxvar = GTK_ENTRY (interface_get_widget (dialog, "entry_gauge_maxvariable"));
  colorsel = GTK_COLOR_SELECTION (interface_get_widget (dialog, "colorsel_gauge"));
  
  var = (gchar *) gtk_entry_get_text (_var);
  maxvar = (gchar *) gtk_entry_get_text (_maxvar);
  gtk_color_selection_get_current_color (colorsel, &color);
  gaugelist = g_object_get_data (G_OBJECT (dialog), "gauges_list");

  if (!strcmp (var, ""))
  {
    interface_display_message("Variable name can not be empty.");
    return;
  }

  if (!gaugelist_exists (gaugelist, var)) {
    GtkWidget *treeview;
    GtkListStore *store;
    GtkTreeIter  iter;
    treeview = interface_get_widget(dialog, "treeview_gauges_list");
    store = (GtkListStore *) gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, var, -1);
  }

  gaugelist_set_gauge (gaugelist, var, maxvar, color);
}

void on_button_gauge_remove_clicked(GtkButton * button,
                                      gpointer user_data)
{
  GtkWidget *dialog;
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GAUGELIST *gauges;
  gchar *variable;

  dialog = gtk_widget_get_toplevel(GTK_WIDGET(button));
  g_return_if_fail(dialog != NULL);

  gauges = g_object_get_data(G_OBJECT(dialog), "gauges_list");
  g_return_if_fail(gauges != NULL);

  treeview =
      interface_get_widget(GTK_WIDGET(button),
                           "treeview_gauges_list");
  g_return_if_fail(treeview != NULL);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  g_return_if_fail(selection != NULL);

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, 0, &variable, -1);
    //g_message("delete %s", expr);
    gaugelist_remove_gauge (gauges, variable);
    g_free(variable);
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  } else {
    interface_display_message("No selection !!!");
  }
}


void on_treeview_gauges_list_selection_changed(GtkTreeSelection *
    selection, gpointer data)
{
  GAUGELIST*         gaugelist;
  GtkWidget*      dialog;
  GAUGE*      gauge = NULL;
  gchar*          variable;
  GtkTreeIter     iter;
  GtkTreeModel*   model;

  dialog = GTK_WIDGET (data);
  g_return_if_fail (dialog != NULL);

  gaugelist = g_object_get_data (G_OBJECT (dialog), "gauges_list");
  g_return_if_fail (gaugelist != NULL);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    GtkEntry *_var, *_maxvar;
    GtkColorSelection *colorsel;
    
    gtk_tree_model_get (model, &iter, 0, &variable, -1);
    gauge = gaugelist_get_gauge (gaugelist, variable);
    g_return_if_fail (gauge != NULL);
    
    // and fill the dialog ...
    _var = GTK_ENTRY (interface_get_widget (dialog, "entry_gauge_variable"));
    _maxvar = GTK_ENTRY (interface_get_widget (dialog, "entry_gauge_maxvariable"));
    colorsel = GTK_COLOR_SELECTION (interface_get_widget (dialog, "colorsel_gauge"));
    
    gtk_entry_set_text (_var, gauge->variable);
    gtk_entry_set_text (_maxvar, gauge->maxvariable);
    gtk_color_selection_set_current_color (colorsel, &gauge->color);

    g_free (variable);
  }
}
/*
EXPORT void
interface_open_local_macros (const gchar* body)
{
    Session* session = interface_get_active_session();
    g_return_if_fail(session != NULL);

    interface_macros_edit(&session->macros, "Macros", body);

    if (interface_get_active_session () == session)
    {
        session_save(session);
        interface_tab_refresh(session->tab);
    }
}

EXPORT
void interface_open_local_aliases (const gchar* body)
{
    Session* session = interface_get_active_session();
    g_return_if_fail(session != NULL);

    interface_triggers_edit(&session->aliases, "Aliases", body);

    if (interface_get_active_session () == session)
        session_save(session);
}

EXPORT
void interface_open_local_triggers (const gchar* body)
{
    Session* session = interface_get_active_session();
    g_return_if_fail(session != NULL);

    interface_triggers_edit(&session->triggers, "Triggers", body);

    if (interface_get_active_session () == session)
        session_save(session);
}
*/
EXPORT
void interface_open_local_statusvars ()
{
  Session* session = interface_get_active_session();
  g_return_if_fail(session != NULL);

  interface_statusvars_edit(session->svlist, "Status variables");

  if (interface_get_active_session () == session)
    session_save(session);
}

EXPORT
    void interface_open_local_gauges ()
{
  Session* session = interface_get_active_session();
  g_return_if_fail(session != NULL);

  interface_gauges_edit(session->gaugelist, "Gauges");

  if (interface_get_active_session () == session)
    session_save(session);
}
/*
EXPORT
void interface_open_global_macros (const gchar* body)
{
    interface_macros_edit(&config->macros, "Global Macros", body);
    configuration_save(config);
}

EXPORT
void interface_open_global_aliases (const gchar* body)
{
    interface_triggers_edit(&config->aliases, "Global Aliases", body);
    configuration_save(config);
}

EXPORT
void interface_open_global_triggers (const gchar* body)
{
    interface_triggers_edit(&config->triggers, "Global Triggers", body);
    configuration_save(config);
}
*/
void on_status_variables_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  interface_open_local_statusvars ();
}

void on_gauges_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  interface_open_local_gauges ();
}

void internal_font_select(GtkButton * button, gpointer user_data)
{
  SESSION_STATE *session;
  GtkWidget *dialog, *top;
  gchar **font_string;

  font_string = (gchar **) user_data;
  g_return_if_fail(font_string != NULL);

  dialog = gtk_font_selection_dialog_new(NULL);
  top = gtk_widget_get_toplevel(GTK_WIDGET(button));
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(top));

  if (*font_string != NULL) {
    gtk_font_selection_dialog_set_font_name
        (GTK_FONT_SELECTION_DIALOG(dialog), *font_string);
  }

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    if (*font_string)
      g_free(*font_string);
    // FIXME shold I use strdup ??? it's not verry clear in gtk doc 
    *font_string =
        gtk_font_selection_dialog_get_font_name
        (GTK_FONT_SELECTION_DIALOG(dialog)
        );
    gtk_button_set_label(button, *font_string);

    session = g_object_get_data(G_OBJECT(top), "session");
    g_return_if_fail(session);
    interface_tab_refresh(session->tab);
  }
  gtk_widget_destroy(dialog);
}


void internal_color_select(GtkButton * button, gpointer user_data)
{
  SESSION_STATE *session;
  GtkWidget *dialog;
  GtkWidget *top;
  GtkColorSelection *colorsel;
  GdkColor color;
  gchar **color_string = NULL;

  color_string = (gchar **) user_data;
  g_return_if_fail(color_string != NULL);

  dialog = gtk_color_selection_dialog_new(NULL);
  top = gtk_widget_get_toplevel(GTK_WIDGET(button));
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(top));

  colorsel = GTK_COLOR_SELECTION((GTK_COLOR_SELECTION_DIALOG
          (dialog))->colorsel);

  if (*color_string)
    gdk_color_parse(*color_string, &color);

  gtk_color_selection_set_current_color(colorsel, &color);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    gtk_color_selection_get_current_color(colorsel, &color);

    // free memory used by old color
    if (*color_string)
      g_free(*color_string);

    // store the new color
    *color_string =
        g_strdup_printf("#%02X%02X%02X", color.red / 256,
            color.green / 256, color.blue / 256);
    gtk_button_set_label(button, *color_string);

    // use the new color in tab
    session = g_object_get_data(G_OBJECT(top), "session");
    g_return_if_fail(session);
    interface_tab_refresh(session->tab);
  }

  gtk_widget_destroy(dialog);

}

void on_preferences_1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *dialog, *wid;
  SESSION_STATE *session;

  session = interface_get_active_session();
  g_return_if_fail(session != NULL);

  dialog = interface_create_object_by_name("dialog_preferences");
  g_return_if_fail(dialog != NULL);

  g_object_set_data(G_OBJECT(dialog), "session", session);

  wid = interface_get_widget(dialog, "button_select_font");
  g_return_if_fail(wid != NULL);
  gtk_button_set_label(GTK_BUTTON(wid), session->font);
  g_signal_connect(GTK_BUTTON(wid), "clicked",
       G_CALLBACK(internal_font_select), &session->font);


  wid = interface_get_widget(dialog, "button_select_bg_color");
  g_return_if_fail(wid != NULL);
  gtk_button_set_label(GTK_BUTTON(wid), session->bg_color);
  g_signal_connect(GTK_BUTTON(wid), "clicked",
       G_CALLBACK(internal_color_select),
       &session->bg_color);

  wid = interface_get_widget(dialog, "button_select_fg_color");
  g_return_if_fail(wid != NULL);
  gtk_button_set_label(GTK_BUTTON(wid), session->fg_color);
  g_signal_connect(GTK_BUTTON(wid), "clicked",
       G_CALLBACK(internal_color_select),
       &session->fg_color);

  wid = interface_get_widget(dialog, "button_select_ufg_color");
  g_return_if_fail(wid != NULL);
  gtk_button_set_label(GTK_BUTTON(wid), session->ufg_color);
  g_signal_connect(GTK_BUTTON(wid), "clicked",
       G_CALLBACK(internal_color_select),
       &session->ufg_color);

  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
  session_save(session);
}

void on_proxy_list_selection_changed (GtkTreeView * tv, gpointer user_data) {
	GtkTreeSelection * sel;
	GtkDialog * dialog;

	dialog = GTK_DIALOG (gtk_widget_get_toplevel (GTK_WIDGET (tv)));
	sel = gtk_tree_view_get_selection (tv);
	if (sel) {
		GtkTreeModel * model;
		GtkTreeIter iter;
		GList * rows;

		model = gtk_tree_view_get_model (tv);
		rows = gtk_tree_selection_get_selected_rows (sel, &model);
		if (1 == g_list_length (rows)) {
			char * name;
			GtkButton * b_remove, * b_edit;

			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) g_list_first (rows)->data);
			gtk_tree_model_get (model, &iter, 1, &name, -1);
			b_edit = GTK_BUTTON (interface_get_widget (GTK_WIDGET (dialog), "button_proxy_edit"));
			b_remove = GTK_BUTTON (interface_get_widget (GTK_WIDGET (dialog), "button_proxy_remove"));
			if (g_ascii_strcasecmp (name, "None") && g_ascii_strcasecmp (name, "MudMagic")) {
				gtk_widget_set_sensitive (GTK_WIDGET (b_edit), TRUE);
				gtk_widget_set_sensitive (GTK_WIDGET (b_remove), TRUE);
			} else {
				gtk_widget_set_sensitive (GTK_WIDGET (b_edit), FALSE);
				gtk_widget_set_sensitive (GTK_WIDGET (b_remove), FALSE);
			}
			g_free (name);
		}
		g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (rows);
	}
}

static GtkDialog * create_proxy_settings_dialog (char * name, char * host, int port, char * user, char * pwd) {
	GtkDialog * dialog = GTK_DIALOG (interface_create_object_by_name ("dialog_proxy_settings"));
	gtk_entry_set_text (GTK_ENTRY (interface_get_widget (GTK_WIDGET (dialog), "entry_proxy_name")), name);
	gtk_entry_set_text (GTK_ENTRY (interface_get_widget (GTK_WIDGET (dialog), "entry_proxy_host")), host);
	gtk_entry_set_text (GTK_ENTRY (interface_get_widget (GTK_WIDGET (dialog), "entry_proxy_auth_user")), user);
	gtk_entry_set_text (GTK_ENTRY (interface_get_widget (GTK_WIDGET (dialog), "entry_proxy_auth_passwd")), pwd);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (interface_get_widget (GTK_WIDGET (dialog), "spinbutton_proxy_port")), port);
	return dialog;
}

static void append_proxy (Proxy * p, GtkWidget * top) {
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (top, "treeview_proxy_list"));
	GtkListStore * store = (GtkListStore *) gtk_tree_view_get_model (tv);
	GtkTreeIter iter;
	gchar port [64];

	g_snprintf (port, 64, "%u", p->port);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, "", 1, p->name, 2, p->host, 3, port, 4, p->user, -1);
	config->proxies = g_list_first (g_list_append (config->proxies, p));
}

static void modify_proxy (Proxy * p, GtkWidget * top) {
	gchar port [64];
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (top, "treeview_proxy_list"));
	GtkListStore * store = (GtkListStore *) gtk_tree_view_get_model (tv);
	GtkTreeIter iter;
	Proxy * p_old = proxy_get_by_name (p->name, config->proxies);

	p->deflt = p_old->deflt;
	proxy_struct_free (p_old);
	* p_old = * p;
	g_snprintf (port, 64, "%u", p->port);
	if (gtk_tree_model_get_iter_first ((GtkTreeModel *) store, &iter)) {
		char * n;
		int r = 1;

		while ( 
			(gtk_tree_model_get ((GtkTreeModel *) store, &iter, 1, &n, -1), r = g_ascii_strcasecmp (p->name, n)) && 
			gtk_tree_model_iter_next ((GtkTreeModel *) store, &iter)
		) g_free (n);
		g_free (n);
		if (!r) {
			gtk_list_store_set (store, &iter, 0, p->deflt ? "*" : "", 1, p->name, 2, p->host, 3, port, 4, p->user, -1);
		}
	}
}

Proxy * get_proxy_values (GtkWidget * w) {
	Proxy * p = g_new0 (Proxy, 1);

	p->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (interface_get_widget (w, "entry_proxy_name"))));
	p->host = g_strdup (gtk_entry_get_text (GTK_ENTRY (interface_get_widget (w, "entry_proxy_host"))));
	p->user = g_strdup (gtk_entry_get_text (GTK_ENTRY (interface_get_widget (w, "entry_proxy_auth_user"))));
	p->passwd = g_strdup (gtk_entry_get_text (GTK_ENTRY (interface_get_widget (w, "entry_proxy_auth_passwd"))));
	p->port = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (interface_get_widget (w, "spinbutton_proxy_port")));
	p->deflt = FALSE;

	return p;
}

void set_default_proxy (Proxy * p, GtkTreeView * tv, gboolean val) {
	GtkListStore * store = (GtkListStore *) gtk_tree_view_get_model (tv);
	GtkTreeIter iter;

	p->deflt = val;
	if (gtk_tree_model_get_iter_first ((GtkTreeModel *) store, &iter)) {
		char * n;
		int r = 1;

		while ( 
			(gtk_tree_model_get ((GtkTreeModel *) store, &iter, 1, &n, -1), r = g_ascii_strcasecmp (p->name, n)) && 
			gtk_tree_model_iter_next ((GtkTreeModel *) store, &iter)
		) g_free (n);
		g_free (n);
		if (!r) gtk_list_store_set (store, &iter, 0, p->deflt ? "*" : "", -1);
	}
}

void run_proxy_settings_dialog (GtkWidget * top, Proxy * p_init) {
	GtkDialog * dialog;
	gboolean rept;
	char * allowed_name = NULL;

	if (p_init) {
		dialog = create_proxy_settings_dialog (p_init->name, p_init->host, p_init->port, p_init->user, p_init->passwd);
		allowed_name = p_init->name;
	} else {
		dialog = create_proxy_settings_dialog ("", "", 8080, "", "");
	}

	do {
		rept = FALSE;
		if (GTK_RESPONSE_OK == gtk_dialog_run (dialog)) {
			Proxy * p = get_proxy_values (GTK_WIDGET (dialog));
			Proxy * p_old;

			if (!strlen (p->name)) {
				GtkMessageDialog * md;
				md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
					NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					(gchar *) "%s",
					"Empty proxy name is not allowed."
				));
				gtk_dialog_run (GTK_DIALOG (md));
				gtk_widget_destroy (GTK_WIDGET (md));
				rept = TRUE;
				proxy_struct_free (p);
				g_free (p);
			} else if (!strlen (p->host)) {
				GtkMessageDialog * md;
				md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
					NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					(gchar *) "%s",
					"Empty host name is not allowed."
				));
				gtk_dialog_run (GTK_DIALOG (md));
				gtk_widget_destroy (GTK_WIDGET (md));
				rept = TRUE;
				proxy_struct_free (p);
				g_free (p);
			} else if (! (g_ascii_strcasecmp ("None", p->name) && g_ascii_strcasecmp ("MudMagic", p->name))) {
				GtkMessageDialog * md;
	
				md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
					NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					(gchar *) "%s",
					"Invalid proxy name."
				));
				gtk_dialog_run (GTK_DIALOG (md));
				gtk_widget_destroy (GTK_WIDGET (md));
				rept = TRUE;
				proxy_struct_free (p);
				g_free (p);
			} else if (p_old = proxy_get_by_name (p->name, config->proxies), !g_ascii_strcasecmp (p_old->name, p->name)) {
				// proxy with same name already exists
				int c = GTK_RESPONSE_YES;

				if ((!allowed_name) || g_ascii_strcasecmp (allowed_name, p->name)) {
					GtkMessageDialog * md;
					md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
						NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						(gchar *) "%s",
						"Proxy entry with that name already exists. Replace existing entry settings?"
					));
					gtk_dialog_add_button (GTK_DIALOG (md), "Continue editing", -234);
					c = gtk_dialog_run (GTK_DIALOG (md));
					gtk_widget_destroy (GTK_WIDGET (md));
				}
				switch (c) {
					case GTK_RESPONSE_YES:
						modify_proxy (p, top);
						g_free (p);
					break;
					case GTK_RESPONSE_NO: 
						proxy_struct_free (p);
						g_free (p);
					break;
					case -234: 
						proxy_struct_free (p);
						g_free (p);
						rept = TRUE;
					break;
				}
			} else {
				// append proxy to list
				append_proxy (p, top);
			}
		}
	} while (rept);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void on_button_help_browser_clicked (GtkButton * button, gpointer user_data) {
	GtkWidget * top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	GtkEntry * entry = GTK_ENTRY (interface_get_widget (top, "entry_help_browser"));
	GtkDialog * dialog = GTK_DIALOG (gtk_file_chooser_dialog_new ("Select executable",
		GTK_WINDOW (top),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	NULL));

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar * filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		gtk_entry_set_text (entry, filename);
		g_free (filename);
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void on_button_proxy_new_clicked (GtkButton * button, gpointer user_data) {
	GtkWidget * top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));

	run_proxy_settings_dialog (top, NULL);
}

void on_button_proxy_edit_clicked (GtkButton * button, gpointer user_data) {
	GtkWidget * top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (top, "treeview_proxy_list"));
	GtkTreeSelection * sel = gtk_tree_view_get_selection (tv);
	GtkTreeModel * model = gtk_tree_view_get_model (tv);
	GList * rows = gtk_tree_selection_get_selected_rows (sel, &model);
	GList * i;
	GtkTreeIter iter;

	if (0 == g_list_length (rows)) {
		GtkMessageDialog * md;

		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "%s",
			"No entry selected. Nothing to edit."
		));
		gtk_dialog_run (GTK_DIALOG (md));
		gtk_widget_destroy (GTK_WIDGET (md));
	} else for (i = g_list_first (rows); i; i = g_list_next (i)) {
		char * name;
		Proxy * p;

		gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) g_list_first (rows)->data);
		gtk_tree_model_get (model, &iter, 1, &name, -1);
		p = proxy_get_by_name (name, config->proxies);
		run_proxy_settings_dialog (top, p);
	}
}

void on_button_proxy_remove_clicked (GtkButton * button, gpointer user_data) {
	GtkWidget * top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (top, "treeview_proxy_list"));
	GtkTreeSelection * sel = gtk_tree_view_get_selection (tv);
	GtkTreeModel * model = gtk_tree_view_get_model (tv);
	GList * rows = gtk_tree_selection_get_selected_rows (sel, &model);
	GList * i;
	GtkTreeIter iter;

	if (0 == g_list_length (rows)) {
		GtkMessageDialog * md;

		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "%s",
			"No entry selected. Nothing to remove."
		));
		gtk_dialog_run (GTK_DIALOG (md));
		gtk_widget_destroy (GTK_WIDGET (md));
	} else for (i = g_list_first (rows); i; i = g_list_next (i)) {
		char * name;
		Proxy * p;
		GtkMessageDialog * md;

		gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) g_list_first (rows)->data);
		gtk_tree_model_get (model, &iter, 1, &name, -1);
		p = proxy_get_by_name (name, config->proxies);
		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			(gchar *) "Remove proxy entry '%s', are you sure?",
			p->name
		));
		if (GTK_RESPONSE_YES == gtk_dialog_run (GTK_DIALOG (md))) {
			gtk_list_store_remove ((GtkListStore *) model, &iter);
			config->proxies = g_list_first (g_list_remove (config->proxies, p));
			if (p->deflt) set_default_proxy (proxy_get_by_name ("MudMagic", config->proxies), tv, TRUE);
			proxy_struct_free (p);
			g_free (p);
		}
		gtk_widget_destroy (GTK_WIDGET (md));
	}
}

void on_button_proxy_set_default_clicked (GtkButton * button, gpointer user_data) {
	GtkWidget * top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (top, "treeview_proxy_list"));
	GtkTreeSelection * sel = gtk_tree_view_get_selection (tv);
	GtkTreeModel * model = gtk_tree_view_get_model (tv);
	GList * rows = gtk_tree_selection_get_selected_rows (sel, &model);
	GList * i;
	GtkTreeIter iter;

	if (0 == g_list_length (rows)) {
		GtkMessageDialog * md;

		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "%s",
			"No entry selected. Nothing to set as default proxy."
		));
		gtk_dialog_run (GTK_DIALOG (md));
		gtk_widget_destroy (GTK_WIDGET (md));
	} else for (i = g_list_first (rows); i; i = g_list_next (i)) {
		char * name;
		Proxy * p, * p_def;

		gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) g_list_first (rows)->data);
		gtk_tree_model_get (model, &iter, 1, &name, -1);
		p = proxy_get_by_name (name, config->proxies);
		p_def = proxy_get_default (config->proxies);
		if (p_def != p) {
			set_default_proxy (p_def, tv, FALSE);
			p->deflt = TRUE;
			gtk_list_store_set ((GtkListStore *) model, &iter, 0, "*", -1);
		}
	}
}

static gboolean link_forgot_event_after (GtkWidget * label, GdkEvent  * ev) {
	GdkEventButton *event;
	GtkWidget * top;
	GtkLabel * link;
	char * url;

	if (ev->type != GDK_BUTTON_RELEASE) return FALSE;
	event = (GdkEventButton *) ev;
	if (event->button != 1) return FALSE;

	top = GTK_WIDGET (gtk_widget_get_toplevel (GTK_WIDGET (label)));
	link = GTK_LABEL (interface_get_widget (top, "label_forgot_link"));
	url = (char *) gtk_label_get_text (link);
	try_to_execute_url (WEB_BROWSER, url);
	return TRUE;
}

/* action settings */

struct _settings_action_data {
	ATM * atm; // atm to edit, NULL if new
	void (* update_list) (struct _settings_action_data *, gboolean); // function for inserting atm
	int type; // 0 - alias, 1 - trigger, 2 - macro
	GtkWidget * dialog;
	GtkWidget * setup_window;
	GList ** atms;
	SESSION_STATE * session;
};

void settings_update_actions_view (GtkWidget * top, char * treeview, GList * atms) {
	GtkTreeView * tv = GTK_TREE_VIEW (interface_get_widget (top, treeview));
	GtkListStore * store = (GtkListStore *) gtk_tree_view_get_model (tv);
	GList * i;
	GtkTreeIter iter;
	char buf [128];

	if (gtk_tree_model_get_iter_first ((GtkTreeModel *) store, &iter))
		while (gtk_list_store_remove (store, &iter));

	for (i = g_list_first (atms); i; i = g_list_next (i)) {
		ATM * a = (ATM *) i->data;
		char * act = "unknown";

		if (ATM_ACTION_SCRIPT == a->action) {
			if (ATM_LANG_BASIC == a->lang) act = "Script (Basic)";
			else act = "Script (Python)";
		} else {
			switch (a->action) {
				case ATM_ACTION_TEXT: act = "Text"; break;
				case ATM_ACTION_NOISE:
					g_snprintf (buf, 128, "Sound/Music (%s)", a->source ? a->source: "???");
					act = buf;
				break;
				case ATM_ACTION_POPUP: act = "Popup message"; break;
			}
		}
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, a->name, 1, a->raiser, 2, act, 3, a->disabled ? "Disabled" : "", 4, a, -1);
	}
}

struct _settings_action_type {
	char * string;
	int value;
};

#define _settings_action_types_count (3)
const struct _settings_action_type _settings_action_types [_settings_action_types_count] = {
	{"Text", ATM_ACTION_TEXT},
	{"Sound/Music", ATM_ACTION_NOISE},
	{"Popup message", ATM_ACTION_POPUP}
};

gboolean settings_get_type (char * t, int * action, int * lang) {
	gboolean found = FALSE;
	int i;

	for (i = 0; (i < _settings_action_types_count) && !found; i++) {
		if (!g_ascii_strcasecmp (t, _settings_action_types [i].string)) {
			found = TRUE;
			* action = _settings_action_types [i].value;
			* lang = ATM_LANG_NONE;
		}
	}
	if ((!found) && g_str_has_prefix (t, "Script (")) {
		char * l = &t [strlen ("Script (")];

		* action = ATM_ACTION_SCRIPT;
		found = TRUE;
		l [strlen (l) - 1] = 0;
		if (!g_ascii_strcasecmp (l, "Basic")) * lang = ATM_LANG_BASIC;
		else if (!g_ascii_strcasecmp (l, "Python")) * lang = ATM_LANG_PYTHON;
		else * lang = ATM_LANG_NONE;
	}
	return found;
}

void settings_alias_cancel_clicked (GtkWidget * w, gpointer data) {
	struct _settings_action_data * d = (struct _settings_action_data *) data;

	gtk_widget_destroy (d->dialog);
	g_free (d);
}

void settings_alias_ok_clicked (GtkWidget * w, gpointer data) {
	GtkWidget * top = gtk_widget_get_toplevel (w);
	GtkWidget * ename = interface_get_widget (top, "entry_name");
	GtkWidget * estat = interface_get_widget (top, "entry_statement");
	GtkMessageDialog * md;
	const char * name, * stat;
	gchar * nfn = NULL, * text = NULL;
	int action = -1, lang = -1;

	name = gtk_entry_get_text (GTK_ENTRY (ename));
	stat = gtk_entry_get_text (GTK_ENTRY (estat));
	if (!strlen (name)) {
		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "Empty name is not allowed."
		));
		gtk_dialog_run (GTK_DIALOG (md));
		gtk_widget_destroy (GTK_WIDGET (md));
	} else if (!strlen (stat)) {
		md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "Empty statement is not allowed."
		));
		gtk_dialog_run (GTK_DIALOG (md));
		gtk_widget_destroy (GTK_WIDGET (md));
	} else {
		GtkComboBox * cb = GTK_COMBO_BOX (interface_get_widget (top, "combo_action"));
		gchar * ct = gtk_combo_box_get_active_text (cb);

		settings_get_type (ct, &action, &lang);
		g_free (ct);
		if (ATM_ACTION_NOISE == action) {
			GtkFileChooser * nfc = GTK_FILE_CHOOSER (interface_get_widget (top, "filechooser_noise"));

			nfn = gtk_file_chooser_get_filename (nfc);
			if (!nfn) {
				md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
					NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					(gchar *) "No file selected."
				));
				gtk_dialog_run (GTK_DIALOG (md));
				gtk_widget_destroy (GTK_WIDGET (md));
			}
		} else {
			GtkTextBuffer * tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (interface_get_widget (top, "textview")));
			GtkTextIter start, end;

			gtk_text_buffer_get_start_iter (tb, &start);
			gtk_text_buffer_get_end_iter (tb, &end);
			text = gtk_text_buffer_get_text (tb, &start, &end, FALSE);
			if (!text) {
				md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
					NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					(gchar *) "Empty text is not allowed."
				));
				gtk_dialog_run (GTK_DIALOG (md));
				gtk_widget_destroy (GTK_WIDGET (md));
			}
		}
		if (nfn || text) {
			struct _settings_action_data * d = (struct _settings_action_data *) data;
			gboolean a_new = NULL == d->atm;

			d->atm = atm_new ();
			if (ATM_ACTION_SCRIPT == action) atm_init (d->atm, -1, name, text, lang, NULL, stat, action, 0);
			else if (ATM_ACTION_NOISE == action) atm_init (d->atm, -1, name, NULL, lang, nfn, stat, action, 0);
			// text and popup messages
			else atm_init (d->atm, -1, name, NULL, lang, text, stat, action, 0);
			d->atm->config = get_configuration ();
			d->atm->session = d->session;
			d->update_list (d, a_new);
			g_free (nfn);
			g_free (text);
		}
	}
}

void settings_trigger_action_changed (GtkWidget * w, gpointer * alias) {
	GtkWidget * top = gtk_widget_get_toplevel (w);
	char * text = gtk_combo_box_get_active_text (GTK_COMBO_BOX (w));
	GtkWidget * frame = interface_get_widget (top, "frame_text");
	GtkWidget * label_file = interface_get_widget (top, "label_file");
	GtkWidget * chooser_noise = interface_get_widget (top, "filechooser_noise");

	if (text) {
		if (g_ascii_strcasecmp (text, "Sound/Music")) {
			gtk_widget_show_all (frame);
			gtk_widget_hide_all (label_file);
			gtk_widget_hide_all (chooser_noise);
		} else {
			gtk_widget_hide_all (frame);
			gtk_widget_show_all (label_file);
			gtk_widget_show_all (chooser_noise);
		}
	}
}

void settings_capture_button (GtkButton * button, gpointer user_data) {
	GtkWidget * entry = GTK_WIDGET (user_data);

	g_return_if_fail (entry != NULL);
	gtk_entry_set_text (GTK_ENTRY(entry), "");
	GTK_WIDGET_SET_FLAGS (entry, GTK_CAN_FOCUS);
	gtk_widget_grab_focus (GTK_WIDGET(entry));
}

gboolean settings_macro_entry_key_event (GtkWidget * widget, GdkEventKey * event, gpointer user_data) {
	gboolean done = FALSE;
	
	gint state = event->state;
	gint key = gdk_keyval_to_upper (event->keyval);
	
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
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_FOCUS);
		gtk_widget_grab_focus (GTK_WIDGET (user_data));
	}
	return FALSE;
}

void settings_setup_alias_dialog (GtkWidget * w, gpointer data) {
	GtkComboBox * cb = GTK_COMBO_BOX (interface_get_widget (w, "combo_action"));
	GtkWidget * label_file = interface_get_widget (w, "label_file");
	GtkWidget * chooser_noise = interface_get_widget (w, "filechooser_noise");
	GtkWidget * frame_text = interface_get_widget (w, "frame_text");
	GtkWidget * ok = interface_get_widget (w, "okbutton");
	GtkWidget * cancel = interface_get_widget (w, "cancelbutton");
	GtkWidget * name = interface_get_widget (w, "label_name");
	GtkWidget * stat = interface_get_widget (w, "label_stat");
	GtkWidget * cap = interface_get_widget (w, "button_capture");
	GtkWidget * textview = interface_get_widget (w, "textview");
	GtkWidget * entry_name = interface_get_widget (w, "entry_name");
	GtkWidget * entry_statement = interface_get_widget (w, "entry_statement");
	struct _settings_action_data * d = (struct _settings_action_data *) data;
	char buf [128];
	int i;
	int idx = 0, cnt = 0;
	int action = -1, lang = -1;

	if (d->atm) {
		action = d->atm->action;
		lang = d->atm->lang;
	}
	gtk_combo_box_remove_text (cb, 0);
	gtk_combo_box_append_text (cb, "Text");
	if (ATM_ACTION_TEXT == action) idx = 0;
	for (i = 0; i < ATMLanguageCount; i++) {
		g_snprintf (buf, 128, "Script (%s)", Languages [i].name);
		gtk_combo_box_append_text (cb, buf);
		cnt++;
		if ((ATM_ACTION_SCRIPT == action) && (Languages [i].id == lang)) idx = cnt;
	}
	if (1 == d->type) {
		gtk_window_set_title (GTK_WINDOW (w), "Trigger Properties");
		gtk_combo_box_append_text (cb, "Popup message");
		gtk_combo_box_append_text (cb, "Sound/Music");
		if (ATM_ACTION_POPUP == action) idx = cnt + 1;
		if (ATM_ACTION_NOISE == action) idx = cnt + 2;
	}
	if (d->atm) {
		if (d->atm->name) gtk_entry_set_text (GTK_ENTRY (entry_name), d->atm->name);
		if (d->atm->raiser) gtk_entry_set_text (GTK_ENTRY (entry_statement), d->atm->raiser);
		if (ATM_ACTION_NOISE == d->atm->action) {
			gtk_widget_show_all (label_file);
			gtk_widget_show_all (chooser_noise);
			gtk_widget_hide_all (frame_text);
			if (d->atm->source) gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser_noise), d->atm->source);
		} else {
			gtk_widget_hide_all (label_file);
			gtk_widget_hide_all (chooser_noise);
			gtk_widget_show_all (frame_text);
			if (ATM_ACTION_SCRIPT == d->atm->action) {
				if (!d->atm->text) atm_load_script (d->atm);
				if (d->atm->text) gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview)), d->atm->text, -1);
			} else {
				if (d->atm->source) gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview)), d->atm->source, -1);
			}
		}
	} else {
		gtk_widget_hide_all (label_file);
		gtk_widget_hide_all (chooser_noise);
	}
	if (2 == d->type) {
		gtk_window_set_title (GTK_WINDOW (w), "Macro Properties");
		gtk_label_set_text (GTK_LABEL (name), "Label:");
		gtk_label_set_text (GTK_LABEL (stat), "Shortcut:");
		g_signal_connect (G_OBJECT (cap), "clicked", G_CALLBACK (settings_capture_button), entry_statement);
		g_signal_connect (G_OBJECT (entry_statement), "key_press_event", G_CALLBACK (settings_macro_entry_key_event), cap);
	} else {
		gtk_widget_hide_all (cap);
	}
	gtk_combo_box_set_active (cb, idx);
	g_signal_connect (G_OBJECT (cb), "changed", G_CALLBACK (settings_trigger_action_changed), NULL);
	g_signal_connect (G_OBJECT (ok), "clicked", G_CALLBACK (settings_alias_ok_clicked), data);
	g_signal_connect (G_OBJECT (cancel), "clicked", G_CALLBACK (settings_alias_cancel_clicked), data);
}

void settings_add_atm (struct _settings_action_data * d, gboolean is_new) {
	GList * i;
	ATM * prev = NULL;
	gboolean do_insert = TRUE;

	// find alias with name 
	for (i = g_list_first (* d->atms); i; i = g_list_next (i)) {
		if (!g_ascii_strcasecmp (((ATM *) i->data)->name, d->atm->name)) prev = (ATM *) i->data;
	}
	if (prev && is_new) {
		GtkDialog * md = GTK_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			(gchar *) "You created new action with name '%s' but action with same name already exists. Replace old action?",
			d->atm->name
		));
		if (! (GTK_RESPONSE_OK == gtk_dialog_run (md))) do_insert = FALSE;
		gtk_widget_destroy (GTK_WIDGET (md));
	}
	if (do_insert) {
		if (prev) {
			* d->atms = g_list_remove (* d->atms, prev);
			atm_free (prev);
		}
		* d->atms = g_list_append (*d->atms, d->atm);
		gtk_widget_destroy (d->dialog);
		g_free (d);
	} else {
		atm_free (d->atm);
		d->atm = NULL;
	}
}

void settings_add_alias (struct _settings_action_data * d, gboolean is_new) {
	d->atm->type = ATM_ALIAS;
	settings_add_atm (d, is_new);
	settings_update_actions_view (d->setup_window, "treeview_aliases", * d->atms);
}

void settings_add_trigger (struct _settings_action_data * d, gboolean is_new) {
	d->atm->type = ATM_TRIGGER;
	settings_add_atm (d, is_new);
	settings_update_actions_view (d->setup_window, "treeview_triggers", * d->atms);
}

void settings_add_macro (struct _settings_action_data * d, gboolean is_new) {
	d->atm->type = ATM_MACRO;
	settings_add_atm (d, is_new);
	settings_update_actions_view (d->setup_window, "treeview_macros", * d->atms);
}

const char * settings_action_page_tv [3] = {
	"treeview_aliases",
	"treeview_triggers",
	"treeview_macros"
};

void settings_update_action (GtkWidget * w, SESSION_STATE * ses, gboolean is_new) {
	GtkWidget * top = gtk_widget_get_toplevel (w);
	GtkNotebook * acts = GTK_NOTEBOOK (interface_get_widget (top, "notebook_actions"));
	GtkWidget * d = NULL;
	gint p = gtk_notebook_get_current_page (acts);
	struct _settings_action_data * ad = g_new (struct _settings_action_data, 1);

	if (is_new) ad->atm = NULL; 
	else {
		GtkWidget * tv = interface_get_widget (top, (gchar *) settings_action_page_tv [p]);
		GtkTreeSelection * sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
		GtkTreeIter iter;
		GtkTreeModel * model;
		if (sel && gtk_tree_selection_get_selected (sel, &model, &iter)) {
		    gtk_tree_model_get (model, &iter, 4, &ad->atm, -1);
		} else return; // no selection?
	}
	ad->setup_window = top;
	ad->type = p;
	ad->session = ses;
	switch (p) {
		case 0: // aliases
			d = interface_create_object_by_name ("dialog_alias_prop");
			gtk_widget_show_all (d);
			if (ses) ad->atms = &ses->aliases;
			else ad->atms = &config->aliases;
			ad->dialog = d;
			ad->update_list = settings_add_alias;
			settings_setup_alias_dialog (d, ad);
		break;
		case 1: // triggers
			d = interface_create_object_by_name ("dialog_alias_prop");
			gtk_widget_show_all (d);
			if (ses) ad->atms = &ses->triggers;
			else ad->atms = &config->triggers;
			ad->dialog = d;
			ad->update_list = settings_add_trigger;
			settings_setup_alias_dialog (d, ad);
		break;
		case 2: // macros
			d = interface_create_object_by_name ("dialog_alias_prop");
			gtk_widget_show_all (d);
			if (ses) ad->atms = &ses->macros;
			else ad->atms = &config->macros;
			ad->dialog = d;
			ad->update_list = settings_add_macro;
			settings_setup_alias_dialog (d, ad);
		break;
		default: // unknown page
			fprintf (stderr, "unknown action page: %d\n", p);
			g_free (ad);
	}
}

void settings_add_action (GtkWidget * w, gpointer data) {
	settings_update_action (w, (SESSION_STATE *) data, TRUE);
}

void settings_edit_action (GtkWidget * w, gpointer data) {
	settings_update_action (w, (SESSION_STATE *) data, FALSE);
}

void settings_remove_action (GtkWidget * w, gpointer data) {
	GtkWidget * top = gtk_widget_get_toplevel (w);
	GtkNotebook * acts = GTK_NOTEBOOK (interface_get_widget (top, "notebook_actions"));
	gint p = gtk_notebook_get_current_page (acts);
	GtkWidget * tv = interface_get_widget (top, (gchar *) settings_action_page_tv [p]);
	GtkTreeSelection * sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	GtkTreeIter iter;
	GtkTreeModel * model;
	ATM * atm;
	GList ** atms = NULL;
	GtkMessageDialog * md;
	SESSION_STATE * ses = (SESSION_STATE *) data;

	if (sel && gtk_tree_selection_get_selected (sel, &model, &iter)) {
	    gtk_tree_model_get (model, &iter, 4, &atm, -1);
	} else return; // no selection?
	switch (p) {
		case 0: // aliases
			if (ses) atms = &ses->aliases;
			else atms = &config->aliases;
		break;
		case 1: // triggers
			if (ses) atms = &ses->triggers;
			else atms = &config->triggers;
		break;
		case 2: // macros
			if (ses) atms = &ses->macros;
			else atms = &config->macros;
		break;
		default: // unknown page
			fprintf (stderr, "unknown action page: %d\n", p);
			return;
	}
	md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
		NULL,
		GTK_DIALOG_MODAL,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO,
		(gchar *) "You are deleting %s named '%s'. Are you sure?",
		(0 == atm->type) ? "alias" : ((1 == atm->type) ? "trigger" : "macro"),
		atm->name
	));
	if (GTK_RESPONSE_YES == gtk_dialog_run (GTK_DIALOG (md))) {
		* atms = g_list_remove (* atms, atm);
		gtk_list_store_remove ((GtkListStore *) model, &iter);
		// FIXME: remove from config file
	}
	gtk_widget_destroy (GTK_WIDGET (md));
}

void settings_endis_action (GtkWidget * w, gpointer data) {
	GtkWidget * top = gtk_widget_get_toplevel (w);
	GtkNotebook * acts = GTK_NOTEBOOK (interface_get_widget (top, "notebook_actions"));
	gint p = gtk_notebook_get_current_page (acts);
	GtkWidget * tv = interface_get_widget (top, (gchar *) settings_action_page_tv [p]);
	GtkTreeSelection * sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	GtkTreeIter iter;
	GtkTreeModel * model;
	ATM * atm;

	if (sel && gtk_tree_selection_get_selected (sel, &model, &iter)) {
	    gtk_tree_model_get (model, &iter, 4, &atm, -1);
	} else return; // no selection?
	atm->disabled = !atm->disabled;
    gtk_list_store_set ((GtkListStore *) model, &iter, 3, atm->disabled ? "Disabled" : "", -1);
}

void settings_action_sel_changed (GtkTreeSelection * sel, gpointer data) {
	GtkTreeView * tv = gtk_tree_selection_get_tree_view (sel);
	GtkWidget * top = gtk_widget_get_toplevel (GTK_WIDGET (tv));
	GtkWidget * edit = interface_get_widget (top, "button_edit_action");
	GtkWidget * remove = interface_get_widget (top, "button_remove_action");
	GtkWidget * endis = interface_get_widget (top, "button_endis_action");

	if (sel && gtk_tree_selection_get_selected (sel, NULL, NULL)) {
		gtk_widget_set_sensitive (edit, TRUE);
		gtk_widget_set_sensitive (remove, TRUE);
		gtk_widget_set_sensitive (endis, TRUE);
	} else {
		gtk_widget_set_sensitive (edit, FALSE);
		gtk_widget_set_sensitive (remove, FALSE);
		gtk_widget_set_sensitive (endis, FALSE);
	}
}

void settings_notebook_actions_changed (GtkNotebook * notebook, GtkNotebookPage * page, guint page_num, gpointer user_data) {
	GtkWidget * top = gtk_widget_get_toplevel (GTK_WIDGET (notebook));
	GtkWidget * tv = interface_get_widget (top, (gchar *) settings_action_page_tv [page_num]);
	GtkTreeSelection * sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	settings_action_sel_changed (sel, NULL);
}

void settings_setup_actions (GtkWidget * w, SESSION_STATE * ses) {
	GtkListStore * store;
	GtkTreeViewColumn *column;
	GtkTreeView * tva = GTK_TREE_VIEW (interface_get_widget (w, "treeview_aliases"));
	GtkTreeView * tvt = GTK_TREE_VIEW (interface_get_widget (w, "treeview_triggers"));
	GtkTreeView * tvm = GTK_TREE_VIEW (interface_get_widget (w, "treeview_macros"));
	GtkWidget * add = interface_get_widget (w, "button_add_action");
	GtkWidget * edit = interface_get_widget (w, "button_edit_action");
	GtkWidget * remove = interface_get_widget (w, "button_remove_action");
	GtkWidget * endis = interface_get_widget (w, "button_endis_action");
	GtkWidget * nba = interface_get_widget (w, "notebook_actions");
	GtkCellRenderer * renderer = gtk_cell_renderer_text_new ();
	GtkTreeSelection * selection;
	GList * aliases, * triggers, * macros;

	store = gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
  	gtk_tree_view_set_model (tva, GTK_TREE_MODEL (store));
	store = gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
  	gtk_tree_view_set_model (tvt, GTK_TREE_MODEL (store));
	store = gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
  	gtk_tree_view_set_model (tvm, GTK_TREE_MODEL (store));

	column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (tva, column);
	column = gtk_tree_view_column_new_with_attributes ("Statement", renderer, "text", 1, NULL);
	gtk_tree_view_append_column (tva, column);
	column = gtk_tree_view_column_new_with_attributes ("Type", renderer, "text", 2, NULL);
	gtk_tree_view_append_column (tva, column);
	column = gtk_tree_view_column_new_with_attributes ("Status", renderer, "text", 3, NULL);
	gtk_tree_view_append_column (tva, column);
	
	column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (tvt, column);
	column = gtk_tree_view_column_new_with_attributes ("Statement", renderer, "text", 1, NULL);
	gtk_tree_view_append_column (tvt, column);
	column = gtk_tree_view_column_new_with_attributes ("Type", renderer, "text", 2, NULL);
	gtk_tree_view_append_column (tvt, column);
	column = gtk_tree_view_column_new_with_attributes ("Status", renderer, "text", 3, NULL);
	gtk_tree_view_append_column (tvt, column);

	column = gtk_tree_view_column_new_with_attributes ("Label", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (tvm, column);
	column = gtk_tree_view_column_new_with_attributes ("Shortcut", renderer, "text", 1, NULL);
	gtk_tree_view_append_column (tvm, column);
	column = gtk_tree_view_column_new_with_attributes ("Type", renderer, "text", 2, NULL);
	gtk_tree_view_append_column (tvm, column);
	column = gtk_tree_view_column_new_with_attributes ("Status", renderer, "text", 3, NULL);
	gtk_tree_view_append_column (tvm, column);

	g_signal_connect (G_OBJECT (nba), "switch-page", G_CALLBACK (settings_notebook_actions_changed), ses);
	g_signal_connect (G_OBJECT (add), "clicked", G_CALLBACK (settings_add_action), ses);
	g_signal_connect (G_OBJECT (edit), "clicked", G_CALLBACK (settings_edit_action), ses);
	g_signal_connect (G_OBJECT (remove), "clicked", G_CALLBACK (settings_remove_action), ses);
	g_signal_connect (G_OBJECT (endis), "clicked", G_CALLBACK (settings_endis_action), ses);
	gtk_widget_set_sensitive (edit, FALSE);
	gtk_widget_set_sensitive (remove, FALSE);
	gtk_widget_set_sensitive (endis, FALSE);
	selection = gtk_tree_view_get_selection (tva);
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (settings_action_sel_changed), NULL);
	selection = gtk_tree_view_get_selection (tvt);
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (settings_action_sel_changed), NULL);
	selection = gtk_tree_view_get_selection (tvm);
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (settings_action_sel_changed), NULL);

	if (ses) {
		aliases = ses->aliases;
		triggers = ses->triggers;
		macros = ses->macros;
	} else {
		aliases = config->aliases;
		triggers = config->triggers;
		macros = config->macros;
	}
	settings_update_actions_view (w, "treeview_aliases", aliases);
	settings_update_actions_view (w, "treeview_macros", macros);
	settings_update_actions_view (w, "treeview_triggers", triggers);
}

void on_profile_actions_activate (GtkMenuItem * menuitem, gpointer user_data) {
	Session * session = interface_get_active_session ();
	GtkWidget * dialog = interface_create_object_by_name ("dialog_actions");

	if (session) {
		settings_setup_actions (dialog, session);
		gtk_dialog_run (GTK_DIALOG (dialog));
		session_save (session);
	} else {
		fprintf (stderr, "on_profile_actions_activate: no active session!\n");
	}
	gtk_widget_destroy (dialog);
}

void on_configuration_1_activate(GtkMenuItem * menuitem,
         gpointer user_data)
{
  GtkWidget *dialog, 
      *cbdownload,  //
      *entrysep;  // Entry Seperator
  GtkWidget *cbkeepsent,  // "Keep sent"
      *cbsavehist,  // "Save history"
      *cbautocompl, // "Enable autocompletion"
      *sphistsize;
  GtkListStore *store;
  GtkWidget *wid;
  GtkTreeIter iter;
  GtkTreeViewColumn *column;
  GList *l;
  GtkTreeView * tv;
  GtkCellRenderer * r = gtk_cell_renderer_text_new ();

  gint dialog_result;

#ifndef HAVE_WINDOWS
  GtkWidget *entry1, *entry2, *entry3;
#endif
  dialog = interface_create_object_by_name("dialog_configuration");
  g_return_if_fail(dialog != NULL);

  cbdownload = interface_get_widget(dialog, "cb_download");
  g_return_if_fail(cbdownload != NULL);

  cbkeepsent = interface_get_widget(dialog, "cb_keep_sent");
  g_return_if_fail(cbkeepsent != NULL);

  entrysep = interface_get_widget(dialog, "entry_seperator");
  g_return_if_fail(entrysep != NULL);
  
  if (config->entry_seperator != NULL) {
                gtk_entry_set_text(GTK_ENTRY(entrysep), config->entry_seperator);
                g_free(config->entry_seperator);
                config->entry_seperator = NULL;
        }

  cbsavehist = interface_get_widget (dialog, "cb_cmd_save_history");
  g_return_if_fail(cbsavehist != NULL);

  cbautocompl = interface_get_widget (dialog, "cb_cmd_autocompl");
  g_return_if_fail(cbsavehist != NULL);

  sphistsize = interface_get_widget (dialog, "sp_cmd_history_size");
  g_return_if_fail(sphistsize != NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cbsavehist),
              config->cmd_buf_size);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cbautocompl),
              config->cmd_autocompl);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (sphistsize),
                 config->cmd_buf_size);

  gtk_widget_set_sensitive (sphistsize, config->cmd_buf_size);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbdownload),
             config->download);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbkeepsent),
             config->keepsent);

  //set transient for popup windows - we want config window to grab focus
  gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW (interface_get_active_window() ));

#ifndef HAVE_WINDOWS
  wid = interface_get_widget(dialog, "frame_cmds");
  g_return_if_fail(wid != NULL);
  gtk_widget_show(wid);
  entry1 = interface_get_widget(dialog, "entry_mp3cmd");
  g_return_if_fail(entry1 != NULL);
  entry2 = interface_get_widget(dialog, "entry_wavcmd");
  g_return_if_fail(entry2 != NULL);
  entry3 = interface_get_widget(dialog, "entry_midcmd");
  g_return_if_fail(entry3 != NULL);

  if (config->mp3cmd != NULL) {
    gtk_entry_set_text(GTK_ENTRY(entry1), config->mp3cmd);
    g_free(config->mp3cmd);
    config->mp3cmd = NULL;
  }
  if (config->wavcmd != NULL) {
    gtk_entry_set_text(GTK_ENTRY(entry2), config->wavcmd);
    g_free(config->wavcmd);
    config->wavcmd = NULL;
  }
  if (config->midcmd != NULL) {
    gtk_entry_set_text(GTK_ENTRY(entry3), config->midcmd);
    g_free(config->midcmd);
    config->midcmd = NULL;
  }
#endif

  g_object_set_data(G_OBJECT(dialog), "triggers_list",
        &config->triggers);
  g_object_set_data(G_OBJECT(dialog), "aliases_list",
        &config->aliases);
  g_object_set_data(G_OBJECT(dialog), "macros_list",
        &config->macros);

  /* setup proxy list settings */
  tv = GTK_TREE_VIEW (interface_get_widget (dialog, "treeview_proxy_list"));
  store = gtk_list_store_new (6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model (tv, GTK_TREE_MODEL (store));
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (tv), GTK_SELECTION_SINGLE);
  column = gtk_tree_view_column_new_with_attributes ("Default", r, "text", 0, NULL);
  gtk_tree_view_append_column (tv, column);
  column = gtk_tree_view_column_new_with_attributes ("Name", r, "text", 1, NULL);
  gtk_tree_view_append_column (tv, column);
  column = gtk_tree_view_column_new_with_attributes ("Host", r, "text", 2, NULL);
  gtk_tree_view_append_column (tv, column);
  column = gtk_tree_view_column_new_with_attributes ("Port", r, "text", 3, NULL);
  gtk_tree_view_append_column (tv, column);
  column = gtk_tree_view_column_new_with_attributes ("User", r, "text", 4, NULL);
  gtk_tree_view_append_column (tv, column);
  for (l = g_list_first (config->proxies); l; l = g_list_next (l)) {
	Proxy * p = (Proxy *) (l->data);
	gchar port [64];

	g_snprintf (port, 64, "%u", p->port);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, p->deflt ? "*" : "", 1, p->name, 2, p->host, 3, port, 4, p->user, -1);

  }

  g_signal_connect (G_OBJECT (interface_get_widget (dialog, "button_proxy_new")), "clicked", G_CALLBACK (on_button_proxy_new_clicked), NULL);
  g_signal_connect (G_OBJECT (interface_get_widget (dialog, "button_proxy_edit")), "clicked", G_CALLBACK (on_button_proxy_edit_clicked), NULL);
  g_signal_connect (G_OBJECT (interface_get_widget (dialog, "button_proxy_remove")), "clicked", G_CALLBACK (on_button_proxy_remove_clicked), NULL);
  g_signal_connect (G_OBJECT (interface_get_widget (dialog, "button_proxy_set_default")), "clicked", G_CALLBACK (on_button_proxy_set_default_clicked), NULL);
  g_signal_connect (G_OBJECT (interface_get_widget (dialog, "treeview_proxy_list")), "cursor_changed", G_CALLBACK (on_proxy_list_selection_changed), NULL);
  /* end of setup proxy list settings */
  gtk_entry_set_text (GTK_ENTRY (interface_get_widget (dialog, "entry_help_browser")), config->help_browser);
  g_signal_connect (G_OBJECT (interface_get_widget (dialog, "button_help_browser")), "clicked", G_CALLBACK (on_button_help_browser_clicked), NULL);
  gtk_entry_set_text (GTK_ENTRY (interface_get_widget (dialog, "entry_mudmagic_user")), config->acct_user);
  gtk_entry_set_text (GTK_ENTRY (interface_get_widget (dialog, "entry_mudmagic_passwd")), config->acct_passwd);

	wid = interface_get_widget (dialog, "event_forgot_link");
	g_signal_connect (wid, "event-after", G_CALLBACK (link_forgot_event_after), NULL);
	settings_setup_actions (dialog, NULL);

  dialog_result = gtk_dialog_run(GTK_DIALOG(dialog));

  if ( dialog_result == GTK_RESPONSE_OK
  ||   dialog_result == GTK_RESPONSE_CLOSE )
  {
      config->download =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cbdownload));

      config->keepsent =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cbkeepsent));

      config->entry_seperator =
    g_strdup(gtk_entry_get_text(GTK_ENTRY(entrysep)));

      config->cmd_buf_size = 
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (sphistsize));

      config->cmd_autocompl =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cbautocompl));

#ifndef HAVE_WINDOWS
      config->mp3cmd = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry1)));
      config->wavcmd = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2)));
      config->midcmd = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry3)));
#endif
	g_free (config->help_browser);
	g_free (config->acct_user);
	g_free (config->acct_passwd);
	config->help_browser = g_strdup (gtk_entry_get_text (GTK_ENTRY (interface_get_widget (dialog, "entry_help_browser"))));
	config->acct_user = g_strdup (gtk_entry_get_text (GTK_ENTRY (interface_get_widget (dialog, "entry_mudmagic_user"))));
	config->acct_passwd = g_strdup (gtk_entry_get_text (GTK_ENTRY (interface_get_widget (dialog, "entry_mudmagic_passwd"))));

      g_signal_emit_by_name (G_OBJECT (config), "changed", 0);

      configuration_save(config);
  }
  gtk_widget_destroy(dialog);

}

void on_cb_cmd_save_history_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
  GtkWidget* dialog;
  GtkWidget* cmd_size_entry;

  dialog = gtk_widget_get_toplevel (GTK_WIDGET(togglebutton));
  g_return_if_fail(dialog != NULL);

  cmd_size_entry = interface_get_widget (dialog, "sp_cmd_history_size");
  g_return_if_fail(cmd_size_entry != NULL);

  if (GTK_TOGGLE_BUTTON(togglebutton)->active)
  {
      gtk_widget_set_sensitive (cmd_size_entry, TRUE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (cmd_size_entry),
               get_configuration()->cmd_buf_size);
  }
  else
  {
      gtk_widget_set_sensitive (cmd_size_entry, FALSE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (cmd_size_entry), 0);
  }
}

void interface_run_atm (Session* session, ATM* atm, const gchar* backrefs[], gsize nbackrefs)
{
    int result;

    if (session == NULL)
        return;

    result = atm_execute (session, atm, backrefs, nbackrefs);

    if (!result)
    {
        interface_show_script_errors (atm, "Script errors");
        atm_clear_errors (atm);
    }
}

