/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* mudaccel.c:                                                             *
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
#include <glib-object.h>
#include <mudmagic.h>
#include <mudaccel.h>

struct _MudAccel
{
  guint           key;
  GdkModifierType mods;
  GtkAccelFlags   flags;
  GClosure*       gclosure;
};

/**
 * mud_accel_new: Creates new #MudAccel.
 *
 * @param key       Key value of the accelerator.
 * @param mods      Modifier combination of the accelerator.
 * @param flags	    A flag mask to configure this accelerator.
 * @param callback  Function which should be called when accelerator being activated.
 * @param user_data Data which should be passed to #callback.
 *
 * Return value: A new #MudAccel.
 **/
EXPORT
MudAccel* mud_accel_new (guint key,
		GdkModifierType     mods,
		GtkAccelFlags	    flags,
		MudAccelActivate_t  callback,
		gpointer	    user_data)
{
  MudAccel* accel = g_new (MudAccel, 1);
  accel->key = key;
  accel->mods = mods;
  accel->flags = flags;
  accel->gclosure = g_cclosure_new (G_CALLBACK (callback), user_data, NULL);

  return accel;
}

/**
 * mud_accel_free: Frees memory occupied by accelerator.
 *
 * @param accel A #MudAccel.
 *
 **/
void
mud_accel_free (MudAccel* accel)
{
  if (accel != NULL)
    {
      g_object_unref (accel->gclosure);
      g_free (accel);
    }
}

/**
 * mud_accelerator_parse: Parses a string representing an accelerator.
 *	    The format looks like "<Control>a" or "<Shift><Alt>F1" or "<Release>z"
 *	    (the last one is for key release). The parser is fairly liberal and
 *	    allows lower or upper case, and also abbreviations such
 *	    as "<Ctl>" and "<Ctrl>".
 *	    Format fully compatible with gtk_accelerator_parse().
 *
 * @param accel	A #MudAccel.
 * @param str   String representing an accelerator.
 *
 * Return value: FALSE if the parse fails or TRUE otherwise.
 *
 **/
gboolean
mud_accelerator_parse (MudAccel* accel, const gchar* str)
{
  g_return_val_if_fail (accel != NULL, FALSE);

  gtk_accelerator_parse (str, &accel->key, &accel->mods);

  return ! (accel->key == 0 && accel->mods == 0);
}

/**
 * mud_accelerator_name: Converts an accelerator keyval and modifier mask
 *	    into a string parseable by gtk_accelerator_parse().
 *	    For example, if you pass in GDK_q and GDK_CONTROL_MASK,
 *	    this function returns "<Control>q".
 *
 * @param accel A #MudAccel.
 *
 * Return value:  A newly-allocated accelerator name.
 *
 **/
gchar*
mud_accelerator_name (MudAccel* accel)
{
  g_return_val_if_fail (accel != NULL, NULL);

  return gtk_accelerator_name (accel->key, accel->mods);
}

/**
 * mud_accelerator_label: Converts an accelerator keyval and modifier
 *	    mask into a string which can be used to represent
 *	    the accelerator to the user.
 *
 * @param accel A #MudAccel.
 *
 * Return value:  A newly-allocated accelerator label.
 *
 **/
gchar*
mud_accelerator_label (MudAccel* accel)
{
  g_return_val_if_fail (accel != NULL, NULL);

  return gtk_accelerator_get_label (accel->key, accel->mods);
}

/**
 * mud_accel_group_connect: Installs an accelerator in this group.
 *	    When group is being activated in response to a call
 *	    to gtk_accel_groups_activate(), callback of #MudAccel will be invoked
 *	    if the key and mods from #MudAccel match those of this connection.
 *
 * @param group The accelerator group to install an accelerator in.
 * @param accel A #MudAccel.
 *
 * Return value: Status of operation.
 **/
EXPORT
MudAccelStatus mud_accel_group_connect (MudAccelGroup* group, MudAccel* accel)
{
  g_return_val_if_fail (group != NULL, MUD_ACCEL_ERROR);
  g_return_val_if_fail (accel != NULL, MUD_ACCEL_ERROR);

  gtk_accel_group_connect (group, accel->key, accel->mods,
                            accel->flags, accel->gclosure);

  return MUD_ACCEL_OK;
}

/**
 * mud_accel_group_disconnect: Removes an accelerator previously
 *	    installed through #mud_accel_group_connect().
 *
 * @param group The accelerator group to remove an accelerator from.
 * @param accel The #MudAccel whicj should be removed.
 *
 * Return value: Status of operation.
 *
 **/
MudAccelStatus
mud_accel_group_disconnect (MudAccelGroup* group, MudAccel* accel)
{
  gboolean res;

  g_return_val_if_fail (group != NULL, MUD_ACCEL_ERROR);
  g_return_val_if_fail (accel != NULL, MUD_ACCEL_ERROR);

  res = gtk_accel_group_disconnect (group, accel->gclosure);

  return res ? MUD_ACCEL_OK : MUD_ACCEL_ERROR;
}

