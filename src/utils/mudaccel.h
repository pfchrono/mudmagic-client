/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* mudaccel.h:                                                             *
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
#ifndef __MUDACCEL_H__MUDMAGIC
#define __MUDACCEL_H__MUDMAGIC

#include <gtk/gtkaccelgroup.h>
#include <muderr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MudAccel    MudAccel;
typedef GtkAccelGroup	    MudAccelGroup;

/** Status of operation */
typedef enum
{
    MUD_ACCEL_OK,     /** Operation completed successfully */
    MUD_ACCEL_ERROR,  /** Unspecified error */
    MUD_ACCEL_AMBIG   /** Installed accelerator conficts pith exists */
} MudAccelStatus;

/** Callback for every accelerator installed by #mud_accel_group_connect */
typedef void (*MudAccelActivate_t) (MudAccelGroup*, gpointer user_data);


MudAccel*
mud_accel_new (guint key,
		GdkModifierType     mods,
		GtkAccelFlags	    flags,
		MudAccelActivate_t  callback,
		gpointer	    user_data);

void
mud_accel_free (MudAccel* accel);

gboolean
mud_accelerator_parse (MudAccel* accel, const gchar* str);

gchar*
mud_accelerator_name (MudAccel* accel);

gchar*
mud_accelerator_label (MudAccel* accel);

/**
 * mud_accel_group_new: Creates a new #MudAccelGroup. g_object_unref should
 *                      be called for freeing resources.
 *
 * Return value: A new #MudAccelGroup object.
 *
 **/
#define mud_accel_group_new gtk_accel_group_new

MudAccelStatus
mud_accel_group_connect (MudAccelGroup* group, MudAccel* accel);

MudAccelStatus
mud_accel_group_disconnect (MudAccelGroup* group, MudAccel* accel);

#ifdef __cplusplus
}
#endif

#endif // __MUDACCEL_H__MUDMAGIC

