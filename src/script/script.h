/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* script.h:                                                               *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef SCRIPT_H
#define SCRIPT_H 1

#include <Python.h>
#include <mudmagic.h>

/*
typedef struct {
	gchar *file;		// the script file
	gchar *name;		// for aliases is the button label
} SCRIPT;
*/

int script_execute(SESSION_STATE * session, ATM * atm,
		    const gchar *backrefs[], gsize backrefcnt);

int script_run(ATM* atm, SESSION_STATE* session, 
	        const gchar* extra_code);
//SCRIPT *script_new(gchar * file, gchar * name);
//void script_delete(SCRIPT * script);

PyObject *mudmagic_send_string(PyObject * self, PyObject * args);
PyMODINIT_FUNC script_init_mudmagic(void);

#endif				// SCRIPT_H
