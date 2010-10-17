/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* alias_triggers.h:                                                       *
*                2005 Tomas Mecir     ( kmuddy@kmuddy.net )               *
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
#ifndef __ATM_H_
#define __ATM_H_

#include <mudmagic.h>
#include "configuration.h"

ATM*
atm_new (void);

void
atm_free (ATM* atm);

void
atm_init (ATM* atm, int type, const gchar* name, const gchar* text,
                    int lang, const gchar* source, const gchar* raiser, int action, int disabled);

void
atm_init_macro (ATM* atm, const gchar* name, const gchar* text,
                          int lang, const gchar* fname, const gchar* raiser, int action);

void
atm_init_alias (ATM* atm, const gchar* name, const gchar* text,
                          int lang, const gchar* fname, const gchar* raiser, int action);

void
atm_init_trigger (ATM* atm, const gchar* name, const gchar* text,
                            int lang, const gchar* fname, const gchar* raiser, int action);

void
atm_set_masters (ATM* atm, Configuration* cfg, Session* ss);

int
atm_detect_type (const gchar* file);

const gchar*
atm_get_text (ATM* atm);

gboolean
atm_macro_in_fire (ATM* atm, gint state, gint key);

gboolean
atm_load_script (ATM* atm);

gboolean
atm_save_script (ATM* atm);

gboolean
atm_test_script_exists (const ATM* atm);

gsize
atm_create_names_list (GList* list, gchar*** arr);

ATM*
atm_get_by_expr (GList* list, const gchar* expr);

gboolean
atm_add_to_list (GList** list, ATM* atm);

gboolean
atm_remove_from_list (GList ** list, const gchar * expr);

gint
atm_execute (SESSION_STATE*, ATM*, const gchar** backrefs, gsize backrefcnt);

ATM*
atm_find_fire (SESSION_STATE* ss, const gchar* data, gsize len,
                        GList* list, gboolean single, gint* result);
#define atm_get_name(atm) (atm)->name

void
atm_list_clear (GList ** list);

void
atm_clear_errors (ATM* atm);

const gchar*
atm_get_config_subdir (const Configuration* cfg, gint type);
#endif
