/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* variables.h:                                                            *
*                2005 Tomas Mecir  ( kmuddy@kmuddy.net )                  *
*                                                                         *
***************************************************************************/
/**************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/   

#ifndef VARIABLES_H
#define VARIABLES_H

typedef struct _VariableList VARLIST;

#include "configuration.h"


/* one variable */
typedef struct
{
  gchar * name;
  gchar * value;
}VARIABLE;

/* variable list */
struct _VariableList
{
  GTree * tree;
  SESSION_STATE *sess;
};

// VARLIST manipulation
VARLIST * varlist_new (SESSION_STATE *s);
void varlist_destroy(VARLIST * v);
gchar * varlist_get_value(VARLIST * v, gchar * name);
int varlist_get_int_value(VARLIST * v, gchar * name);

/** does this variable exist in the list ? */ 
gboolean varlist_exists(VARLIST * v, gchar * name);
VARIABLE * varlist_get_variable(VARLIST * v, gchar * name);
void varlist_set_value(VARLIST * v, gchar * name, gchar * value);
void varlist_remove_value(VARLIST * v, gchar * name);

/** load variables */ 
void  varlist_load(VARLIST * v, FILE * f);

/* save variables */ 
void  varlist_save(VARLIST * v, FILE * f);
gchar   *variables_expand(VARLIST * v, gchar * string, int len);

/* VARIABLE manipulation mostly for VARLIST, should not be used elsewhere */
VARIABLE  *variable_new(gchar * name);
void  variable_destroy(VARIABLE * var);
void  variable_set_name(VARIABLE * var, gchar * name);
void  variable_set_value(VARIABLE * var, gchar * value);
gchar   *variable_name(VARIABLE * var);
gchar   *variable_value(VARIABLE * var);

#endif  // VARIABLES_H
