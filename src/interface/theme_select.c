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
#include <glib.h>
#include <glib/gprintf.h>
#include <mudmagic.h>
#include <protocols.h>
#include "interface.h"
#include "cmdentry.h"
#include "theme_select.h"
#include <module.h>
#include <utils.h>


#ifdef HAVE_WINDOWS  
#define MUDCFG_HOME_DIR           "."
#else
#define MUDCFG_HOME_DIR           ".mudmagic"
#endif

#define MUDRCCFG_CONFIG_FILE_TMP "gtkrc.tmp"
#define MUDRCCFG_CONFIG_FILE     "gtkrc"

//view the theme selection window
void on_theme_select1_activate(GtkMenuItem * menuitem, gpointer user_data)
{
    GtkWidget *theme_window;
    GtkWidget *tree_view;
    GList *theme_list;
    gchar *theme;

    theme_window = interface_create_object_by_name("window_theme");
    tree_view = interface_get_widget(theme_window, "main_themelist");

    GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(store));
    int i =0, curr=0;
    GtkTreeIter   iter;
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes ( "Theme",
								           gtk_cell_renderer_text_new(),
                                                                           "text", 0,
                                                                           NULL);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

    theme_list = build_theme_list();
    if( theme_list == NULL )
                return;

    while ((theme = (g_list_nth_data(theme_list, i))) != NULL ) 
    {
    	gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, theme, -1);

        if (strcmp(theme, get_current_theme() ) == 0) 
	{
        	curr = i;
        }
        ++i;
    }
    GtkTreeSelection* selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree_view));

    // set the default theme
    // THIS IS IMPORTANT!!!
    gtk_widget_grab_focus(tree_view);
    char curr_string[100];
    sprintf( curr_string, "%d", i );

    GtkTreePath* selpath = gtk_tree_path_new_from_string (curr_string);
    if (selpath) 
    {
 	gtk_tree_selection_select_path(selection, selpath);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree_view), selpath, NULL, TRUE, 0.5, 0.0);
        gtk_tree_path_free(selpath);
    }

    g_signal_connect ( G_OBJECT (selection), "changed",
                      G_CALLBACK (themelist_selection_changed_cb), NULL);
    g_object_unref (G_OBJECT (store));
}

gchar *theme_dir_location()
{
	return gtk_rc_get_theme_dir ();
}

GList * build_theme_list(void)
{
    GDir* gdir;
    const char  *name;
    gchar *theme_dir;
    GList *theme_list = 0;

    theme_dir = theme_dir_location();
    gdir = g_dir_open(theme_dir, 0, NULL );
    if (gdir == NULL)
	return 0;

    while (( name = (g_dir_read_name(gdir))) != NULL ) 
    {
	gboolean is_dir = 0;
	const char *filename = name;
        char *fullname =  g_build_filename(theme_dir, filename, NULL);
	char *rc;
		
	if (*name == '.')
		continue;
		
	rc = g_build_filename( fullname, G_DIR_SEPARATOR_S, "gtk-2.0", G_DIR_SEPARATOR_S, "gtkrc", NULL);

	if (g_file_test(fullname, G_FILE_TEST_IS_DIR))
        	is_dir = 1;

        if (is_dir && g_file_test(rc, G_FILE_TEST_IS_REGULAR)) {
		theme_list = g_list_insert_sorted(theme_list, g_strdup(filename), (GCompareFunc)strcmp);
	}
    }
    g_dir_close(gdir);
    return theme_list;
}

gchar *get_current_theme()
{

        GtkSettings* settings = gtk_settings_get_default();
        gchar* theme;
        g_object_get(settings, "gtk-theme-name", &theme, NULL);

        return (theme);
}

void themelist_selection_changed_cb(GtkTreeSelection* selection, gpointer data)
{
	
        if (gtk_tree_selection_get_selected (selection, 0, 0))
                apply_theme(get_selected_theme(), get_current_font(), 1);
}

gchar * get_selected_theme()
{
	GtkWidget *theme_window = NULL;
	GList *tmplist;

	tmplist = g_list_first(gtk_window_list_toplevels());
	while (tmplist) 
	{
		if (! strcmp (gtk_widget_get_name (GTK_WIDGET (tmplist->data)), "window_theme"))
		{
			theme_window = tmplist->data;
		}
	tmplist = g_list_next(tmplist);
	}
	if( theme_window == NULL )
		return NULL;

  	GtkWidget *treeview = interface_get_widget(theme_window, "main_themelist");
      GtkTreeModel* model = gtk_tree_view_get_model( GTK_TREE_VIEW( treeview ) );
      GtkTreeSelection* selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ) );

      GtkTreeIter iter;
      gtk_tree_selection_get_selected(selection, 0, &iter);

      gchar* theme_name;
      gtk_tree_model_get(model, &iter, 0, &theme_name, -1);

return (theme_name);
}

gchar * get_current_font()
{
	GtkWidget *main_window;
	GtkWidget *tab;
	main_window = interface_get_active_window();

	tab = interface_get_active_tab();
	if( tab )
	{
		SESSION_STATE *session;

		session = g_object_get_data(G_OBJECT(tab), "session");
		return(session->font);
	}
	else
	{
		return(pango_font_description_to_string(gtk_rc_get_style(main_window)->font_desc));
	}
}

void apply_theme(gchar *theme_name, gchar *font, gboolean is_preview )
{
      if(!theme_name) return;
	gchar *theme_dir;
	gchar* mudhome = NULL;
  	gchar* gtkrc_fp = NULL;
	FILE* gtkrc_fh;

	theme_dir = theme_dir_location();

#ifdef HAVE_WINDOWS  
	mudhome = "";
	gtkrc_fp = "gtkrc.tmp";
#else
	char *rc_file;
	Configuration* cfg = get_configuration ();

	mudhome = g_build_path (G_DIR_SEPARATOR_S, cfg->basepath, MUDCFG_HOME_DIR, NULL);
        gtkrc_fp = g_build_path (G_DIR_SEPARATOR_S, mudhome, MUDRCCFG_CONFIG_FILE_TMP, NULL);

	rc_file = g_build_filename( theme_dir, G_DIR_SEPARATOR_S, theme_name, G_DIR_SEPARATOR_S, "gtk-2.0", G_DIR_SEPARATOR_S, "gtkrc", NULL);

	if( !g_file_test(mudhome, G_FILE_TEST_IS_DIR | G_FILE_TEST_IS_EXECUTABLE ) )
	{
		g_printf("not a file\n");
		return;
	}
#endif
	gtkrc_fh = fopen(gtkrc_fp, "w+");

#ifdef HAVE_WINDOWS 
	g_fprintf(gtkrc_fh, "# -- THEME AUTO-WRITTEN DO NOT EDIT\ninclude \"" );
	fprintf(gtkrc_fh, ".\\\\share\\\\themes\\\\");
	fprintf(gtkrc_fh, theme_name );
	fprintf(gtkrc_fh, "\\\\gtk-2.0\\\\gtkrc");
	fprintf(gtkrc_fh, "\"\n\n" );
#else
     	fprintf(gtkrc_fh,
               "# -- THEME AUTO-WRITTEN DO NOT EDIT\n" "include \"%s\"\n\n",
                rc_file);
#endif

       	if (font)
       	{
               	fprintf(gtkrc_fh,
                       	"style \"user-font\" {\n" "\tfont_name = \"%s\"\n" "}\n\n", font);
               	fprintf(gtkrc_fh, "widget_class \"*\" style \"user-font\"\n\n");
               	fprintf(gtkrc_fh, "gtk-font-name=\"%s\"\n\n", font);
       	}
	fclose(gtkrc_fh);

	if( !is_preview )
	{
		 FILE* gtkrc_fh;
		 Configuration* cfg = get_configuration ();
		 gchar* mudhome = NULL;
        	 gchar* gtkrc_fp = NULL;

		 mudhome = g_build_path (G_DIR_SEPARATOR_S, cfg->basepath, MUDCFG_HOME_DIR, NULL);
        	 gtkrc_fp = g_build_path (G_DIR_SEPARATOR_S, mudhome, MUDRCCFG_CONFIG_FILE, NULL);

#ifdef HAVE_WINDOWS  
	gtkrc_fp = "gtkrc";
#endif

		gtkrc_fh = fopen(gtkrc_fp, "w+");
#ifdef HAVE_WINDOWS
	fprintf(gtkrc_fh, "# -- THEME AUTO-WRITTEN DO NOT EDIT\ninclude\"" );
	fprintf(gtkrc_fh, ".\\\\share\\\\themes\\\\");
	fprintf(gtkrc_fh, theme_name );
	fprintf(gtkrc_fh, "\\\\gtk-2.0\\\\gtkrc");
	fprintf(gtkrc_fh, "\"\n\n" );
#else
		fprintf(gtkrc_fh,
               			"# -- THEME AUTO-WRITTEN DO NOT EDIT\n" "include \"%s\"\n\n",
                		rc_file);
#endif
        	if (font)
        	{
                	fprintf(gtkrc_fh,
                        	"style \"user-font\" {\n" "\tfont_name = \"%s\"\n" "}\n\n", font);
                	fprintf(gtkrc_fh, "widget_class \"*\" style \"user-font\"\n\n");
                	fprintf(gtkrc_fh, "gtk-font-name=\"%s\"\n\n", font);
        	}
		fclose(gtkrc_fh);
	}

	gchar *default_files[] = { gtkrc_fp, NULL };
        gtk_rc_set_default_files(default_files);

        if (is_preview)
        {
                gtk_rc_reparse_all_for_settings(gtk_settings_get_default(), TRUE);
        }
        else
        {
		GdkEventClient event =
                        { GDK_CLIENT_EVENT, NULL, TRUE, gdk_atom_intern("_GTK_READ_RCFILES",
                                FALSE), 8 };
                gdk_event_send_clientmessage_toall((GdkEvent *) & event);
        }
return;
}

void close_theme_window()
{
        GList *tmplist;

        tmplist = g_list_first(gtk_window_list_toplevels());
        while (tmplist)
        {
                if (! strcmp (gtk_widget_get_name (GTK_WIDGET (tmplist->data)), "window_theme"))
                {
                        gtk_widget_destroy(tmplist->data);
                }
        tmplist = g_list_next(tmplist);
        }
}

void init_theme()
{
	gchar *file;
	gchar *contents;

#ifdef HAVE_WINDOWS  
	file = "gtkrc";
#else
	gchar *mudhome = NULL;
	Configuration* cfg = get_configuration ();

	mudhome = g_build_path (G_DIR_SEPARATOR_S, cfg->basepath, MUDCFG_HOME_DIR, NULL);
	file = g_build_path (G_DIR_SEPARATOR_S, mudhome, MUDRCCFG_CONFIG_FILE, NULL);
#endif

      if( !g_file_test(file, G_FILE_TEST_IS_REGULAR ) )
	{
                return;
	}

	if (!g_file_get_contents(file, &contents, NULL, NULL))
	{
		return;
	}
	gchar *default_files[] = { file, NULL };
      gtk_rc_set_default_files(default_files);

//added kyndig
	GtkSettings* settings = gtk_settings_get_default();
  	gtk_rc_reparse_all_for_settings (settings, TRUE);
//stop added

      gtk_rc_reparse_all_for_settings(gtk_settings_get_default(), TRUE);

	g_free(contents);
}

void on_theme_reset_button_clicked(GtkMenuItem * menuitem, gpointer user_data)
{
	//just reinit the original theme
	init_theme();
}

void on_theme_cancel_button_enter(GtkMenuItem * menuitem, gpointer user_data)
{
	init_theme();
	close_theme_window();
}

void on_theme_ok_button_clicked(GtkMenuItem * menuitem, gpointer user_data)
{
	apply_theme(get_selected_theme(), get_current_font(), 0);
	close_theme_window();
}

