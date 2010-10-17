/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* zmp.h:                                                                  *
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
#ifndef ZMP_H
#define ZMP_H 1

#define TELOPT_ZMP 93

#include <glib.h>

typedef void (*zmp_func) (gint socket, gsize argc, gchar ** argv);
struct _zmp_command {
	gchar *name;
	zmp_func function;
};
typedef struct _zmp_command ZMP_COMMAND;

// management
void zmp_register(const gchar * name, zmp_func function);
void zmp_unregister(const gchar * name);
void zmp_unregister_all();

ZMP_COMMAND *zmp_lookup(const gchar * name);
gboolean zmp_match(const gchar * pattern);
void zmp_handle(gint fd, gchar * buff, gsize size);

// init
void zmp_init_std();

#endif				// ZMP_H
