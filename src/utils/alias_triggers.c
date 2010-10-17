/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* alias_triggers.c:					  		  *
*                2005 Tomas Mecir  ( kmuddy@kmuddy.net )                  *
*		 2005 Shlykov Vasiliy ( vash@zmail.ru )                   *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <glib/gprintf.h>
#include <pcre.h>
#include <iscript.h>
#include <script.h>
#include <interface.h>
#include "alias_triggers.h"

//static gint atm_uid = 1;

ATM*
atm_new (void)
{
  return g_new0 (ATM, 1);
}

/**
 * atm_add_error: Adds an error to ATM's error list.
 *
 * @atm:   A #ATM.
 * @error: A #GError;
 *
 **/
static inline void
atm_add_error (ATM* atm, GError* error)
{
  g_assert (atm);
  atm->errors = g_list_append (atm->errors, g_strdup (error->message));

  //atm->errors = g_list_append (atm->errors, g_error_copy (error));
}

/**
 * atm_free: Frees.
 *
 * @atm: A #ATM.
 *
 **/
void
atm_free (ATM* atm)
{
  if (! atm)
      return;

  g_free (atm->name);
  g_free (atm->text);
  g_free (atm->raiser);
  g_free (atm->source);
  atm_clear_errors (atm);

  g_free (atm);
}

/**
 * atm_detect_type: Tests the extension of specified file.
 *
 * @file: File name.
 *
 * @Return value: #ATMLang.
 *
 **/
int
atm_detect_type (const gchar* file)
{
  int ret = ATM_LANG_BASIC;

  if (NULL == file)
      return ret;

  if (! g_str_has_suffix (file, ".bas")
	  && ! g_str_has_suffix (file, ".BAS"))
    {
      ret = ATM_LANG_PYTHON;
    }

  return ret;
}

/**
 * atm_init:
 *
 **/
void
atm_init (ATM* atm, int type, const gchar* name, const gchar* text,
                    int lang, const gchar* source, const gchar* raiser, int action, int disabled)
{
  if (NULL == atm)
      return;

  g_assert (name);

  atm->type = type;
  atm->name = g_strdup (name);
  if (text) atm->text = g_strdup (text);
  else atm->text = NULL;
  atm->lang = lang;
  atm->raiser = g_strdup (raiser);
  atm->action = action;
  atm->disabled = disabled;
  if (source) atm->source = g_strdup (source);
  else atm->source = NULL;
}

/**
 * atm_set_masters: Sets owners of given #ATM.
 *
 * @atm: A #ATM.
 * @cfg: A #Configuration, should be non-NULL.
 * @ss : A #Session.
 *
 **/

void
atm_set_masters (ATM* atm, Configuration* cfg, Session* ss)
{
  g_assert (atm);
  g_assert (cfg);

  atm->config = cfg;
  atm->session = ss;
}

/**
 * atm_init_macro:
 *
 **/
void
atm_init_macro (ATM* atm, const gchar* name, const gchar* text,
                          int lang, const gchar* fname, const gchar* raiser, int action)
{
  atm_init (atm, ATM_MACRO, name, text, lang, fname, raiser, action, 0);
}

/**
 * atm_init_alias:
 *
 **/
void
atm_init_alias (ATM* atm, const gchar* name, const gchar* text,
                          int lang, const gchar* fname, const gchar* raiser, int action)
{
  atm_init (atm, ATM_ALIAS, name, text, lang, fname, raiser, action, 0);
}

/**
 * atm_init_triiger:
 *
 **/
void
atm_init_trigger (ATM* atm, const gchar* name, const gchar* text,
                          int lang, const gchar* fname, const gchar* raiser, int action)
{
  atm_init (atm, ATM_TRIGGER, name, text, lang, fname, raiser, action, 0);
}

/**
 * atm_macro_in_fire: Check whether macro should runs.
 *
 * @atm:   A #ATM.
 * @state: Keyboard state.
 * @key:   Keyboard key.
 *
 * Return value: TRUE if pressed macro's hotkey,
 *               FALSE otherwise.
 **/
gboolean
atm_macro_in_fire (ATM* atm, gint state, gint key)
{
  gboolean ret;
  gchar*   strkey;
    
  g_assert (atm);

  strkey = internal_key_to_string (state, key);
  ret = ! strcmp (atm->raiser, strkey);

  g_free (strkey);

  return ret;
}

/**
 * atm_build_filename: Builds filename (with path) for specified #ATM.
 *
 * @atm: A #ATM.
 *
 * Return value: Full path to script file.
 *
 **/
static gchar*
atm_build_filename (const ATM* atm)
{
  gchar*       filename;
  const gchar* sdir;
//  gboolean     need_full_path = TRUE;

  g_assert (atm);

  sdir = atm_get_config_subdir (atm->config, atm->type);
/*
  if (! atm->internal)
    {
#     ifdef HAVE_WINDOWS
      need_full_path = (atm->filename != ':');
#     else
      need_full_path = (*atm->filename != G_DIR_SEPARATOR); 
#     endif
    }

  if (need_full_path)
    {
*/		

      filename = g_build_path (G_DIR_SEPARATOR_S, 
                        atm->session == NULL ? atm->config->gamedir : atm->session->slot,
                        sdir, atm->source, NULL);
/*    }
  else
    {
      filename = g_strdup (atm->filename);
    }
*/
  return filename;
}

/**
 * atm_test_script_exist: Tests whether the script file already exists.
 *
 * @atm: A #ATM.
 *
 * Return value: TRUE if script file exists,
 *		 FALSE otherwise.
 *
 **/
gboolean
atm_test_script_exists (const ATM* atm)
{
  gchar*   file;
  gboolean ret;

  file = atm_build_filename (atm);

  ret = g_file_test (file, G_FILE_TEST_EXISTS);

  g_free (file);

  return ret;
}

/**
 * atm_load_script: Loads content of script file and saves in ATM::#text.
 *
 * @amt: A #ATM. Should be initialized.
 *
 * Return value: TRUE if reads successfully, FALSE otherwise. Errors appends
 *               to ATM::#errors.
 **/
gboolean
atm_load_script (ATM* atm)
{
  gchar*       filename;
  gsize        len;
  GError*      error = NULL;
  gboolean     ret = TRUE;
  
  g_assert (atm);
  g_assert (atm->config);
  
  if (atm->source == NULL)
    {
	  atm->errors = g_list_append (atm->errors, "Script file not specified.");
	  return FALSE;
	}
  // TODO: Append an error when filename is NULL.

  filename = atm_build_filename (atm);

  if (atm->text) {
	g_free (atm->text);
	atm->text = NULL;
  }
  if (! g_file_get_contents (filename, &atm->text, &len, &error))
    {
      atm_add_error (atm, error);
      ret = FALSE;
    }

  g_free (filename);

  return ret;
}

/**
 * atm_save_script: Saves script to file.
 *
 * Return value: TRUE if save was successful,
 *               FALSE otherwise.
 **/
gboolean
atm_save_script (ATM* atm)
{
  gchar*       filename;
  gboolean     ret = TRUE;
  GError*      error = NULL;

  g_assert (atm);
  g_assert (atm->config);
	if (ATM_ACTION_SCRIPT != atm->action) return TRUE;
  // Not loaded scripts should not be saved.
  if ( !(atm->source && atm->text)/* == NULL*/)
    {
      return TRUE;
    }

  filename = atm_build_filename (atm);

  if (! g_file_set_contents (filename, atm->text, strlen (atm->text), &error))
    {
      if (error != NULL)
	  atm_add_error (atm, error);
      ret = FALSE;
    }

  g_free (filename);

  return ret;
}

/**
 * atm_get_text: Gets script body for specified #ATM. Reads it from
 *               file if necessary.
 *
 * @atm: Specified #ATM.
 *
 * Return value: Script body.
 *
 **/
const gchar*
atm_get_text (ATM* atm)
{
  g_assert (atm);

  if (atm->text == NULL)
    {
      atm_load_script (atm);
    }

  return atm->text;
}

/**
 * atm_create_names_list:
 *
 **/
gsize
atm_create_names_list (GList* list, gchar*** ret)
{
  GList*        it;
//  gchar**       ret;
  const gchar*  name;
  gsize         ait = 0;

  if (list == NULL)
      return 0;
//  g_assert (*ret);
  
  if (g_list_length (list) == 0)
    {
	  *ret = NULL;
	  return 0;
	}

  *ret = g_new0 (gchar*, g_list_length (list) + 1);

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      name = atm_get_name ((ATM*) it->data);
      (*ret)[ait++] = g_strdup (name);
    }
  (*ret)[g_list_length (list)] = NULL;
	
  return g_list_length (list);
}

/**
 * atm_get_by_expr:
 *
 **/
ATM*
atm_get_by_expr (GList* list, const gchar* expr)
{
  GList* it;
  ATM*   atm;

  g_assert (expr);

  if (list == NULL)
      return NULL;

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      atm = (ATM*) it->data;
      if (atm && atm->raiser && !strcmp (atm->raiser, expr))
          return atm;
    }

  return NULL;
}

gboolean
atm_add_to_list (GList** list, ATM* atm)
{
  ATM* fatm = atm_get_by_expr (*list, atm->raiser);

  if (NULL != fatm)
    {
      GList* fl = g_list_find (*list, fatm);
      g_assert (fl);

      atm_free (fatm);
      fl->data = atm;
      return FALSE;
    }
  else
    {
      *list = g_list_append (*list, atm);
      return TRUE;
    }
}

gboolean
atm_remove_from_list (GList** list, const gchar* expr)
{
  ATM* atm = atm_get_by_expr (*list, expr);

  if (NULL != atm)
    {
      atm_free (atm);
      *list = g_list_remove (*list, atm);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

void
atm_list_clear(GList ** list)
{
  GList* it;

  for (it = g_list_first (*list); it; it = g_list_next (it))
  {
    atm_free ((ATM*) it->data);
  }

  g_list_free (*list);
  *list = NULL;
}

static inline
void atm_format_pcre_error (GList** errors, const gchar* expr,
                                                int offset, const gchar* msg)
{
  gchar* res = g_strdup_printf ("Error parsing expression '%s' at offset %d: %s",
                                        expr, offset, msg);

  *errors = g_list_append (*errors, res);
}

ATM*
atm_find_fire (Session* ss, const gchar* data, gsize len,
                        GList* list, gboolean single, gint* result)
{
  GList*       it;
  ATM*         atm;
  pcre*        re;

  const gchar**  backrefs = NULL;
  const gchar* error;
  int          rc, erroffset, ovector[30];
  ATM*         ret = NULL;

  for (it = g_list_first (list); it; it = g_list_next (it))
  {
      atm = (ATM*) it->data;

/**
 * We will allow regex for all values

      if (atm->type == ATM_ALIAS )
      {
	  if (!strcmp (atm->raiser, data))
	    {
	      ret = atm;
	      *result = atm_execute (ss, atm, NULL, 0);
	    }
      }
      else if (atm->type == ATM_TRIGGER)
	{
**/
	if (!atm->disabled) {
	  re = pcre_compile(atm->raiser, 0, &error, &erroffset, NULL);
	  if (re == NULL)
	    {
	      atm_format_pcre_error (&atm->errors, atm->raiser, erroffset, error);
	    }
	  else
	    {
	      rc = pcre_exec(re, NULL, data, len, 0, 0, ovector, 30);
	      if (rc > 0)	// expresion matched 
		{
		  ret = atm;

		  pcre_get_substring_list (data, ovector, rc, &backrefs);
		  
		  *result = atm_execute (ss, atm, backrefs, rc);

		  pcre_free_substring_list (backrefs);
		}
	      pcre_free(re);
	    }
/*	} */
      if (ret && single)
	  return ret;
	}

    }
  return ret;
}

void
script_err_hndl (int line, int code, const char* msg, void* userdata)
{
  gchar rmsg[80];
  ATM*  at = (ATM*) userdata;

  g_assert (at);

  g_sprintf (rmsg, "ERROR:%d:%s\n", line, msg);

  at->errors = g_list_append (at->errors, g_strdup (msg));
}

static inline gint
atm_execute_python (SESSION_STATE* session, ATM* at,
		    const gchar *backrefs[], gsize backrefcnt)
{
  return script_execute (session, at, backrefs, backrefcnt);
}

void
atm_basic_message_box (IScript* is, int argc, const char** argv, void** data)
{
  interface_display_message ((gchar*)argv[0]);
}

static gint
atm_execute_basic (SESSION_STATE* session, ATM* at,
		    const gchar *backrefs[], gsize backrefcnt)
{
  static IScriptExtFuncInfo message_box_func_info =
  {
      "MESSAGEBOX",
      1,
      &atm_basic_message_box,
      0
  };

  IScript*  is;
  gsize     i;
  gchar     extv[10],
            outbuf[256];
  gint      ret;
  const gchar* text;
  *outbuf = '\0';
  
  //fprintf (stderr, "atm_execute_basic: '%s'\n", atm_get_text (at, session));
  text = atm_get_text (at);

  if (text == NULL)
    {
      return FALSE;
    }

  is = iscript_new ();
  iscript_init (is);
  iscript_set_err_handler (is, &script_err_hndl, at);
  iscript_ext_func_add (is, &message_box_func_info);

  for (i = 0; i < backrefcnt && backrefs; i++)
    {
      g_sprintf (extv, "%d", i);
      iscript_ext_var_add (is, extv, backrefs[i]);
    }
 
  ret = iscript_run (is, text, outbuf, 256);

  iscript_free (is);
  
  if (ret && *outbuf != '\0')
  {
      gsize size = strlen(outbuf);
      g_message( "kyndig: outbuf is %s - %d", outbuf, strlen(outbuf) );
      send_command(session, outbuf, size);
  }
  return ret;
}

/**
 * atm_execute_script: Runs specified script.
 *
 * Return value: TRUE if successfull,
 *               FALSE otherwise.
 *
 **/
gint atm_execute_script (SESSION_STATE* session, ATM* atm,
		    const gchar** backrefs, gsize backrefcnt)
{
	gint ret = FALSE;
  g_assert (atm);

# if DBG_ATM > 0
  gsize i;

  mdebug (DBG_ATM, 0, "atm_executen: %d\n", atm->type);
  mdebug (DBG_ATM, 1, "name\t[%s]\n", atm->name);
  mdebug (DBG_ATM, 1, "raiser\t[%s]\n", atm->raiser);
  mdebug (DBG_ATM, 1, "lang\t[%s]\n", atm->lang == ATM_LANG_BASIC ? "BASIC" : "PYTHON");
  mdebug (DBG_ATM, 1, "fname\t[%s]\n", atm->filename);
  mdebug (DBG_ATM, 1, "parms\t[%d]\n", backrefcnt);
  for (i = 0; i < backrefcnt; i++)
      mdebug (DBG_ATM, 2, "%d: %s\n", i, backrefs[i]);

  if (atm->text)
    {
      mdebug (DBG_ATM, 1, "text\n");
      mdebug (DBG_ATM, 0, atm->text);
    }
# endif

  switch (atm->lang)
  {
      case ATM_LANG_BASIC :
		ret = atm_execute_basic (session, atm, backrefs, backrefcnt);
      break;

      case ATM_LANG_PYTHON : 
		ret = atm_execute_python (session, atm, backrefs, backrefcnt);
      break;

      default : 
	g_assert (0);
      break;
  }

  return ret;
}

gint atm_execute_text (SESSION_STATE * session, ATM * atm, const gchar ** backrefs, gsize backrefcnt) {
	g_assert (atm);
	g_assert (atm->source);
	send_command (session, atm->source, strlen (atm->source));
	return TRUE;
}

gint atm_execute_noise (SESSION_STATE * session, ATM * atm, const gchar ** backrefs, gsize backrefcnt) {
	g_assert (atm);
	g_assert (atm->source);
	if (utils_play_file (atm->source)) {
		GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			(gchar *) "Playing sound file '%s' failed",
			atm->source
		));
		gtk_dialog_run (GTK_DIALOG (md));
		gtk_widget_destroy (GTK_WIDGET (md));
	}
}

gint atm_execute_popup (SESSION_STATE * session, ATM * atm, const gchar ** backrefs, gsize backrefcnt) {
	GtkMessageDialog * md = GTK_MESSAGE_DIALOG (gtk_message_dialog_new (
		NULL,
		GTK_DIALOG_MODAL,
		GTK_MESSAGE_WARNING,
		GTK_BUTTONS_OK,
		"%s",
		atm->source
	));
	gtk_dialog_run (GTK_DIALOG (md));
	gtk_widget_destroy (GTK_WIDGET (md));
	return TRUE;
}

/**
 * atm_execute: Runs specified action.
 *
 * Return value: TRUE if successfull,
 *               FALSE otherwise.
 *
 **/
gint atm_execute (SESSION_STATE * session, ATM * atm, const gchar ** backrefs, gsize backrefcnt) {
	g_assert (atm);
	switch (atm->action) {
		case ATM_ACTION_TEXT: return atm_execute_text (session, atm, backrefs, backrefcnt);
		case ATM_ACTION_SCRIPT: return atm_execute_script (session, atm, backrefs, backrefcnt);
		case ATM_ACTION_NOISE: return atm_execute_noise (session, atm, backrefs, backrefcnt);
		case ATM_ACTION_POPUP: return atm_execute_popup (session, atm, backrefs, backrefcnt);
		default: fprintf (stderr, "atm_execute: unknown action type: %d\n", atm->action);
	}
	return FALSE;
}

const gchar*
atm_get_config_subdir (const Configuration* cfg, gint type)
{
  g_assert (cfg);
  
  switch (type)
    {
      case ATM_ALIAS : return cfg->aliasdir;
      case ATM_TRIGGER : return cfg->triggerdir;
      case ATM_MACRO : return cfg->macrodir;
      default : g_assert (0); return NULL;
    }
}

/**
 * atm_clear_errors: Clears errors list.
 *
 **/
void
atm_clear_errors (ATM* atm)
{
  g_assert (atm);

  utils_clear_errors (&atm->errors);
}

