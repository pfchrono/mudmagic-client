/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* main.c:                                                                 *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                2005  Vasiliy Shlykov ( vash@vasiliyshlykov.org )        *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdio.h>
#include <gtk/gtk.h>
#include <mudmagic.h>
#ifdef HAVE_WINDOWS
#include <winsock2.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <log.h>
#include <interface.h>
#include <Python.h>
#include <script.h>
#include <module.h>
#include <curl/curl.h>

static void
mud_load_modules (GList* list)
{
  GList*        it;
  MODULE_ENTRY* pe;

  for (it = g_list_first (list); it != NULL; it = g_list_next (it))
    {
      pe = (MODULE_ENTRY*) it->data;

      if (pe->used)
          module_load (pe);
    }

  if (list != NULL)
      interface_modules_init (list);
}

extern Configuration* config;

#ifdef HAVE_WINDOWS
void null_printf( const gchar *string )
{
    //if (debug_log_file_stream ())
    //    fprintf (debug_log_file_stream (), string);
}    
void null_log_handler( const gchar *log_domain, GLogLevelFlags log_level,
                           const gchar *message, gpointer user_data)
{
   // if (debug_log_file_stream ())
   //     fprintf (debug_log_file_stream (), "%s: %s\n", log_domain, message);
}    
#endif 
static void main_window_destroyed (GtkWidget * widget, gpointer data) {
	gboolean res = configuration_save (config);

	if (!res)
	{
		gchar buf[400];
		g_snprintf (buf, 400, "Couldn't save configuration to file \"%s\".\n"
							  "All options will lost.", config->filename);
	    interface_show_gerrors (config->cfg_errors, buf);
		//interface_messagebox (GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, buf);
	}
	
	//make some clean up
	configuration_end( config );
	
	Py_Finalize();
	curl_global_cleanup ();
#ifdef HAVE_WINDOWS
	WSACleanup();
#endif
}

int main( int argc, char **argv )
{
        int fd;
	gboolean res;
	GtkWidget * mw;
        
	init_muderr ();
#ifdef HAVE_WINDOWS
	WSADATA wsaData;   // if this doesn't work
	//WSAData wsaData; // then try this instead
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	} 
	// don't write anything on console
       // fd = debug_log_file ();
      //  if (fd != -1)
      //  {
      //      dup2 (fd, STDOUT_FILENO);
      //      dup2 (fd, STDERR_FILENO);
      //  }
	//g_set_print_handler( null_printf );
	//g_set_printerr_handler( null_printf );
	g_log_set_handler( "Gdk", G_LOG_LEVEL_MASK | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, null_log_handler, NULL);
	g_log_set_handler( "GLib", G_LOG_LEVEL_MASK | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, null_log_handler, NULL);
	g_log_set_handler( "Pango", G_LOG_LEVEL_MASK | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, null_log_handler, NULL);
	g_log_set_handler( "GLib-GObject", G_LOG_LEVEL_MASK | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, null_log_handler, NULL);
	g_log_set_handler( "GThread", G_LOG_LEVEL_MASK | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, null_log_handler, NULL);
	g_log_set_handler( "Gtk", G_LOG_LEVEL_MASK, null_log_handler, NULL);
	g_log_set_handler( "libglade", G_LOG_LEVEL_MASK | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, null_log_handler, NULL);
	g_log_set_handler( NULL, G_LOG_LEVEL_MASK | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, null_log_handler, NULL);
#endif 
	curl_global_init (CURL_GLOBAL_NOTHING);
	Py_Initialize();
	script_init_mudmagic(); 	// export mudmagic functions to python 
	g_thread_init( NULL );
	gtk_init( &argc, &argv );

	config = CONFIG (configuration_new());
	res = configuration_load (config, config->cfgfile);
	if (!res)
	{
		gchar buf[400];
		g_snprintf (buf, 400, "Couldn't load configuration from file \"%s\".", config->cfgfile);
	    	interface_show_gerrors (config->cfg_errors, buf);
		utils_clear_gerrors (&config->cfg_errors);
	}
	mw = interface_add_window();
	g_signal_connect (G_OBJECT (mw), "destroy", G_CALLBACK (main_window_destroyed), NULL);

        mud_load_modules (get_configuration()->modules);
	
	init_theme();

	gtk_main();
/*	
	res = configuration_save (config);
	if (!res)
	{
		gchar buf[400];
		g_snprintf (buf, 400, "Couldn't save configuration to file \"%s\".\n"
							  "All options will lost.", config->filename);
	    interface_show_gerrors (config->cfg_errors, buf);
		//interface_messagebox (GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, buf);
	}
	
	//make some clean up
	configuration_end( config );
	
	Py_Finalize();
	curl_global_cleanup ();
#ifdef HAVE_WINDOWS
	WSACleanup();
#endif
*/
	return 0;
}
