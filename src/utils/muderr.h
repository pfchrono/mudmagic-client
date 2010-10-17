/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* muderr.h:                                                               *
*                2005 Shlykov Vasiliy ( vash@zmail.ru )                   *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef __MUDERR_H__MUDMAGIC
#define __MUDERR_H__MUDMAGIC

#include <glib/gtypes.h>
#include <glib/glist.h>
#include <glib/gquark.h>
#include <glib/gerror.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef GError MudError;
typedef GList  MudErrorList;

extern GQuark  MUD_NETWORK_ERROR;
extern GQuark  MUD_GAMELIST_ERROR;

/**
 * mud_error_new: Creates a new #MudError with the given #domain and #code,
 *                and a message formatted with #format.
 *
 * @param quark Error domain.
 * @param code  Error code.
 * @param fmt   Error message format.
 *
 **/
#define mud_error_new g_error_new

/**
 * mud_error_free: Frees a #MudError and associated resources.
 *
 **/
#define mud_error_free g_error_free

/**
 * mud_error_add: Add #error to the given #list.
 *
 * @param list  Pointer to a #MudErrorList.
 * @param error A #MudError.
 *
 **/
static inline void
mud_error_add (MudErrorList** list, MudError* error)
{
  *list = g_list_append (*list, error);
}

/**
 * mud_error_copy: Makes a copy of #error.
 *
 * @param error A #MudError.
 * 
 * Return value: A new #MudError.
 *
 **/
static inline MudError*
mud_error_copy (const MudError* error)
{
  return g_error_copy (error);
}

/**
 * mud_cnv: Converts standart glib #GError to #MudError.
 *
 * @param gerror A #GError.
 *
 * Return value: A new #MudError.
 *
 **/
static inline MudError*
mud_cnv (const GError* gerror)
{
  return g_error_copy (gerror);
}

/**
 * mud_error_list_clear: Frees memory occupied by errors and list itself.
 *
 * @param list A #MudErrorList.
 *
 **/
static inline void
mud_error_list_clear (MudErrorList** list)
{
  extern void utils_clear_errors (GList**);

  utils_clear_errors (list);
}

/**
 * mud_error_list_join: Concatenates all errors in list to one string
 *                      with given delimiter.
 * 
 * @param list  A #MudErrorList.
 * @param delim Delimiter.
 *
 * @return A newly allocated string.
 **/
static inline gchar*
mud_error_list_join (const MudErrorList* list, const gchar* delim)
{
  extern gchar* utils_join_gerrors (GList* list, const gchar* delim);

  return utils_join_gerrors ((GList*) list, delim);
}

/**
 * mud_error_get_msg: Gets error message associated with given error.
 *
 * @param error A #MudError.
 *
 * @return Error message or NULL if #error is NULL.
 **/
static inline const gchar*
mud_error_get_msg (const MudError* error)
{
  return error ? error->message : NULL;
}

void init_muderr (void);

#ifdef __cplusplus
}
#endif

#endif /* end of header file */

