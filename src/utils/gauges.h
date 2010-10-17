/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2006 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* gauges.h:                                                            *
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
#ifndef GAUGES_H
#define GAUGES_H

typedef struct _GaugeList GAUGELIST;

#include "configuration.h"

/* one gauge */
typedef struct
{
  gchar *variable, *maxvariable;
  int cur, max;
  GdkColor color;
} GAUGE;

/* gauge list */
struct _GaugeList
{
  GList *list;
  gboolean loading;
  SESSION_STATE *sess;
};

// GAUGELIST manipulation
GAUGELIST * gaugelist_new (SESSION_STATE *s);
void gaugelist_destroy(GAUGELIST * gl);

/** does this gauge exist in the list ? */ 
gboolean gaugelist_exists(GAUGELIST *gl, gchar * name);
GAUGE * gaugelist_get_gauge(GAUGELIST *gl, gchar * name);
void gaugelist_set_gauge(GAUGELIST *gl, gchar *variable, gchar *maxvariable,
    GdkColor color);
void gaugelist_handle_variable_change (GAUGELIST *gl, gchar *variable);
void gaugelist_remove_gauge (GAUGELIST *gl, gchar * variable);

/** load gauges */ 
void  gaugelist_load(GAUGELIST *gl, FILE * f);
/** save gauges */ 
void  gaugelist_save(GAUGELIST *gl, FILE * f);

void update_gaugelist (GAUGELIST *gl);

GAUGE *gauge_new ();
void gauge_destroy (GAUGE *g);
void gauge_set_var (GAUGE *g, gchar *var);
void gauge_set_maxvar (GAUGE *g, gchar *var);

#endif // GAUGES_H
