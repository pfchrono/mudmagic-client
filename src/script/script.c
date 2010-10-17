/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* script.c:                                                               *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                2006  Victor Vorodyukhin ( victor.scorpion@gmail.com )   *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <mudmagic.h>
#include <pcre.h>
#include <interface.h>
#include <string.h>
#include "script.h"


static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
static SESSION_STATE *current_session = NULL;
static PyObject * p_main_dict = NULL;

/*
SCRIPT *script_new(gchar * file, gchar * name)
{
	SCRIPT *ret;
	ret = g_new0(SCRIPT, 1);
	ret->file = g_strdup(file);
	g_strstrip(ret->file);
	if (name)
		ret->name = g_strdup(name);
	return ret;
}

void script_delete(SCRIPT * script)
{
	if (!script)
		return;
	if (script->file)
		g_free(script->file);
	if (script->name)
		g_free(script->name);
	g_free(script);
}
*/
int script_execute(SESSION_STATE * session, ATM* atm,
		    const gchar *backrefs[], gsize backrefcnt)
{
	int i, ret;
	gchar *code = g_strdup("");

	for (i = 0; i < backrefcnt; ++i) {
		gchar *t =
		    g_strdup_printf("%s_%d=\"%s\"\n", code, i,
				    backrefs[i]);
		g_free(code);
		code = t;
	}
	ret = script_run(atm, session, code);
	g_free(code);
        return ret;
}


int script_run(ATM* atm, SESSION_STATE * session,
		const gchar* extra_code)
{
	/* (Tomas) this mutex is a very bad idea, but there's no better solution
	 *  with this implementation
	 */
	PyObject * ret, * err, * dict;
	int r = TRUE;
	

	g_static_mutex_lock(&mutex);	// serialize the scripts execution 
	current_session = session;

	dict = PyDict_New ();
	PyErr_Clear ();
	if (dict && extra_code) {
		ret = PyRun_String(extra_code, Py_file_input, p_main_dict, dict);
		//g_print(">>extra code:\n%s\n>>end extra code\n",
		//	extra_code);
		Py_XDECREF (ret);
	}

	err = PyErr_Occurred ();
	if (!err) {
		ret = NULL;
        if (atm->text) {
			ret = PyRun_String (atm->text, Py_file_input, p_main_dict, dict);
        } else if (atm->source) {
			FILE *fd;
			fd = fopen(atm->source, "r");
			if (fd) {
				ret = PyRun_File(fd, atm->source, Py_file_input, p_main_dict, dict);
				fclose(fd);
			} else {
				char buf [1024];
				g_snprintf (buf, 1024, "%s: %s", atm->source, strerror (errno));
				atm->errors = g_list_append (atm->errors, g_strdup (buf));
				//se = g_strdup (buf);
			}
		}
		err = PyErr_Occurred ();
		Py_XDECREF (ret);
	}
	if (err) {
		PyObject * type, * val, * trback, * pystr = NULL;
		char * c;

		r = FALSE;
		PyErr_Fetch (&type, &val, &trback);
		if (val) {
			pystr = PyObject_Str (val);
		} else if (type) {
			pystr = PyObject_Str (val);
		}
		if (pystr) {
			c = PyString_AsString (pystr);
		} else {
			c = "<unknown error>";
		}
		atm->errors = g_list_append (atm->errors, g_strdup (c));
		Py_XDECREF (type);
		Py_XDECREF (val);
		Py_XDECREF (trback);
		Py_XDECREF (pystr);
	}

	Py_XDECREF (dict);

	g_static_mutex_unlock(&mutex);

	return r;
}

// export this function to python 
PyObject *mudmagic_send_string(PyObject * self, PyObject * args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
		return NULL;
	send_command(current_session, s, strlen(s));
	return Py_BuildValue("");
}

PyObject *mudmagic_messagebox(PyObject * self, PyObject * args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
		return NULL;
	interface_display_message(s);
	return Py_BuildValue("");
}

static PyMethodDef MudmagicMethods[] = {
	{"send", mudmagic_send_string, METH_VARARGS,
	 "send a string to server"},
	{"messagebox", mudmagic_messagebox, METH_VARARGS,
	 "display a message box"},
	{NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC script_init_mudmagic(void) {
	PyObject * p_main;

	PyImport_AddModule ("mudmagic");
	Py_InitModule ("mudmagic", MudmagicMethods);
	p_main = PyImport_AddModule ("__main__");
	p_main_dict = PyModule_GetDict (p_main);
	PyRun_SimpleString("from mudmagic import *\n");
}
