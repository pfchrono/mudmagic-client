/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2006 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* statusvars.h:                                                            *
*                2006 Tomas Mecir  ( kmuddy@kmuddy.net )                  *
*                                                                         *
***************************************************************************/
/**************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/    
#ifndef STATUSVARS_H
#define STATUSVARS_H


typedef struct _StatusvarList SVLIST;

#include "configuration.h"

/* one statusvar */
typedef struct
{
  gchar *variable, *maxvariable;
  int cur, max;
  gboolean percentage;
} STATUSVAR;

/* statusvar list */
struct _StatusvarList
{
  GList *list;
  gboolean loading;
  SESSION_STATE *sess;
};

// SVLIST manipulation
SVLIST * svlist_new (SESSION_STATE *s);
void svlist_destroy(SVLIST *svl);

/** does this statusvar exist in the list ? */ 
gboolean svlist_exists(SVLIST *svl, gchar * name);
STATUSVAR * svlist_get_statusvar(SVLIST *svl, gchar * name);
void svlist_set_statusvar(SVLIST *svl, gchar *variable, gchar *maxvariable,
    gboolean percentage);
void svlist_handle_variable_change (SVLIST *svl, gchar *variable);
void svlist_remove_statusvar (SVLIST *svl, gchar * variable);

/** load statusvars */ 
void  svlist_load(SVLIST * v, FILE * f);
/** save statusvars */ 
void  svlist_save(SVLIST * v, FILE * f);

void update_svlist (SVLIST *svl);

STATUSVAR  *statusvar_new ();
void  statusvar_destroy(STATUSVAR * sv);
void statusvar_set_var (STATUSVAR *sv, gchar *var);
void statusvar_set_maxvar (STATUSVAR *sv, gchar *var);

#endif // STATUSVARS_H
