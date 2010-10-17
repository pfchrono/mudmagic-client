/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* zmp-manage.c:                                                           *
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
#include "zmp.h"
#include "mudmagic.h"

static GList *zmp_commands = NULL;

EXPORT ZMP_COMMAND *zmp_lookup(const gchar * name)
{
	GList *l;
	g_return_val_if_fail(name != NULL && *name != '\0', NULL);
	l = zmp_commands;
	while (l != NULL) {
		if (strcmp(((ZMP_COMMAND *) l->data)->name, name) == 0) {
			return l->data;
		}
		l = g_list_next(l);
	}
	return NULL;
}

EXPORT void zmp_register(const gchar * name, zmp_func function)
{
	ZMP_COMMAND *command;
	g_return_if_fail(name != NULL && *name != '\0'
			 && function != NULL);
	command = g_new0(ZMP_COMMAND, 1);
	command->name = g_strdup(name);
	command->function = function;
	zmp_commands = g_list_append(zmp_commands, command);
}

EXPORT void zmp_unregister(const gchar * name)
{
	ZMP_COMMAND *command;
	g_return_if_fail(name != NULL && *name != '\0');
	command = zmp_lookup(name);
	if (command != NULL) {
		// remove entry from list 
		zmp_commands = g_list_remove(zmp_commands, command);
		// free memory alocate for name
		g_free(command->name);
		// free memory alocate for command entry
		g_free(command);
	}
}

EXPORT gboolean zmp_match(const gchar * pattern)
{
	gboolean package = FALSE;
	GList *l;
	gchar *s;
	g_return_val_if_fail(pattern != NULL && *pattern != '\0', FALSE);
	package = g_str_has_suffix(pattern, ".");
	l = zmp_commands;
	while (l != NULL) {
		s = ((ZMP_COMMAND *) l->data)->name;
		if (package && g_str_has_prefix(s, pattern))
			return TRUE;
		if (!package && strcmp(s, pattern) == 0)
			return TRUE;
		l = g_list_next(l);
	}
	return FALSE;
}

EXPORT void zmp_unregister_all()
{
	ZMP_COMMAND *command;
	while (zmp_commands != NULL) {
		command = (ZMP_COMMAND *) zmp_commands->data;
		// remove entry from list 
		zmp_commands = g_list_remove(zmp_commands, command);
		// free memory alocate for name
		g_free(command->name);
		// free memory alocate for command entry
		g_free(command);
	}
}
