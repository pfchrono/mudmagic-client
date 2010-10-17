/***************************************************************************
 *  Mud Magic Client                                                       *
 *  Copyright (C) 2006 MudMagic.Com ( hosting@mudmagic.com )               *
 *                2006 Calvin Ellis ( kyndig@mudmagic.com  )               *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MODULES
#define MODULES 1

/* This file contains function delcarations from all the "modules". */

// automapper entries
void module_automapper_load(void);
void module_automapper_unload(void );
void module_automapper_menu_modify ( GtkWidget *menubar );
void module_automapper_menu_reset ( GtkWidget *menubar );
void module_automapper_session_open( SESSION_STATE *session );
void module_automapper_session_close( SESSION_STATE *session );
void module_automapper_data_out( SESSION_STATE *session, gchar **data, gsize *size);

// database entries
void module_database_load(void);
void module_database_unload (void);
void module_database_toolbar_modify( GtkWidget *toolbar );
void module_database_toolbar_reset( GtkWidget *toolbar );

// notes entries
void module_notes_load(void);
void module_notes_unload(void);
void module_notes_toolbar_modify( GtkWidget *toolbar );
void module_notes_toolbar_reset( GtkWidget *toolbar );

// recorder entries
void module_recorder_load (void);
void module_recorder_unload (void);
void module_recorder_toolbar_modify (GtkWidget* toolbar);
void module_recorder_toolbar_reset (GtkWidget *toolbar);
void module_recorder_data_out (Session* ss, gchar** data, gsize* size);
void module_recorder_session_changed (Session* ss);
void module_recorder_session_open (Session* ss);
void module_recorder_session_close (Session* ss);


#endif //MODULES

