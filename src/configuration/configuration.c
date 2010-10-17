/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* configuration.c:                                                        *
*                2004 Calvin Ellis  ( kyndig@mudmagic.com )               *
*                2005 Shlykov Vasiliy ( vash@zmail.ru )                   *
*                2006 Victor Vorodyukhin ( victor.scorpion@gmail.com )   *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

//#include <glib/glist.h>
#include <glib/gstdio.h>
//#include <gtk/gtk.h>
#include <mudmagic.h>
#include <cmdentry.h>
#include <alias_triggers.h>
#include <gauges.h>
#include <statusvars.h>
#include <callbacks.h>
#include <utils.h>
#include <module.h>
#include <gamelist.h>
#include <stdlib.h>
#include "proxy.h"
#include "tools.h"

#define MUDCFG_HOME_DIR         ".mudmagic"
#define MUDCFG_CONFIG_FILE      "config2"
#define MUDCFG_CONFIG_OLD_FILE  "config"
#define MUDCFG_SAVES_DIR        "saves"
#define MUDCFG_IMAGES_DIR       "images"
#define DEFAULT_MACROS_DIR      "macros"
#define DEFAULT_ALIAS_DIR       "alias"
#define DEFAULT_TRIGGER_DIR     "trigger"
#define MUDCFG_LOG_FILE         "mudmagic.log"
#define MUDCFG_GAMELIST_GZ_FILE "gamelist.gz"
#define MUDCFG_GAMELIST_FILE    "gamelist.xml"
#define MUDCFG_GAMELIST_URL     "http://www.mudmagic.com/listings/clientlist.gz"

#define MUDCFG_MAIN_GROUP       "Global"
#define MUDCFG_MACROSES_GROUP   "Macroses"
#define MUDCFG_TRIGGERS_GROUP   "Triggers"
#define MUDCFG_ALIASES_GROUP    "Aliases"
#define MUDCFG_MODULES_GROUP    "Modules"
#define MUDCFG_PROXIES_GROUP    "Proxies"

#define MUDCFG_CONFIG_SLOT_FILE     "slot.cfg"
#define MUDCFG_CONFIG_SLOT_OLD_FILE "config"
#define MUDCFG_SLOT_VARIABLES_FILE  "variables"
#define MUDCFG_SLOT_STATUSVARS_FILE  "statusvars"
#define MUDCFG_SLOT_GAUGES_FILE  "gauges"
#define MUDCFG_SLOT_OWIN_FILE  "owindows"
#define MUDCFG_SLOT_HISTORY_FILE    "history"


#define MUDCFG_SLOT_MAIN_GROUP      "Global"
#define MUDCFG_SLOT_MACROSES_GROUP  "Macroses"
#define MUDCFG_SLOT_TRIGGERS_GROUP  "Triggers"
#define MUDCFG_SLOT_ALIASES_GROUP   "Aliases"

#define MUDCFG_DEFAULT_BROWSER "/usr/bin/mozilla"
#define MUDCFG_WIN_DEFAULT_BROWSER "iexplore.exe"

enum
{
  CONFIGURATION_SIGNAL,
  LAST_SIGNAL
};

enum ConfigDirRequest {
	CONFIG_DIR_ALIAS,
	CONFIG_DIR_MACRO,
	CONFIG_DIR_TRIGGER
};

static void configuration_class_init      (ConfigurationClass *klass);
static void configuration_init            (Configuration      *ttt);

// language list for scripting engine
const ATMLanguage Languages [ATMLanguageCount] = {
	{"Python", ATM_LANG_PYTHON},
	{"Basic", ATM_LANG_BASIC}
};

#if 0
#ifndef HAVE_WINDOWS
#define DEFAULT_AUDIO_CONF      \
"mp3cmd = \n"                   \
"wavcmd = \n"                   \
"midcmd = \n"                   
#else
#define DEFAULT_AUDIO_CONF      ""
#endif

static const char default_config_file[] =
"["MUDCFG_MAIN_GROUP"]\n"
"download=true\n"
"entry_separator=true\n"
"keepsent=false\n"
"cmd_buffer_size="#DEFAULT_CMD_BUF_SIZE"\n"
"cmd_autocompletion="#DEFAULT_CMD_AUTOCOMPLETION"\n"
DEFAULT_AUDIO_CONF
"\n"
"["MUDCFG_MODULES_GROUP"]\n"
"Dir=\n"
"List=\n"
"\n"
"["MUDCFG_MACROSES_GROUP"]\n"
"Dir=\n"
"List=\n"
"\n"
"["MUDCFG_ALIASES_GROUP"]\n"
"Dir=\n"
"List=\n"
"\n"
"["MUDCFG_TRIGGERS_GROUP"]\n"
"Dir=\n"
"List=\n"
"\n";
#endif

static const char default_config_file[] =
"["MUDCFG_MAIN_GROUP"]\n"
"["MUDCFG_MODULES_GROUP"]\n"
"["MUDCFG_MACROSES_GROUP"]\n"
"["MUDCFG_ALIASES_GROUP"]\n"
"["MUDCFG_TRIGGERS_GROUP"]\n"
"["MUDCFG_PROXIES_GROUP"]\n";

static const char default_slot_config_file[] =
"["MUDCFG_MAIN_GROUP"]\n"
"["MUDCFG_MACROSES_GROUP"]\n"
"["MUDCFG_ALIASES_GROUP"]\n"
"["MUDCFG_TRIGGERS_GROUP"]\n";

static guint configuration_signals[LAST_SIGNAL] = { 0 };


CONFIGURATION *config;

EXPORT
CONFIGURATION *get_configuration ()
{
  return config;
}


char * config_get_dir (int req, SESSION_STATE * ses) {
	char * typedir = NULL;

	if (!ses) {
		switch (req) {
			case CONFIG_DIR_ALIAS: return get_configuration ()->aliasdir;
			case CONFIG_DIR_MACRO: return get_configuration ()->macrodir;
			case CONFIG_DIR_TRIGGER: return get_configuration ()->triggerdir;
			default:
				fprintf (stderr, "config_get_dir: unknown directory request (%d)\n", req);
				return NULL;
		}
	}
	switch (req) {
		case CONFIG_DIR_ALIAS: typedir = "alias"; break;
		case CONFIG_DIR_MACRO: typedir = "macro"; break;
		case CONFIG_DIR_TRIGGER: typedir = "trigger"; break;
		default:
			fprintf (stderr, "config_get_dir: unknown directory request (%d)\n", req);
			return NULL;
	}
	return g_build_path (G_DIR_SEPARATOR_S, ses->slot, typedir, NULL);
}

// returns language name by id
const char * config_get_lang_name (int id) {
	int i;

	for (i = 0; i < ATMLanguageCount; i++) {
		if (Languages [i].id == id) return Languages [i].name;
	}
	fprintf (stderr, "config_get_lang_name: unknown lanuage id (%d)\n", id);
	return NULL;
}

int config_get_lang_id (char * lang) {
	int i;

	for (i = 0; i < ATMLanguageCount; i++) {
		if (!g_ascii_strcasecmp (Languages [i].name, lang)) return Languages [i].id;
	}
	fprintf (stderr, "config_get_lang_id: unknown lanuage name (%s)\n", lang);
	return -1;
}

const char * config_get_action_name (int id) {
	switch (id) {
		case ATM_ACTION_TEXT: return "Text";
		case ATM_ACTION_SCRIPT: return "Script";
		case ATM_ACTION_NOISE: return "Sound/Music";
		case ATM_ACTION_POPUP: return "Popup message";
	}
	fprintf (stderr, "config_get_action_name: unknown action id (%d)\n", id);
	return NULL;
}

int config_get_action_id (char * name) {
	if (!g_ascii_strcasecmp (name, "Sound/Music")) return ATM_ACTION_NOISE;
	else if (!g_ascii_strcasecmp (name, "Text")) return ATM_ACTION_TEXT;
	else if (!g_ascii_strcasecmp (name, "Script")) return ATM_ACTION_SCRIPT;
	else if (!g_ascii_strcasecmp (name, "Popup message")) return ATM_ACTION_POPUP;
	fprintf (stderr, "config_get_action_id: unknown action name (%s)\n", name);
	return -1;
}

GType
configuration_get_type (void)
{
  static GType cfg_type = 0;

  if (!cfg_type)
    {
      static const GTypeInfo cfg_info =
      {
  sizeof (ConfigurationClass),
  NULL, /* base_init */
        NULL, /* base_finalize */
  (GClassInitFunc) configuration_class_init,
        NULL, /* class_finalize */
  NULL, /* class_data */
        sizeof (Configuration),
  0,
  (GInstanceInitFunc) configuration_init,
      };

      cfg_type = g_type_register_static (GTK_TYPE_OBJECT, "Configuration", &cfg_info, 0);
    }

  return cfg_type;
}

static void
configuration_class_init (ConfigurationClass *klass)
{
  configuration_signals[CONFIGURATION_SIGNAL] =
         g_signal_new ("changed",
         G_TYPE_FROM_CLASS (klass),
                           G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                           G_STRUCT_OFFSET (ConfigurationClass, configuration),
                                 NULL,
                                 NULL,
         g_cclosure_marshal_VOID__VOID,
                                 G_TYPE_NONE, 0);
}

GtkObject*
configuration_new ()
{
  return GTK_OBJECT (g_object_new (configuration_get_type (), NULL));
}

static void
config_strip_nl (gchar* str)
{
  gchar* ptr = str + strlen (str);

  if (ptr == str) // empty string
      return;

  ptr--;

  while (*ptr == '\n' || *ptr == '\r')
      *ptr-- = '\0';

}

/**
 * config_migrate_atm: Creates new style ALIAS/TRIGGER/MACROS from old-style
 *                     configuration data.
 *
 **/
ATM*
config_migrate_atm (int type, const gchar* expr, int cmds,
                        gchar* commands[], const gchar* file, Session* session)
{
  static int auid = 1;
  static int muid = 1;
  static int tuid = 1;

  static const gchar mig_fmt[] = "PRINT \"%s\"\n";

  gsize  i = 0;
  gchar* text;
  gsize  len = 0;
  ATM*   atm = atm_new ();
  gchar  name[9];
  int    ltype;
  gchar  fname[13];
  gsize  mig_fmt_len = strlen (mig_fmt);
  gboolean intr = TRUE;
  const gchar* filename;

  for (; i < cmds; i++)
      len += strlen (commands[i]) + mig_fmt_len;

  text = g_malloc (len + 1);
  *text = '\0';

  for (len = 0, i = 0; i < cmds; i++)
    {
      config_strip_nl (commands[i]);
      len += g_sprintf (text + len, mig_fmt, commands[i]);
    }

  switch (type)
    {
      case ATM_ALIAS   : sprintf (name, "alia%.4d", auid++); break;
      case ATM_TRIGGER : sprintf (name, "trig%.4d", tuid++); break;
      case ATM_MACRO   : sprintf (name, "macr%.4d", muid++); break;
      default : g_assert (0);
    }

  intr = file == NULL;
  if (NULL == file)
    {
      ltype = ATM_LANG_BASIC;
      sprintf (fname, "%s.bas", name);
      filename = fname;
    }
  else
    {
      ltype = ATM_LANG_PYTHON;
      filename = file;
    }

  switch (type)
    {
      case ATM_ALIAS   : atm_init_alias   (atm, name, text, ltype, NULL, expr, ATM_ACTION_SCRIPT);
                         break;
      case ATM_TRIGGER : atm_init_trigger (atm, name, text, ltype, NULL, expr, ATM_ACTION_SCRIPT);
                         break;
      case ATM_MACRO   : atm_init_macro   (atm, name, text, ltype, NULL, expr, ATM_ACTION_SCRIPT);
                         break;
      default : g_assert (0);
    }

  atm_set_masters (atm, get_configuration (), session);

  g_free (text);

  return atm;
}

#ifdef HAVE_WINDOWS
#include <windows.h>
static int
config_migrate_file (const gchar* srcdir, const gchar* dstdir,
                     const gchar* sfile, const gchar* dfile)
{
  gchar* src,
       * dst;
  int    ret;

  SHFILEOPSTRUCT oper =
    {
      NULL,     //hWnd
      FO_COPY,  //wFunc
      NULL,     //pFrom
      NULL,     //pTo
      FOF_NOCONFIRMATION | FOF_SILENT,
      TRUE,     //fAnyOperationsAborted
      NULL,     //hNameMapping
      NULL      //lpszProgressTitle
    };
      
  src = g_build_path (G_DIR_SEPARATOR_S, srcdir, sfile, NULL);
  dst = g_build_path (G_DIR_SEPARATOR_S, dstdir, dfile, NULL);

  oper.pFrom = src;
  oper.pTo = dst;
 
  ret = SHFileOperation (&oper);

  mdebug (DBG_CONFIG, 0,"Migrating `%s` -> `%s` - %s\n", src, dst, ret == 0 ? "OK" : "FAILED");

  g_free (src);
  g_free (dst);

  return ret;
}

static void
config_migrate_to_new_style (Configuration* cfg)
{
  gchar* srcdir,
       * dstdir;

  srcdir = g_strdup (".");
  dstdir = g_build_path (G_DIR_SEPARATOR_S, cfg->basepath, MUDCFG_HOME_DIR, NULL);

  utils_mkdir(dstdir);
  mdebug (DBG_CONFIG, 0,"Creating: %s\n", dstdir);
  
  config_migrate_file (srcdir, dstdir, MUDCFG_CONFIG_OLD_FILE, MUDCFG_CONFIG_OLD_FILE);
  config_migrate_file (srcdir, dstdir, MUDCFG_SAVES_DIR, MUDCFG_SAVES_DIR);
  config_migrate_file (srcdir, dstdir, MUDCFG_IMAGES_DIR, MUDCFG_IMAGES_DIR);
  config_migrate_file (srcdir, dstdir, MUDCFG_GAMELIST_FILE, MUDCFG_GAMELIST_FILE);
  config_migrate_file (srcdir, dstdir, MUDCFG_GAMELIST_GZ_FILE, MUDCFG_GAMELIST_GZ_FILE);

  g_free (srcdir);
  g_free (dstdir);
}
#endif
/**
 * config_check_new_version: Checks whether new style config should be loads.
 *
 * @cfg: A #Configuration.
 *
 * Return value: TRUE if new style configuration should be used,
 *               FALSE otherwise.
 **/
static gboolean
config_check_new_version (Configuration* cfg)
{
  gchar*   mudhome,
       *   config;
  gboolean ret = TRUE;
  
  mudhome = g_build_path (G_DIR_SEPARATOR_S, cfg->basepath, MUDCFG_HOME_DIR, NULL);
  config  = g_build_path (G_DIR_SEPARATOR_S, mudhome, MUDCFG_CONFIG_FILE, NULL);
  
  // Checking for new style config (=> 1.9 version)
  if (! g_file_test (config, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    {
       gchar* mudhome_old,
            * config_old;

       mdebug (DBG_CONFIG, 0,"New style cfg '%s' doesn't exist\n", config);
# ifdef HAVE_WINDOWS
       mudhome_old = ".";
# else
       mudhome_old = mudhome;
# endif
       config_old = g_build_path (G_DIR_SEPARATOR_S, mudhome_old, MUDCFG_CONFIG_OLD_FILE, NULL);

       if (g_file_test (config_old, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
         {
           ret = FALSE; 
         }
       mdebug (DBG_CONFIG, 0,"Checking %s: %s\n", config_old, ret ? "NOT FOUND" : "FOUND" );
       //If any config file not found, will use new scheme
       g_free (config_old);
    }

  g_free (mudhome);
  g_free (config);
  return ret;
}

gchar*
config_get_log_path (void)
{
  gchar* basepath = utils_get_home_dir ();
  gchar* ret = NULL;

  if (basepath != NULL)
    {
      ret = g_build_path(G_DIR_SEPARATOR_S, basepath, MUDCFG_HOME_DIR,
                                 MUDCFG_LOG_FILE, NULL);
    }

  g_free (basepath);
  return ret;
}

/**
 * configuration_init: Initializes a #Configuration object by default values.
 *
 **/
static void
configuration_init (Configuration *ret)
{
  ret->cfgfile        = NULL;
    
  ret->file           = NULL;
  ret->cfg_errors     = NULL;

  ret->download       = TRUE;
  ret->upgrade        = FALSE;
  ret->cmd_autocompl  = DEFAULT_CMD_AUTOCOMPLETION;
  ret->cmd_buf_size   = DEFAULT_CMD_BUF_SIZE;
  ret->macros         = NULL;
  ret->gamelisturl    = MUDCFG_GAMELIST_URL;
  ret->gamelist       = NULL;
  ret->proxies        = NULL;
  ret->windows        = NULL;
    
  ret->basepath       = utils_get_home_dir ();
  // May be NULL on Windows platform
  if (NULL == ret->basepath)
    {
      g_critical ("Cannot get home directory.");
      return; //TODO: Push error to list instead of call g_critical.
    }

  ret->filename   = g_build_path(G_DIR_SEPARATOR_S, ret->basepath, MUDCFG_HOME_DIR,
                                 MUDCFG_CONFIG_FILE, NULL);
  
  // Maybe old-style configuration?
  ret->upgrade = ! config_check_new_version (ret);

  if (ret->upgrade)
    {
      mdebug (DBG_CONFIG, 0, "Reading old style config\n");
#ifdef HAVE_WINDOWS
      config_migrate_to_new_style (ret);
#endif
      ret->cfgfile = g_build_path (G_DIR_SEPARATOR_S, ret->basepath, MUDCFG_HOME_DIR,
                              MUDCFG_CONFIG_OLD_FILE, NULL);
    }
  else
    {
      ret->cfgfile = g_strdup (ret->filename);
    }

#ifndef HAVE_WINDOWS
  ret->mp3cmd  = g_strdup ("");
  ret->wavcmd  = g_strdup ("");
  ret->midcmd  = g_strdup ("");
#endif

  ret->gamedir    = g_build_path (G_DIR_SEPARATOR_S, ret->basepath, MUDCFG_HOME_DIR, NULL);
    
  ret->savedir    = g_build_path (G_DIR_SEPARATOR_S, ret->basepath, MUDCFG_HOME_DIR,
                                  MUDCFG_SAVES_DIR, NULL);
  ret->imagedir   = g_build_path (G_DIR_SEPARATOR_S, ret->basepath, MUDCFG_HOME_DIR,
                                  MUDCFG_IMAGES_DIR, NULL);

  ret->gamelistfile = g_build_path (G_DIR_SEPARATOR_S, ret->gamedir, MUDCFG_GAMELIST_FILE, NULL);
  /*
  ret->macrodir   = g_build_path(G_DIR_SEPARATOR_S, ret->basepath, MUDCFG_HOME_DIR,
                                 DEFAULT_MACROS_DIR, NULL);
  ret->aliasdir   = g_build_path(G_DIR_SEPARATOR_S, ret->basepath, MUDCFG_HOME_DIR,
                                 DEFAULT_ALIAS_DIR, NULL);
  ret->triggerdir = g_build_path(G_DIR_SEPARATOR_S, ret->basepath, MUDCFG_HOME_DIR,
                                 DEFAULT_TRIGGER_DIR, NULL);
  */
  ret->macrodir   = g_strdup (DEFAULT_MACROS_DIR);
  ret->aliasdir   = g_strdup (DEFAULT_ALIAS_DIR);
  ret->triggerdir = g_strdup (DEFAULT_TRIGGER_DIR);
  
  mdebug (DBG_CONFIG, 0,"Base path: %s\n", ret->basepath);
  mdebug (DBG_CONFIG, 0,"Game dir: %s\n", ret->gamedir);
  mdebug (DBG_CONFIG, 0,"Config file: %s\n", ret->filename);

  utils_mkdir(ret->gamedir);
  utils_mkdir(ret->savedir);
  utils_mkdir(ret->imagedir);

  module_init(&ret->modules);
}

void configuration_end(CONFIGURATION * config)
{
  module_end(config->modules);
  configuration_delete(config);
  config = NULL;
}

static inline void
add_gerror (GList** gerrs, GError* gerr)
{
  *gerrs = g_list_append (*gerrs, g_error_copy (gerr));
}

static inline void
config_add_gerror (Configuration* cfg, GError* gerror)
{
  g_assert (gerror);

  cfg->cfg_errors = g_list_append (cfg->cfg_errors, g_error_copy (gerror));
}

gboolean
utils_get_next(FILE * f, gchar ** key, gchar ** expr,
         int* cmds, char** commands[], gchar ** file)
{
    const int buff_len = 512; // the maximum length of a line 
    gchar buff[buff_len];
    gchar *p = NULL;
    gchar *s = NULL;
    if (key)
            *key = NULL;
    if (expr)
            *expr = NULL;
    if (commands)
            *commands = NULL;
    if (file)
            *file = NULL;
    if (f == NULL)
            return FALSE;

    while (TRUE)
    {
        memset(buff, 0, buff_len);
        if (fgets(buff, buff_len - 1, f) != NULL)
        {
            // if we have some data ...
            p = g_strstr_len(buff, buff_len, "=");
            if (p != NULL)
            {
                    // ... and we have a initialization

                    // what is in the left of "=" is key 
                    s = g_strndup(buff, p - buff);
                    g_strstrip(s);
                    if (key)
                            *key = g_strdup(s);
                    g_free(s);


                    // what is in the rigth of "=" is value
                    s = g_strdup(p + 1);
                    g_strstrip(s);
                    // strip trailing newline
                    if (s[strlen(s) - 1] == '\n')
                            s[strlen(s) - 1] = '\0';
                    if (expr)
                            *expr = g_strdup(s);
                    g_free(s);

                    // and in some case we have some extra data
                    if (g_str_has_prefix(*key, "alias") ||
                        g_str_has_prefix(*key, "macro") ||
                        g_str_has_prefix(*key, "trigger") ||
                        g_str_has_prefix(*key, "description"))
                    {
                            // read until EOF 
                            gchar line[buff_len];

                            while (TRUE) {
                                    memset(line, 0, buff_len);
                                    if (fgets
                                        (line, buff_len - 1,
                                         f) != NULL) {
                                            if (line
                                                [strlen(line) -
                                                 1] == '\n')
                                                    line[strlen
                                                         (line)
                                                         - 1] =
                                                        '\0';
                                            if (g_str_has_prefix(line, "EOF")) {
                                                    break;
                                            } else
                                                if
                                                (g_str_has_prefix
                                                 (line,
                                                  "FILE:")) {
                                                    if (file)
                                                            *file
                                                                =
                                                                g_strdup
                                                                (line
                                                                 +
                                                                 5);
                                            } else
                                                if
                                                (g_str_has_prefix
                                                 (line,
                                                  "CMDS:")) {
                                                    int i;
                                                    if (cmds) {
                                                            *cmds
                                                                =
                                                                0;
                                                            sscanf
                                                                (line
                                                                 +
                                                                 5,
                                                                 "%d",
                                                                 cmds);
                                                            if (*cmds) {
                                                                    *commands
                                                                        =
                                                                        g_new0
                                                                        (char
                                                                         *,
                                                                         *cmds);
                                                                    for (i = 0; i < *cmds; ++i) {
                                                                            fgets
                                                                                (line,
                                                                                 buff_len
                                                                                 -
                                                                                 1,
                                                                                 f);
                                                                            (*commands)[i] = g_strdup(line);
                                                                    }
                                                            }
                                                    }
                                            }
                                    } else {  // if we are here we have a broken 
                                            // format
                                            // proceed with what we have, hoping for the
                                            // best
                                            break;
                                    }
                            }
                    }
                    return TRUE;
            }
        } else {
                return FALSE;
        }
    }
}

void free_proxy (Proxy * p, gpointer user_data) {
	proxy_struct_free (p);
	g_free (p);
}

void configuration_delete(CONFIGURATION * config)
{
  g_free(config->cfgfile);

  if (config->filename)
    g_free(config->filename);
  if (config->gamedir)
    g_free(config->gamedir);
  if (config->savedir)
    g_free(config->savedir);
#ifndef HAVE_WINDOWS
  if (config->mp3cmd)
    g_free(config->mp3cmd);
  if (config->wavcmd)
    g_free(config->wavcmd);
  if (config->midcmd)
    g_free(config->midcmd);
#endif        // HAVE_WINDOWS

        g_free (config->macrodir);
        g_free (config->aliasdir);
        g_free (config->triggerdir);

  gl_gamelist_free (config->gamelist);

  atm_list_clear(&config->triggers);
  atm_list_clear(&config->aliases);
  atm_list_clear(&config->macros);

        utils_clear_gerrors (&config->cfg_errors);
        g_key_file_free (config->file);
	g_list_foreach (g_list_first (config->proxies), (GFunc) free_proxy, NULL);
	g_list_free (config->proxies);
	if (config->acct_user) g_free (config->acct_user);
	if (config->acct_passwd) g_free (config->acct_passwd);
	if (config->help_browser) g_free (config->help_browser);
}

#if 0
static gboolean
config_backup_old (Configuration* cfg)
{
  if (cfg->upgrade)
    {
      gint   res;
      gchar* oldfname = g_malloc (strlen (cfg->filename) + 5);

      strcpy (oldfname, cfg->filename);
      strcat (oldfname, ".old");

      res = g_rename (cfg->filename, oldfname);

      if (res == -1)
        {
          return FALSE;
        }
    }
  return TRUE;
}
#endif

/**
 * config_save_string:
 *
 **/
static inline void
config_save_string (GKeyFile* file, const gchar* group, const gchar* key, const gchar* str)
{
  g_key_file_set_string (file, group, key, str != NULL ? str : "");
}

/**
 * config_save_int:
 *
 **/
static inline void
config_save_int (GKeyFile* file, const gchar* group, const gchar* key, gint i)
{
  g_key_file_set_integer (file, group, key, i);
}

/**
 * config_save_bool:
 *
 **/
static inline void
config_save_bool (GKeyFile* file, const gchar* group, const gchar* key, gboolean b)
{
  g_key_file_set_boolean (file, group, key, b);
}

/**
 * config_save_atm: Saves configuration and script body of specified #ATM.
 *
 * @atm:       Given #ATM.
 * @keyfile:   Configuration file.
 * @group:     Group for ATM into @keyfile.
 * @dir:       Base path for saving.
 * @errlist:   Where place possible errors.
 *
 * Return value: TRUE if save was successful, FALSE otherwise.
 *
 **/
static gboolean
config_save_atm (ATM* atm, GKeyFile* keyfile, const gchar* group, const gchar* dir, GList** errlist)
{
  GError*  gerror = NULL;
  gboolean ret;
    
  if ((ATM_ACTION_SCRIPT == atm->action) && !atm->source) {
	// allocate new file for script body
	char * fn = g_build_path (G_DIR_SEPARATOR_S, dir, "scrXXXXXX", NULL);
	int fd = g_mkstemp (fn);
	if (-1 == fd) {
		fprintf (stderr, "unable to open file %s\n", fn);
		g_free (fn);
		return FALSE;
	} else {
		close (fd);
		atm->source = g_path_get_basename (fn);
		g_free (fn);
	}
  }
  config_save_string (keyfile, group, "Name", atm->name);
  if (ATM_ACTION_SCRIPT == atm->action) config_save_string (keyfile, group, "Language", config_get_lang_name (atm->lang));
  config_save_string (keyfile, group, "Action", config_get_action_name (atm->action));
  config_save_string (keyfile, group, "Raiser", atm->raiser);
  if (atm->source) config_save_string (keyfile, group, "Source", atm->source);
  config_save_int (keyfile, group, "Disabled", atm->disabled);

  ret = atm_save_script (atm);
  if (gerror != NULL)
    {
      add_gerror (errlist, gerror);
    }

  return ret;
}

/**
 * config_save_macro: Saves given macro.
 *
 **/
static gboolean
config_save_macro (ATM* atm, GKeyFile* keyfile, const gchar* dir, GList** errlist)
{
  gchar         group[80];
  const gchar*  name;

  g_assert (atm);
  g_assert (errlist);

  name = atm_get_name (atm);
  g_snprintf (group, 80, "Macro:%s", name);

  return config_save_atm (atm, keyfile, group, dir, errlist);
}

/**
 * config_save_alias: Saves given alias.
 *
 * @dir: base path for saving script's body
 *
 **/
static gboolean
config_save_alias (ATM* atm, GKeyFile* keyfile, const gchar* dir, GList** errlist)
{
  gchar         group[80];
  const gchar*  name;

  g_assert (atm);
  g_assert (errlist);

  name = atm_get_name (atm);
  g_snprintf (group, 80, "Alias:%s", name);

  return config_save_atm (atm, keyfile, group, dir, errlist);
}

/**
 * config_save_trigger: Saves given trigger.
 *
 **/
static gboolean
config_save_trigger (ATM* atm, GKeyFile* keyfile, const gchar* dir, GList** errlist)
{
  gchar         group[80];
  const gchar*  name;

  g_assert (atm);
  g_assert (errlist);

  name = atm_get_name (atm);
  g_snprintf (group, 80, "Trigger:%s", name);

  return config_save_atm (atm, keyfile, group, dir, errlist);
}

/**
 * config_save_macros: Saves macros list.
 *
 **/
static void
config_save_macros (GKeyFile* keyfile, const gchar* dir, GList* list, GList** gerrors)
{
  ATM*          atm;
  GList*        it;
  gchar**       alist = NULL;
  gsize         alen;

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      atm = (ATM*) it->data;
      config_save_macro (atm, keyfile, dir, gerrors);
    }

  alen = atm_create_names_list (list, &alist);

  if (alen > 0)
    {
      g_key_file_set_string_list (keyfile, MUDCFG_MACROSES_GROUP, "List",
                                  (const gchar**const)alist, alen);
      g_strfreev (alist);
    }
  else
    {
      config_save_string (keyfile, MUDCFG_MACROSES_GROUP, "List", "");
    }
}

/**
 * config_save_macro: Saves given proxy.
 *
 **/
static void config_save_proxy (Proxy * proxy, GKeyFile * keyfile) {
	gchar group [128];

	g_assert (proxy);
	g_snprintf (group, 128, "Proxy:%s", proxy->name);
	config_save_string (keyfile, group, "Host", proxy->host);
	config_save_int (keyfile, group, "Port", proxy->port);
	config_save_bool (keyfile, group, "Default", proxy->deflt);
	config_save_string (keyfile, group, "User", proxy->user);
	config_save_string (keyfile, group, "Passwd", proxy->passwd);
}

/**
 * config_save_proxies: Saves list of proxies.
 *
 **/
static void config_save_proxies (GKeyFile * keyfile, GList * pl) {
	if (pl) {
		GList * it;
		int i = 0;
		int len = g_list_length (pl);
		char ** names = g_new0 (gchar *, len + 1);

		for (it = g_list_first (pl); it; it = g_list_next (it)) {
			config_save_proxy ((Proxy *) it->data, keyfile);
			names [i++] = g_strdup (((Proxy *) it->data)->name);
		}
		g_key_file_set_string_list (keyfile, MUDCFG_PROXIES_GROUP, "List", (const gchar**const) names, len);
		g_strfreev (names);
	} else {
		config_save_string (keyfile, MUDCFG_PROXIES_GROUP, "List", "");
	}
}

/**
 * config_save_aliases: Saves alias list.
 *
 **/
static void
config_save_aliases (GKeyFile* keyfile, const gchar* dir, GList* list, GList** gerrors)
{
  ATM*          atm;
  GList*        it;
  gchar**       alist = NULL;
  gsize         alen;

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      atm = (ATM*) it->data;
      config_save_alias (atm, keyfile, dir, gerrors);
    }

  alen = atm_create_names_list (list, &alist);

  if (alen > 0)
    {
      g_key_file_set_string_list (keyfile, MUDCFG_ALIASES_GROUP, "List",
                                  (const gchar**const)alist, alen);
      g_strfreev (alist);
    }
  else
    {
      config_save_string (keyfile, MUDCFG_ALIASES_GROUP, "List", "");
    }
}

/**
 * config_save_triggers: Saves triggers list.
 *
 **/
static void
config_save_triggers (GKeyFile* keyfile, const gchar* dir, GList* list, GList** gerrors)
{
  ATM*          atm;
  GList*        it;
  gchar**       alist = NULL;
  gsize         alen;

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      atm = (ATM*) it->data;
      config_save_trigger (atm, keyfile, dir, gerrors);
    }

  alen = atm_create_names_list (list, &alist);

  if (alen > 0)
    {
      g_key_file_set_string_list (keyfile, MUDCFG_TRIGGERS_GROUP, "List",
                                  (const gchar**const)alist, alen);
      g_strfreev (alist);
    }
  else
    {
      config_save_string (keyfile, MUDCFG_TRIGGERS_GROUP, "List", "");
    }
}

/**
 * config_save_modules_cfg: Saves modules list.
 *
 * @cfg:    A #Configuration.
 * @list:   Modules list.
 *
 **/
void
config_save_modules_cfg (GKeyFile* keyfile, GList* list)
{
  gchar** plist = NULL;
  gsize   plist_len;

  plist = module_create_names_list (list, &plist_len);

  if (plist_len > 0)
    {
      g_key_file_set_string_list (keyfile, MUDCFG_MODULES_GROUP, "List",
                                  (const gchar**const) plist, plist_len);

      g_strfreev (plist);
    }
  else
    {
      config_save_string (keyfile, MUDCFG_MODULES_GROUP, "List", "");
    }
}

/**
 * config_save_key_file: Workaround for GLib <= 2.6, in 2.8+ already
 *                       present funtion g_key_file_to_file.
 *
 * Return value: TRUE in case of successfull save,
 *               FALSE otherwise.
 **/
static gboolean
config_save_key_file (GKeyFile* file, const gchar* filename, GList** gerrors)
{
  GError*  error = NULL;
  gsize    len;
  gboolean ret;
  gchar*   out = g_key_file_to_data (file, &len, NULL);

  ret = g_file_set_contents (filename, out, len, &error);
  if (error != NULL)
    {
      add_gerror (gerrors, error);
    }

  g_free (out);

  return ret;
}

/**
 * configuration_save:
 *
 **/
gboolean
configuration_save (Configuration* cfg)
{
  gint    res;
  GError* error = NULL;
  gchar*  macrodir,
       *  aliasdir,
       *  triggerdir;
  char buf [64];

  g_assert (cfg);

//  if (! config_backup_old (cfg))
//    {
//      mdebug (DBG_CONFIG, 0,"Cannot rename files: %s -> %s.old\n", cfg->filename, cfg->filename);
//      g_error ("Cannot backup previous configuration file. Save declined.");
//      return;
//    }

  if (NULL == cfg->file)
    {
      cfg->file = g_key_file_new ();
      res = g_key_file_load_from_data (cfg->file, default_config_file,
                                       strlen (default_config_file),
                                       G_KEY_FILE_NONE, &error);

      if (! res)
        {
          mdebug (DBG_CONFIG, 0,"Default file: %s\n", default_config_file);
          g_critical ("Cannot parse default config file.");
          return FALSE;
        }
    }

  config_save_int    (cfg->file, MUDCFG_MAIN_GROUP, "config_version", cfg->cfg_ver);
  config_save_bool   (cfg->file, MUDCFG_MAIN_GROUP, "download", cfg->download);
  config_save_bool   (cfg->file, MUDCFG_MAIN_GROUP, "keepsent", cfg->keepsent);
  config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "entry_seperator", cfg->entry_seperator);
  config_save_int    (cfg->file, MUDCFG_MAIN_GROUP, "cmd_buffer_size", cfg->cmd_buf_size);
  config_save_bool   (cfg->file, MUDCFG_MAIN_GROUP, "cmd_autocompletion", cfg->cmd_autocompl);
  config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "help_browser", cfg->help_browser);
  g_snprintf (buf, 64, "%ld", (long int) cfg->wiz_vis_cols);
  config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "wiz_vis_cols", buf);
  config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "acct_user", cfg->acct_user);
  config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "acct_passwd", cfg->acct_passwd);
  if (((time_t)(-1)) != cfg->gl_last_upd) {
	g_snprintf (buf, 64, "%ld", (long int) cfg->gl_last_upd);
    config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "gl_last_upd", buf);
  }
  
#ifndef HAVE_WINDOWS
  config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "mp3cmd", cfg->mp3cmd);
  config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "wavcmd", cfg->wavcmd);
  config_save_string (cfg->file, MUDCFG_MAIN_GROUP, "midcmd", cfg->midcmd);
#endif

  config_save_string (cfg->file, MUDCFG_MACROSES_GROUP, "Dir", cfg->macrodir);
  config_save_string (cfg->file, MUDCFG_ALIASES_GROUP,  "Dir", cfg->aliasdir);
  config_save_string (cfg->file, MUDCFG_TRIGGERS_GROUP, "Dir", cfg->triggerdir);

  macrodir   = g_build_path (G_DIR_SEPARATOR_S, cfg->gamedir, cfg->macrodir, NULL);
  aliasdir   = g_build_path (G_DIR_SEPARATOR_S, cfg->gamedir, cfg->aliasdir, NULL);
  triggerdir = g_build_path (G_DIR_SEPARATOR_S, cfg->gamedir, cfg->triggerdir, NULL);

  utils_mkdir (macrodir);
  utils_mkdir (aliasdir);
  utils_mkdir (triggerdir);
    
  config_save_macros (cfg->file, macrodir, cfg->macros, &cfg->cfg_errors);
  config_save_aliases (cfg->file, aliasdir, cfg->aliases, &cfg->cfg_errors);
  config_save_triggers (cfg->file, triggerdir, cfg->triggers, &cfg->cfg_errors);
  config_save_modules_cfg (cfg->file, cfg->modules);
  config_save_proxies (cfg->file, cfg->proxies);

  g_free (macrodir);
  g_free (aliasdir);
  g_free (triggerdir);

  return config_save_key_file (cfg->file, cfg->filename, &cfg->cfg_errors);
}

/**
 * config_load_int: Reads the value associated with @key under @group into @result.
 *                  In the event the key cannot be found, FALSE is returned, error
 *                  is append to error list and result returned unmodified.
 *
 * @cfg:  A #GKeyFile.
 * @group:  A group name.
 * @key:  A key name.
 * @result:     Where place result.
 * @gerrors:    #GList for errors.
 *
 * Returns:   TRUE if success, FALSE otherwise.
 *
 **/
gboolean
config_load_int (GKeyFile* keyfile, const gchar* group, const gchar* key,
                        gint* result, GList** gerrors)
{
  GError*   error = NULL;
  gint      res;

  res = g_key_file_get_integer (keyfile, group, key, &error);

  if (error != NULL)
    {
      add_gerror (gerrors, error);
      res = FALSE;
    }
  else
    {
      *result = res;
      res = TRUE;
    }

  return res;
}

/**
 * config_load_bool: Reads the value associated with @key under @group into @result.
 *                   In the event the key cannot be found, FALSE is returned, error
 *                   is append to error list and result returned unmodified.
 *
 * @cfg:  A #GKeyFile.
 * @group:  A group name.
 * @key:  A key name.
 * @result:     Where place result (newly allocated string).
 * @gerrors:    #GList for errors.
 *
 * Returns:   TRUE if success, FALSE otherwise.
 *
 **/
gboolean
config_load_bool (GKeyFile* keyfile, const gchar* group, const gchar* key,
                        gboolean* result, GList** gerrors)
{
  GError*   error = NULL;
  gboolean  res;

  res = g_key_file_get_boolean (keyfile, group, key, &error);

  if (error != NULL)
    {
      add_gerror (gerrors, error);
      res = FALSE;
    }
  else
    {
      *result = res;
      res = TRUE;
    }

  return res;
}
/**
 * config_load_string: Reads the value associated with @key under @group into @result.
 *                     In the event the key cannot be found, FALSE is returned, error
 *                     is append to error list and result returned unmodified.
 *
 * @group:  A group name.
 * @key:  A key name.
 * @result:     Where place result (newly allocated string).
 * @gerrors:    #GList for errors.
 *
 * Returns:   TRUE if success, FALSE otherwise.
 *
 **/
gboolean
config_load_string (GKeyFile* keyfile, const gchar* group, const gchar* key,
                        gchar** result, GList** gerrors)
{
  GError*   error = NULL;
  gchar*    res = NULL;


  res = g_key_file_get_string (keyfile, group, key, &error);

  if (res == NULL)
    {
      if (gerrors) add_gerror (gerrors, error);
      *result = NULL;
    }
  else
    {
      *result = res;
    }

  return res != NULL;
}

/**
 * config_load_atm: loads atm with given name and type
 *
 **/
ATM * config_load_atm (GKeyFile * kf, const gchar * dir, const gchar * name, int type, GList ** errors) {
	char groupname [80];
	ATM * atm = NULL;
	char * caction = NULL, * raiser = NULL, * clang = NULL, * source = NULL, * text = NULL;
	int action;
	int lang = -1;
	int disabled;
	

	g_snprintf (
		groupname, 80,
		"%s:%s",
		(ATM_ALIAS == type) ? "Alias" : (ATM_TRIGGER == type ? "Trigger" : "Macro"),
		name
	);
	if (!config_load_string (kf, groupname, "Action", &caction, errors)) return NULL;
	action = config_get_action_id (caction);
	g_free (caction);
	if (!config_load_string (kf, groupname, "Source", &source, errors)) return NULL;
	if (!config_load_int (kf, groupname, "Disabled", &disabled, errors)) return NULL;
	if (!config_load_string (kf, groupname, "Raiser", &raiser, errors)) {
		g_free (source);
		return NULL;
	}
	if (ATM_ACTION_SCRIPT == action) {
		if (!config_load_string (kf, groupname, "Language", &clang, errors)) return NULL;
		else {
			char * fname = g_build_path (G_DIR_SEPARATOR_S, dir, source, NULL);
			GError * err = NULL;

			lang = config_get_lang_id (clang);
			g_free (clang);
			if (!g_file_get_contents (fname, &text, NULL, &err)) {
				* errors = g_list_append (*errors, err);
				g_free (source);
				return NULL;
			}
		}
	}
	atm = atm_new ();
	atm_init (atm, type, name, text, lang, source, raiser, action, disabled);
	g_free (source);
	g_free (raiser);
	if (text) g_free (text);
	return atm;
}

/**
 * config_load_atms: loads atm list of given type
 *
 **/
static void config_load_atms (
		Configuration * cfg,
		Session * ss,
		GKeyFile* keyfile,
		GList** dst,
		const gchar* dir,
		gchar ** list, gsize len,
		int type,
		GList** gerrors
	) {
	ATM*      atm;
	gsize     i;

	for (i = 0; i < len; i++) {
		atm = config_load_atm (keyfile, dir, list [i], type, gerrors);
		if (atm) {
			atm_set_masters (atm, cfg, ss);
			* dst = g_list_append (*dst, atm);
		}
	}
}

/**
 * config_load_macro: Loads macros configuration for prehistoric config file version.
 *
 * @keyfile: A #GKeyFile.
 * @dir:   Path to macros dir.
 * @name:  Name of macro.
 * @gerrors:    #GList for errors.
 *
 * Return value: Newly created ATM object, or NULL if errors has occured
 *               during load.
 **/
ATM*
config_load_macro (GKeyFile* keyfile, const gchar* dir, const gchar* name, GList** gerrors)
{
  gchar*    fullfilename = NULL;
  gchar     groupname[80];
  gchar*    filename = NULL,
       *    expr = NULL,
       *    priname = NULL,
       *    lang = NULL;
  gint      type;
  gboolean  internal;
  ATM*      atm = NULL;

  g_snprintf (groupname, 80, "Macro:%s", name);

  if (!config_load_string (keyfile, groupname, "File", &filename, gerrors))
    {
      return NULL;
    }

  //fullfilename = g_build_path(G_DIR_SEPARATOR_S, dir, filename, NULL);

  if (config_load_string (keyfile, groupname, "Raiser", &expr, gerrors)
          && config_load_bool (keyfile, groupname, "Internal", &internal, gerrors)
          && config_load_string (keyfile, groupname, "Name", &priname, gerrors)
          && config_load_string (keyfile, groupname, "Type", &lang, gerrors)
          )
    {
      // TODO: better error checking.
      type = strcmp (lang, "python") ? ATM_LANG_BASIC : ATM_LANG_PYTHON;

      atm = atm_new ();
		if (internal) {
			atm_init_macro (atm, priname, NULL, type, filename, expr, ATM_ACTION_SCRIPT);
		} else {
			// upgrade configuration
			atm_init_macro (atm, priname, NULL, type, NULL, expr, ATM_ACTION_SCRIPT);
		} 
    }

  g_free (fullfilename);
  g_free (expr);
  g_free (priname);
  g_free (lang);
  g_free (filename);

  return atm;
}

/**
 * config_load_alias: Loads alias configuration for prehistoric config file version.
 *
 * @keyfile: A #GKeyFile.
 * @dir:   Path to alias dir.
 * @name:  Internal name of alias.
 * @gerrors:    #GList for errors.
 *
 * Return value: Newly created ATM object, or NULL if errors has occured
 *               during load.
 **/
ATM*
config_load_alias (GKeyFile* keyfile, const gchar* dir, const gchar* name, GList** gerrors)
{
  gchar*    fullfilename = NULL;
  gchar     groupname[80];
  gchar*    filename = NULL,
       *    expr = NULL,
       *    priname = NULL,
       *    lang = NULL;
  gint      type;
  gboolean  internal;
  ATM*      atm = NULL;

  g_snprintf (groupname, 80, "Alias:%s", name);

  if (!config_load_string (keyfile, groupname, "File", &filename, gerrors))
    {
      return NULL;
    }
  //fullfilename = g_build_path(G_DIR_SEPARATOR_S, dir, filename, NULL);
  if (config_load_string (keyfile, groupname, "Raiser", &expr, gerrors)
          && config_load_bool (keyfile, groupname, "Internal", &internal, gerrors)
          && config_load_string (keyfile, groupname, "Name", &priname, gerrors)
          && config_load_string (keyfile, groupname, "Type", &lang, gerrors)
          )
    {
      // TODO: better error checking.
      type = strcmp (lang, "python") ? ATM_LANG_BASIC : ATM_LANG_PYTHON;

      atm = atm_new ();
		if (internal) {
			atm_init_alias (atm, priname, NULL, type, filename, expr, ATM_ACTION_SCRIPT);
		} else {
			// upgrade configuration
			atm_init_alias (atm, priname, NULL, type, NULL, expr, ATM_ACTION_SCRIPT);
		} 
    }

  g_free (fullfilename);
  g_free (expr);
  g_free (priname);
  g_free (lang);
  g_free (filename);

  return atm;
}

/**
 * config_load_trigger: Loads trigger configuration for prehistoric config file version.
 *
 * @keyfile: A #GKeyFile.
 * @dir:   Path to trigger dir.
 * @name:  Internal name of trigger.
 * @gerrors:    #GList for errors.
 *
 * Return value: Newly created ATM object, or NULL if errors has occured
 *               during load.
 **/
ATM*
config_load_trigger (GKeyFile* keyfile, const gchar* dir, const gchar* name, GList** gerrors)
{
  gchar*    fullfilename = NULL;
  gchar     groupname[80];
  gchar*    filename = NULL,
       *    expr = NULL,
       *    priname = NULL,
       *    lang = NULL;
  gint      type;
  gboolean  internal;
  ATM*      atm = NULL;

  g_snprintf (groupname, 80, "Trigger:%s", name);

  if (!config_load_string (keyfile, groupname, "File", &filename, gerrors))
    {
      return NULL;
    }

  //fullfilename = g_build_path(G_DIR_SEPARATOR_S, dir, filename, NULL);

  if (config_load_string (keyfile, groupname, "Raiser", &expr, gerrors)
          && config_load_bool (keyfile, groupname, "Internal", &internal, gerrors)
          && config_load_string (keyfile, groupname, "Name", &priname, gerrors)
          && config_load_string (keyfile, groupname, "Type", &lang, gerrors)
          )
    {
      // TODO: better error checking.
      type = strcmp (lang, "python") ? ATM_LANG_BASIC : ATM_LANG_PYTHON;

      atm = atm_new ();
		if (internal) {
			atm_init_trigger (atm, priname, NULL, type, filename, expr, ATM_ACTION_SCRIPT);
		} else {
			// upgrade configuration
			atm_init_trigger (atm, priname, NULL, type, NULL, expr, ATM_ACTION_SCRIPT);
		} 
    }

  g_free (fullfilename);
  g_free (expr);
  g_free (priname);
  g_free (lang);
  g_free (filename);

  return atm;
}
/**
 * Loads proxy
 **/
static Proxy * config_load_proxy (GKeyFile * keyfile, const gchar * name, GList ** gerrors) {
	gchar group [128];
	Proxy * p = g_new0 (Proxy, 1);
	gint port;

	g_snprintf (group, 128, "Proxy:%s", name);
	p->name = g_strdup (name);
	if (config_load_string (keyfile, group, "Host", &p->host, gerrors)
		&& config_load_int (keyfile, group, "Port", &port, gerrors)
		&& config_load_bool (keyfile, group, "Default", &p->deflt, gerrors)
		&& config_load_string (keyfile, group, "User", &p->user, gerrors)
		&& config_load_string (keyfile, group, "Passwd", &p->passwd, gerrors)
	) {
		p->port = port;
	} else {
		g_free (p);
		p = NULL;
	}
	return p;
}
/**
 * Loads proxy list
 **/
static void config_load_proxies (GKeyFile * keyfile, GList ** dst, gchar ** list, gsize len, GList ** gerrors) {
	int i;
	Proxy * p;

	for (i = 0; i < len; i++) {
		p = config_load_proxy (keyfile, list [i], gerrors);
		if (p) * dst = g_list_append (* dst, p);
	}
}

/**
 * config_load_macroses: Loads macroses specified in array. All errors during load
 *                       stored in common error list.
 *
 * @cfg:     A #Configuration.
 * @ss:      A #Session.
 * @keyfile: A #GKeyFile.
 * @dir:    Path to common dir.
 * @list:   Macros array.
 * @len:    Length of array.
 * @gerrors:    #GList for errors.
 * 
 **/
static void
config_load_macroses (Configuration* cfg, Session* ss,
                                GKeyFile* keyfile, GList** dst, const gchar* dir,
                                gchar** list, gsize len, GList** gerrors)
{
  ATM*      atm;
  gsize     i;

  for (i = 0; i < len; i++)
    {
      atm = config_load_macro (keyfile, dir, list[i], gerrors);
      if (atm)
        {
          atm_set_masters (atm, cfg, ss);
          *dst = g_list_append (*dst, atm);
        }
    }
}

/**
 * config_load_aliases: Loads aliases specified in array. All errors during load
 *                      stored in common error list.
 *
 * @cfg:     A #Configuration.
 * @ss:      A #Session.
 * @keyfile: A #GKeyFile.
 * @dir:    Path to common dir.
 * @list:   Alias array.
 * @len:    Length of array.
 * @gerrors:    #GList for errors.
 * 
 **/
static void
config_load_aliases(Configuration* cfg, Session* ss,
                                GKeyFile* keyfile, GList** dst, const gchar* dir,
                                gchar** list, gsize len, GList** gerrors)
{
  ATM*      atm;
  gsize     i;

  for (i = 0; i < len; i++)
    {
      atm = config_load_alias (keyfile, dir, list[i], gerrors);
      if (atm)
        {
          atm_set_masters (atm, cfg, ss);
          *dst = g_list_append (*dst, atm);
        }
    }
}

/**
 * config_load_triggers: Loads triggers specified in array. All errors during load
 *                       stored in common error list.
 *
 * @cfg:     A #Configuration.
 * @ss:      A #Session.
 * @keyfile: A #GKeyFile.
 * @dir:    Path to common dir.
 * @list:   Trigger array.
 * @len:    Length of array.
 * @gerrors:    #GList for errors.
 * 
 **/
static void
config_load_triggers (Configuration* cfg, Session* ss,
                                GKeyFile* keyfile, GList** dst, const gchar* dir,
                                gchar** list, gsize len, GList** gerrors)
{
  ATM*      atm;
  gsize     i;

  for (i = 0; i < len; i++)
    {
      atm = config_load_trigger (keyfile, dir, list[i], gerrors);
      if (atm)
        {
          atm_set_masters (atm, cfg, ss);
          *dst = g_list_append (*dst, atm);
        }
    }
}

/**
 * config_load_modules: Loads modules specified in array. All errors during load
 *                      stored in common error list.
 *                      TODO: Reading from specified directory.
 *
 * @keyfile: A #GKeyFile.
 * @list:   Trigger array.
 * @len:    Length of array.
 * 
 **/
static void
config_load_modules (GKeyFile* keyfile, GList** dst, gchar** list, gsize len)
{
  gsize         i;
  MODULE_ENTRY* pe;
//
//  for (i = 0; i < len; i++)
//    {
//      module_load (module_get_by_name (*dst, list[i]));
//    }

  // Mark modules that should be loaded
  for (i = 0; i < len; i++)
    {
      pe = module_get_by_name (*dst, list[i]);

      pe->used = (pe != NULL);
    }
}

gboolean
configuration_load_old(CONFIGURATION * config, const gchar* cfgfile);

/**
 * configuration_load: Loads configuration from file.
 *
 * @cfg:    A #Configuration.
 *
 **/
gboolean
configuration_load (Configuration* cfg, const gchar* cfgfile)
{
  GError*   error = NULL;
  gint      res;
  gchar**   macroses,
       **   triggers,
       **   aliases,
       **   modules,
       **   proxies;
  gchar*    macropath,
       *    aliaspath,
       *    triggerpath;
  gsize     len, alen, mlen, tlen;
  gchar*    s;

  if (cfg->upgrade)
    {
      mdebug (DBG_CONFIG, 0,"Load old-style configuration.\n");
      return configuration_load_old (cfg, cfgfile);
    }

  if (NULL == cfg->file)
    {
      cfg->file = g_key_file_new ();
    }

  if (g_file_test (cfgfile, G_FILE_TEST_EXISTS))
    {
      res = g_key_file_load_from_file(cfg->file, cfgfile, G_KEY_FILE_NONE, &error);

      if (!res)
        {
          config_add_gerror (cfg, error);
          return FALSE;
        }
    }
  else
    {
		// perform initial setup with predefined values
#ifdef HAVE_WINDOWS
		char * hb = MUDCFG_WIN_DEFAULT_BROWSER;
#else
		char * hb = MUDCFG_DEFAULT_BROWSER;
#endif
	    cfg->proxies = g_list_append (cfg->proxies, proxy_get_none ());
    	cfg->proxies = g_list_append (cfg->proxies, proxy_get_mudmagic ());
		cfg->help_browser = g_strdup (hb);
		cfg->cfg_ver = 1;
		
      return TRUE; // First run
    }

# if 0  
  if (! g_key_file_has_group (cfg->file, MUDCFG_MAIN_GROUP))
    {
      g_key_file_free (cfg->file);
      cfg->file = NULL;
      configuration_load_old (cfg, cfgfile);
      return;
    }
# endif
	if (g_key_file_has_key (cfg->file, MUDCFG_MAIN_GROUP, "config_version", NULL)) {
		config_load_int (cfg->file, MUDCFG_MAIN_GROUP, "config_version", &cfg->cfg_ver, &cfg->cfg_errors);
	} else cfg->cfg_ver = 0; // prehistoric config file
  config_load_bool   (cfg->file, MUDCFG_MAIN_GROUP, "download",
                                        &cfg->download, &cfg->cfg_errors);
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "entry_seperator",
                                        &cfg->entry_seperator, &cfg->cfg_errors);
  config_load_bool   (cfg->file, MUDCFG_MAIN_GROUP, "keepsent",
                                        &cfg->keepsent, &cfg->cfg_errors);
  config_load_int    (cfg->file, MUDCFG_MAIN_GROUP, "cmd_buffer_size",
                                        &cfg->cmd_buf_size, &cfg->cfg_errors);
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "help_browser",
                                        &cfg->help_browser, &cfg->cfg_errors);
  if (!cfg->help_browser) {
#ifdef HAVE_WINDOWS
		char * hb = MUDCFG_WIN_DEFAULT_BROWSER;
#else
		char * hb = MUDCFG_DEFAULT_BROWSER;
#endif
		cfg->help_browser = g_strdup (hb);
  }
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "wiz_vis_cols",
                                        &s, NULL);
  if (s) {
	cfg->wiz_vis_cols = atol (s);
	g_free (s);
  } else {
	// first six colums are visible by default
	cfg->wiz_vis_cols = 0x0F; 
  }
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "acct_user",
                                        &cfg->acct_user, &cfg->cfg_errors);
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "acct_passwd",
                                        &cfg->acct_passwd, &cfg->cfg_errors);
  if (!cfg->acct_user) cfg->acct_user = g_strdup ("");
  if (!cfg->acct_passwd) cfg->acct_passwd = g_strdup ("");
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "gl_last_upd",
                                        &s, &cfg->cfg_errors);
  if (s) {
        cfg->gl_last_upd = (time_t) strtol (s, NULL, 10);
        g_free (s);
  } else {
        cfg->gl_last_upd = (time_t) (-1);
  }
  
  if (cfg->cmd_buf_size < 0)
    {
        cfg->cmd_buf_size = DEFAULT_CMD_BUF_SIZE;
    }

  config_load_bool   (cfg->file, MUDCFG_MAIN_GROUP, "cmd_autocompletion",
                                        &cfg->cmd_autocompl, &cfg->cfg_errors);
# ifndef HAVE_WINDOWS
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "mp3cmd",
                                        &cfg->mp3cmd, &cfg->cfg_errors);
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "wavcmd",
                                        &cfg->wavcmd, &cfg->cfg_errors);
  config_load_string (cfg->file, MUDCFG_MAIN_GROUP, "midcmd",
                                        &cfg->midcmd, &cfg->cfg_errors);
# endif

  config_load_string (cfg->file, MUDCFG_ALIASES_GROUP, "Dir",
                                        &cfg->aliasdir, &cfg->cfg_errors);
  config_load_string (cfg->file, MUDCFG_MACROSES_GROUP, "Dir",
                                        &cfg->macrodir, &cfg->cfg_errors);
  config_load_string (cfg->file, MUDCFG_TRIGGERS_GROUP, "Dir",
                                        &cfg->triggerdir, &cfg->cfg_errors);
  macropath   = g_build_path (G_DIR_SEPARATOR_S, cfg->gamedir, cfg->macrodir, NULL);
  aliaspath   = g_build_path (G_DIR_SEPARATOR_S, cfg->gamedir, cfg->aliasdir, NULL);
  triggerpath = g_build_path (G_DIR_SEPARATOR_S, cfg->gamedir, cfg->triggerdir, NULL);

  macroses = g_key_file_get_string_list (cfg->file, MUDCFG_MACROSES_GROUP, "List", &mlen, &error);
  aliases  = g_key_file_get_string_list (cfg->file, MUDCFG_ALIASES_GROUP,  "List", &alen, &error);
  triggers = g_key_file_get_string_list (cfg->file, MUDCFG_TRIGGERS_GROUP, "List", &tlen, &error);

	if (cfg->cfg_ver) {
		// load new foramt config file
		if (macroses) config_load_atms (cfg, NULL, cfg->file, &cfg->macros, macropath, macroses, mlen, ATM_MACRO, &cfg->cfg_errors);
		if (aliases) config_load_atms (cfg, NULL, cfg->file, &cfg->aliases, aliaspath, aliases, alen, ATM_ALIAS, &cfg->cfg_errors);
		if (triggers) config_load_atms (cfg, NULL, cfg->file, &cfg->triggers, triggerpath, triggers, tlen, ATM_TRIGGER, &cfg->cfg_errors);
	} else {
	// load old style format and upgrade it
	cfg->cfg_ver = 1;
  // Load macroses.

  if (macroses != NULL)
      config_load_macroses (cfg, NULL, cfg->file, &cfg->macros,
                                macropath, macroses, mlen, &cfg->cfg_errors);
  else
    {
      config_add_gerror (cfg, error);
      error = NULL;
    }

  // Loading aliases.
  
  if (aliases != NULL)
      config_load_aliases (cfg, NULL, cfg->file, &cfg->aliases,
                                aliaspath, aliases, alen, &cfg->cfg_errors);
  else
    {
      config_add_gerror (cfg, error);
      error = NULL;
    }

  // Load triggers.
  
  if (triggers != NULL)
      config_load_triggers (cfg, NULL, cfg->file, &cfg->triggers,
                                triggerpath, triggers, tlen, &cfg->cfg_errors);
  else
    {
      config_add_gerror (cfg, error);
      error = NULL;
    }
  }
  // Load modules.
  modules = g_key_file_get_string_list (cfg->file, MUDCFG_MODULES_GROUP, "List", &len, &error);
  
  if (modules != NULL)
      config_load_modules (cfg->file, &cfg->modules, modules, len);
  else
    {
      config_add_gerror (cfg, error);
      error = NULL;
    }

  // Load proxies
  proxies = g_key_file_get_string_list (cfg->file, MUDCFG_PROXIES_GROUP, "List", &len, &error);
  
  if (proxies) config_load_proxies (cfg->file, &cfg->proxies, proxies, len, &cfg->cfg_errors);
  else {
    config_add_gerror (cfg, error);
    error = NULL;
  }
  if (!g_list_length (g_list_first (cfg->proxies))) {
    cfg->proxies = g_list_append (cfg->proxies, proxy_get_none ());
    cfg->proxies = g_list_append (cfg->proxies, proxy_get_mudmagic ());
  }
  
  // Work has been done.
  g_free (macropath);
  g_free (aliaspath);
  g_free (triggerpath);
  g_strfreev (macroses);
  g_strfreev (aliases);
  g_strfreev (triggers);
  g_strfreev (modules);
  g_strfreev (proxies);

  return TRUE;
}


gboolean
configuration_load_old(CONFIGURATION * config, const gchar* cfgfile)
{
  gchar *k = NULL;  // key
  gchar *e = NULL;  // name/expr
  gchar *file = NULL; // file
  int cmds;   // number of commands
  char **commands;        // commands
  FILE *f;
  if (!config)
    return FALSE;
  f = fopen(cfgfile, "r");
  if (!f) {
    g_warning("couldn't open config file\n");
    return FALSE;
  }
  while (utils_get_next(f, &k, &e, &cmds, &commands, &file)) {
    if (g_str_has_prefix(k, "trigger")) {
      ATM *atm = config_migrate_atm(ATM_TRIGGER, e, cmds, commands, file, NULL);
      config->triggers =
          g_list_append(config->triggers, (gpointer) atm);
    }
    if (g_str_has_prefix(k, "alias")) {
      ATM *atm = config_migrate_atm(ATM_ALIAS, e, cmds, commands, file, NULL);
      config->aliases =
          g_list_append(config->aliases, (gpointer) atm);
    }
    if (g_str_has_prefix(k, "macro")) {
      ATM *atm = config_migrate_atm(ATM_MACRO, e, cmds, commands, file, NULL);
      config->macros =
          g_list_append(config->macros, (gpointer) atm);
    }
    if (g_str_has_prefix(k, "module")) {
      module_load(module_get_by_name
            (config->modules, e));
    }
    if (!strcmp(k, "download")) {
      config->download = strcmp(e, "on") ? FALSE : TRUE;
    }

    if (!strcmp(k, "entry_seperator")) {
      config->entry_seperator = g_strdup(e);
    }

    if (!strcmp(k, "keepsent")) {
      config->keepsent = strcmp(e, "on") ? FALSE : TRUE;
    }
    if (!strcmp(k, "cmd_buffer_size")) {
      config->cmd_buf_size = (gint)utils_atoi (e, -1);

      if (config->cmd_buf_size < 0) {
          config->cmd_buf_size = DEFAULT_CMD_BUF_SIZE;
      }
    }
    if (!strcmp(k, "cmd_autocompletion")) {
      config->cmd_autocompl = strcmp(e, "on") ? FALSE : TRUE;
    }
#ifndef HAVE_WINDOWS
    if (!strcmp(k, "mp3cmd")) {
      config->mp3cmd = g_strdup(e);
    }
    if (!strcmp(k, "wavcmd")) {
      config->wavcmd = g_strdup(e);
    }
    if (!strcmp(k, "midcmd")) {
      config->midcmd = g_strdup(e);
    }
#endif
    g_free(k);
    k = NULL;
    g_free(file);
    file = NULL;
    g_free(e);
    e = NULL;
    if (commands) {
      int i;
      for (i = 0; i < cmds; ++i)
        g_free(commands[i]);
      g_free(commands);
    }
  }
  fclose(f);

        return TRUE;
}

gchar *session_get_free_slot(CONFIGURATION * config)
{
  gchar *ret;
  gchar *t;
  int i;
  // find a free slot 
  for (i = 0; i < 10000; i++) {
    t = g_strdup_printf("slot%04d", i);
    ret =
        g_build_path(G_DIR_SEPARATOR_S, config->savedir, t,
         NULL);
    g_free(t);
    if (g_file_test(ret, G_FILE_TEST_IS_DIR)) {
      g_free(ret);
    } else {
      break;
    }
  }
  if (!utils_mkdir(ret)) {
    g_free(ret);
    return NULL;
  }
  return ret;
}

SESSION_STATE *session_new()
{
  SESSION_STATE *ret;
  ret = g_new0(SESSION_STATE, 1);

        ret->file = NULL;
        ret->upgrade = FALSE;

  ret->cfg_ver = 1;
  ret->telnet = telnet_new();
  ret->telnet->link = ret;  // let telnet know which session belongs
  ret->pconn = NULL;
  // to

  ret->single_line = TRUE;
  ret->out_sep_pos = 0;
  ret->font = g_strdup("Monospace 12");
  ret->bg_color = g_strdup("black");
  ret->fg_color = g_strdup("white");
  ret->ufg_color = g_strdup("yellow");
  initialize_ansi(ret);
  ret->local_echo = TRUE;
  ret->input_event_id = -1;

  ret->variables = varlist_new (ret);
 
  ret->svlist = svlist_new (ret);
  ret->gaugelist = gaugelist_new (ret);
  ret->windowlist = owindowlist_new (ret);

  ret->extra = g_hash_table_new(g_str_hash, g_str_equal);

  ret->cmdline.cmd_buf = g_queue_new();
//  g_queue_push_tail(ret->cmdline.cmd_buf, "");
//  ret->cmdline.cmd_current = g_queue_peek_tail_link (ret->cmdline.cmd_buf);
  ret->cmdline.cmd_store = TRUE;
        ret->gerrors = NULL;
  ret->proxy = NULL;

  return ret;
}

void session_delete(SESSION_STATE * session)
{

  // save the session before deleting
  // need to have it here, so that variables are saved correctly
  session_save(session);

  // delete the session
  if (session->slot)
    g_free(session->slot);
  if (session->game_name)
    g_free(session->game_name);
  if (session->game_host)
    g_free(session->game_host);

  if (session->font)
    g_free(session->font);
  if (session->bg_color)
    g_free(session->bg_color);
  if (session->fg_color)
    g_free(session->fg_color);
  if (session->ufg_color)
    g_free(session->ufg_color);
  if (session->log_file)
    fclose(session->log_file);
  if (session->triggers)
    atm_list_clear(&session->triggers);
  if (session->aliases)
    atm_list_clear(&session->aliases);
  if (session->macros)
    atm_list_clear(&session->macros);
  if (session->variables)
    varlist_destroy(session->variables);
  if (session->svlist)
    svlist_destroy (session->svlist);
  if (session->gaugelist)
    gaugelist_destroy (session->gaugelist);
  if (session->windowlist)
    owindowlist_destroy (session->windowlist);
  if (session->extra)
    g_hash_table_destroy(session->extra);

  if (session->telnet)
    telnet_free(session->telnet);

  if (session->cmdline.cmd_buf)
  {
    cmd_entry_history_clear (&session->cmdline, 0);
    g_queue_free (session->cmdline.cmd_buf);
  }
  g_list_free (session->timers);
        utils_clear_gerrors (&session->gerrors);
        g_key_file_free (session->file);

  if (session->proxy) g_free (session->proxy);
  if (session->pconn) proxy_connection_close (session->pconn);
  g_free(session);
}


void session_save(SESSION_STATE* ss)
{
  gint    res;
  FILE*   f;
  GError* error;
  gchar*  macrodir,
       *  aliasdir,
       *  triggerdir,
       *  filename;

  Configuration* cfg = get_configuration ();

  g_assert (ss);
  g_assert (cfg);

  if (ss->slot == NULL)
    {
      g_warning("no slot allocated; save not possible.");
      return;
    }

  if (NULL == ss->file)
    {
      ss->file = g_key_file_new ();
      res = g_key_file_load_from_data (ss->file, default_slot_config_file,
                                       strlen (default_slot_config_file),
                                       G_KEY_FILE_NONE, &error);

      if (! res)
        {
          mdebug (DBG_CONFIG, 0,"Default file: %s\n", default_config_file);
          g_critical ("Cannot parse default config file.");
          return;
        }
    }
  config_save_int    (ss->file, MUDCFG_SLOT_MAIN_GROUP, "config_version", ss->cfg_ver);
  config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "name", ss->name);
  config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "game_name", ss->game_name);
  config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "game_host", ss->game_host);
  config_save_int    (ss->file, MUDCFG_SLOT_MAIN_GROUP, "game_port", ss->game_port);
  config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "font", ss->font);
  config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "bg_color", ss->bg_color);
  config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "fg_color", ss->fg_color);
  config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "ufg_color", ss->ufg_color);
  config_save_bool   (ss->file, MUDCFG_SLOT_MAIN_GROUP, "single_line", ss->single_line);
  config_save_bool   (ss->file, MUDCFG_SLOT_MAIN_GROUP, "local_echo", ss->local_echo);
  
  config_save_bool   (ss->file, MUDCFG_SLOT_MAIN_GROUP, "logging", ss->logging);
  config_save_bool   (ss->file, MUDCFG_SLOT_MAIN_GROUP, "sound", ss->sound);
  if (ss->proxy) config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "proxy", ss->proxy);

  // save scripts
  macrodir   = g_build_path (G_DIR_SEPARATOR_S, ss->slot, cfg->macrodir, NULL);
  aliasdir   = g_build_path (G_DIR_SEPARATOR_S, ss->slot, cfg->aliasdir, NULL);
  triggerdir = g_build_path (G_DIR_SEPARATOR_S, ss->slot, cfg->triggerdir, NULL);

  utils_mkdir (macrodir);
  utils_mkdir (aliasdir);
  utils_mkdir (triggerdir);

  config_save_macros   (ss->file, macrodir, ss->macros, &ss->gerrors);
  config_save_aliases  (ss->file, aliasdir, ss->aliases, &ss->gerrors);
  config_save_triggers (ss->file, triggerdir, ss->triggers, &ss->gerrors);

  g_free (macrodir);
  g_free (aliasdir);
  g_free (triggerdir);

  // saving delayed commands
	if (ss->timers) {
		GList * it;
		int i = 0;
		int len = g_list_length (ss->timers);
		char ** names = g_new0 (gchar *, len + 1);
		gchar group [128];

		for (it = g_list_first (ss->timers); it; it = g_list_next (it)) {
			delayed_cmd * c = (delayed_cmd *) (it->data);
			if (c->repeat && !c->stop) {
				g_snprintf (group, 128, "DelayedCommand:cmd_%i", i + 1);
				config_save_string (ss->file, group, "Command", c->command);
				config_save_int (ss->file, group, "Interval", c->interval);
				g_snprintf (group, 128, "cmd_%i", i + 1);
				names [i++] = g_strdup (group);
			} else len--;
		}
		g_key_file_set_string_list (ss->file, MUDCFG_SLOT_MAIN_GROUP, "DelayedCommands", (const gchar**const) names, len);
		g_strfreev (names);
	} else {
		config_save_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "DelayedCommands", "");
	}

  filename = g_build_path(G_DIR_SEPARATOR_S, ss->slot, MUDCFG_CONFIG_SLOT_FILE, NULL);

  config_save_key_file (ss->file, filename, &ss->gerrors);
  g_free (filename);

  // saving variables
  filename = g_build_path(G_DIR_SEPARATOR_S, ss->slot, MUDCFG_SLOT_VARIABLES_FILE, NULL);
  
  f = fopen(filename, "w+");
  if (!f)
    {
      g_warning("Couldn't save variables to file '%s'.", filename);
    }
  else
    {
      varlist_save(ss->variables, f);
      fclose(f);
    }
  g_free(filename);

  // saving status vars
  filename = g_build_path(G_DIR_SEPARATOR_S, ss->slot, MUDCFG_SLOT_STATUSVARS_FILE, NULL);
  
  f = fopen(filename, "w+");
  if (!f)
  {
    g_warning("Couldn't save status variables to file '%s'.", filename);
  }
  else
  {
    svlist_save(ss->svlist, f);
    fclose(f);
  }
  g_free(filename);

  // saving gauges
  filename = g_build_path(G_DIR_SEPARATOR_S, ss->slot, MUDCFG_SLOT_GAUGES_FILE, NULL);
  
  f = fopen(filename, "w+");
  if (!f)
  {
    g_warning("Couldn't save gauges to file '%s'.", filename);
  }
  else
  {
    gaugelist_save(ss->gaugelist, f);
    fclose(f);
  }
  g_free(filename);

  // saving output windows
  filename = g_build_path(G_DIR_SEPARATOR_S, ss->slot, MUDCFG_SLOT_OWIN_FILE, NULL);
  
  f = fopen(filename, "w+");
  if (!f)
  {
    g_warning("Couldn't save output windows to file '%s'.", filename);
  }
  else
  {
    owindowlist_save(ss->windowlist, f);
    fclose(f);
  }
  g_free(filename);

  // saving history
  filename = g_build_path(G_DIR_SEPARATOR_S, ss->slot, MUDCFG_SLOT_HISTORY_FILE, NULL);
  f = fopen(filename, "w+");
  if (!f)
    {
      g_warning("Couldn't save command history to file '%s'\n", filename);
    }
  else
    {
      cmd_entry_history_save(&ss->cmdline, f);
      fclose(f);
    }
  g_free(filename);
}

/**
 * config_check_new_version_session: Checks whether new style config should be loads.
 *
 * @slotpath: Path to slot dir.
 *
 * Return value: TRUE if new style configuration should be used,
 *               FALSE otherwise.
 **/
static gboolean
config_check_new_version_session (const gchar* slotpath)
{
  gchar*   config;
  gboolean ret = TRUE;
  
  config  = g_build_path (G_DIR_SEPARATOR_S, slotpath, MUDCFG_CONFIG_SLOT_FILE, NULL);
  
  // Checking for new style config (=> 1.9 version)
  if (! g_file_test (config, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    {
       gchar* config_old;

       mdebug (DBG_CONFIG, 0,"New style slot cfg '%s' doesn't exist\n", config);
       
       config_old = g_build_path (G_DIR_SEPARATOR_S, slotpath, MUDCFG_CONFIG_SLOT_OLD_FILE, NULL);

       if (g_file_test (config_old, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
         {
           ret = FALSE; 
         }
       mdebug (DBG_CONFIG, 0,"Checking %s: %s\n", config_old, ret ? "NOT FOUND" : "FOUND" );
       //If any config file not found, will use new scheme
       g_free (config_old);
    }

  g_free (config);
  return ret;
}

static gboolean
session_load_old (SESSION_STATE * ss, const gchar * slot);


gboolean
session_load(SESSION_STATE* ss, const gchar* slot)
{
  gint      res;
  GError*   error = NULL;
  gchar*    filename;
  FILE*     f;
  gchar**   macroses,
       **   triggers,
       **   aliases,
       **   commands;
  gchar*    macropath,
       *    aliaspath,
       *    triggerpath;
  gsize     len, alen, mlen, tlen;

  Configuration* cfg = get_configuration ();
  g_assert (cfg);

  utils_clear_gerrors (&ss->gerrors);

  ss->upgrade = ! config_check_new_version_session (slot);

  if (ss->upgrade)
    {
      mdebug (DBG_CONFIG, 0,"Detected old-style configuration for slot %s.\n", slot);
      return session_load_old (ss, slot);
    }

  if (NULL == ss->file)
    {
      ss->file = g_key_file_new ();
    }

  filename = g_build_path(G_DIR_SEPARATOR_S, slot, MUDCFG_CONFIG_SLOT_FILE, NULL);

  res = g_key_file_load_from_file(ss->file, filename, G_KEY_FILE_NONE, &error);
  g_free (filename);

  if (!res)
    {
      add_gerror (&ss->gerrors, error);
      return FALSE;
    }

	if (g_key_file_has_key (ss->file, MUDCFG_SLOT_MAIN_GROUP, "config_version", NULL)) {
		config_load_int (ss->file, MUDCFG_SLOT_MAIN_GROUP, "config_version", &ss->cfg_ver, &ss->gerrors);
	} else ss->cfg_ver = 0; // prehistoric config file

  config_load_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "name",
                        &ss->name, &ss->gerrors);
  config_load_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "game_name",
                        &ss->game_name, &ss->gerrors);
  config_load_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "game_host",
                        &ss->game_host, &ss->gerrors);
  config_load_int    (ss->file, MUDCFG_SLOT_MAIN_GROUP, "game_port",
                        &ss->game_port, &ss->gerrors);
  config_load_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "font",
                        &ss->font, &ss->gerrors);
  config_load_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "bg_color",
                        &ss->bg_color, &ss->gerrors);
  config_load_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "fg_color",
                        &ss->fg_color, &ss->gerrors);
  config_load_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "ufg_color",
                        &ss->ufg_color, &ss->gerrors);
  config_load_bool   (ss->file, MUDCFG_SLOT_MAIN_GROUP, "single_line",
                        &ss->single_line, &ss->gerrors);
  config_load_bool   (ss->file, MUDCFG_SLOT_MAIN_GROUP, "local_echo",
                        &ss->local_echo, &ss->gerrors);
  config_load_bool   (ss->file, MUDCFG_SLOT_MAIN_GROUP, "logging",
                        &ss->logging, &ss->gerrors);
  config_load_bool (ss->file, MUDCFG_SLOT_MAIN_GROUP, "sound",
                        &ss->sound, &ss->gerrors);
  config_load_string (ss->file, MUDCFG_SLOT_MAIN_GROUP, "proxy",
                        &ss->proxy, &ss->gerrors);
  if (!ss->proxy) ss->proxy = g_strdup ("Default");
  macropath   = g_build_path (G_DIR_SEPARATOR_S, ss->slot, cfg->macrodir, NULL);
  aliaspath   = g_build_path (G_DIR_SEPARATOR_S, ss->slot, cfg->aliasdir, NULL);
  triggerpath = g_build_path (G_DIR_SEPARATOR_S, ss->slot, cfg->triggerdir, NULL);

  macroses = g_key_file_get_string_list (ss->file, MUDCFG_SLOT_MACROSES_GROUP, "List", &mlen, &error);
  aliases  = g_key_file_get_string_list (ss->file, MUDCFG_SLOT_ALIASES_GROUP, "List", &alen, &error);
  triggers = g_key_file_get_string_list (ss->file, MUDCFG_SLOT_TRIGGERS_GROUP, "List", &tlen, &error);

  if (ss->cfg_ver) {
		// load new foramt config file
		if (macroses) config_load_atms (cfg, ss, ss->file, &ss->macros, macropath, macroses, mlen, ATM_MACRO, &ss->gerrors);
		if (aliases) config_load_atms (cfg, ss, ss->file, &ss->aliases, aliaspath, aliases, alen, ATM_ALIAS, &ss->gerrors);
		if (triggers) config_load_atms (cfg, ss, ss->file, &ss->triggers, triggerpath, triggers, tlen, ATM_TRIGGER, &ss->gerrors);
  } else {
	// load prehistoric style actions
	ss->cfg_ver = 1;
  // Load macroses.
  if (macroses != NULL)
      config_load_macroses (cfg, ss, ss->file, &ss->macros, macropath, macroses, mlen, &ss->gerrors);
  else
    {
      add_gerror (&ss->gerrors, error);
      error = NULL;
    }

  // Loading aliases.
  if (aliases != NULL)
      config_load_aliases (cfg, ss, ss->file, &ss->aliases, aliaspath, aliases, alen, &ss->gerrors);
  else
    {
      add_gerror (&ss->gerrors, error);
      error = NULL;
    }

  // Load triggers.
  if (triggers != NULL)
      config_load_triggers (cfg, ss, ss->file, &ss->triggers, triggerpath, triggers, tlen, &ss->gerrors);
  else
    {
      add_gerror (&ss->gerrors, error);
      error = NULL;
    }
  }
  // Work has been done.
  g_free (macropath);
  g_free (aliaspath);
  g_free (triggerpath);
  g_strfreev (macroses);
  g_strfreev (aliases);
  g_strfreev (triggers);

  // variables
  filename = g_build_path(G_DIR_SEPARATOR_S, slot, MUDCFG_SLOT_VARIABLES_FILE, NULL);
  f = fopen(filename, "r");

  if (!f)
    {
      g_warning ("Couldn't open variable list (%s).", filename);
    }
  else
    {
      varlist_load(ss->variables, f);
      fclose(f);
    }
  g_free(filename);

  // status vars
  filename = g_build_path(G_DIR_SEPARATOR_S, slot, MUDCFG_SLOT_STATUSVARS_FILE, NULL);
  f = fopen(filename, "r");

  if (!f)
  {
    g_warning ("Couldn't open status variable list (%s).", filename);
  }
  else
  {
    svlist_load(ss->svlist, f);
    fclose(f);
  }
  g_free(filename);

  // gauges
  filename = g_build_path(G_DIR_SEPARATOR_S, slot, MUDCFG_SLOT_GAUGES_FILE, NULL);
  f = fopen(filename, "r");

  if (!f)
  {
    g_warning ("Couldn't open gauge list (%s).", filename);
  }
  else
  {
    gaugelist_load(ss->gaugelist, f);
    fclose(f);
  }
  g_free(filename);

  // output windows
  filename = g_build_path(G_DIR_SEPARATOR_S, slot, MUDCFG_SLOT_OWIN_FILE, NULL);
  f = fopen(filename, "r");

  if (!f)
  {
    g_warning ("Couldn't open window list (%s).", filename);
  }
  else
  {
    owindowlist_load(ss->windowlist, f);
    fclose(f);
  }
  g_free(filename);


  // command history
  filename = g_build_path(G_DIR_SEPARATOR_S, slot, MUDCFG_SLOT_HISTORY_FILE, NULL);
  f = fopen(filename, "r");

  if (f)
    {
      cmd_entry_history_load (&ss->cmdline, f);
      fclose(f);
    }
  else
    {
      g_warning ("Couldn't open history file (%s).", filename);
    }
  g_free(filename);
	// delayed commands
  ss->timers = NULL;
  commands = g_key_file_get_string_list (ss->file, MUDCFG_SLOT_MAIN_GROUP, "DelayedCommands", &len, &error);
  
  if (commands) {
	int i;
	delayed_cmd * c;
	gchar group [128];

	for (i = 0; i < len; i++) {
		c = g_new0 (delayed_cmd, 1);
		g_snprintf (group, 128, "DelayedCommand:%s", commands [i]);
		if (config_load_string (ss->file, group, "Command", &c->command, &ss->gerrors)
			&& config_load_int (ss->file, group, "Interval", &c->interval, &ss->gerrors)
		) {
			c->stop = FALSE;
			c->repeat = TRUE;
			c->paused = TRUE;
			c->session = ss;
			ss->timers = g_list_append (ss->timers, c);
		} else {
			g_free (c);
		}
	}
  } else {
    config_add_gerror (cfg, error);
    error = NULL;
  }

  return TRUE;
}

static gboolean
session_load_old(SESSION_STATE * ss, const gchar * slot)
{
  gchar *k = NULL;  // key
  gchar *e = NULL;  // name/expr
  gchar *file = NULL; // file
  int cmds;   // number of commands
  char **commands;  // commands
  FILE *f;

  file = g_build_path(G_DIR_SEPARATOR_S, slot, "config", NULL);

  f = fopen(file, "r");

  g_free(file);

  if (!f) {
    g_warning("couldn't open slot");
    return FALSE;
  }
  while (utils_get_next(f, &k, &e, &cmds, &commands, &file)) {
    char *v = e;
    if (!strcmp(k, "game_name")) {
      ss->game_name = g_strdup(v);
    }
    if (!strcmp(k, "game_host")) {
      ss->game_host = g_strdup(v);
    }
    if (!strcmp(k, "game_port")) {
      ss->game_port = utils_atoi(v, -1);
    }
    if (!strcmp(k, "font")) {
      ss->font = g_strdup(v);
    }
    if (!strcmp(k, "bg_color")) {
      ss->bg_color = g_strdup(v);
    }
    if (!strcmp(k, "fg_color")) {
      ss->fg_color = g_strdup(v);
    }
    if (!strcmp(k, "ufg_color")) {
      ss->ufg_color = g_strdup(v);
    }
    if (!strcmp(k, "single_line")) {
      ss->single_line =
          strcmp(v, "on") ? FALSE : TRUE;
    }
    if (!strcmp(k, "local_echo")) {
      ss->local_echo =
          strcmp(v, "on") ? FALSE : TRUE;
    }
    if (!strcmp(k, "logging")) {
      ss->logging = strcmp(v, "on") ? FALSE : TRUE;
    }
    if (!strcmp(k, "sound")) {
      ss->sound = strcmp(v, "on") ? FALSE : TRUE;
    }
    if (g_str_has_prefix(k, "trigger")) {
      ATM *atm = config_migrate_atm(ATM_TRIGGER, e, cmds, commands, file, ss);
      ss->triggers =
          g_list_append(ss->triggers,
            (gpointer) atm);
    }
    if (g_str_has_prefix(k, "alias")) {
      ATM *atm = config_migrate_atm(ATM_ALIAS, e, cmds, commands, file, ss);
      ss->aliases =
          g_list_append(ss->aliases, (gpointer) atm);
    }
    if (g_str_has_prefix(k, "macro")) {
      ATM *atm = config_migrate_atm(ATM_MACRO, e, cmds, commands, file, ss);
      ss->macros =
          g_list_append(ss->macros, (gpointer) atm);
    }
    g_free(k);
    k = NULL;
    g_free(file);
    file = NULL;
    g_free(e);
    e = NULL;
    if (commands) {
      int i;
      for (i = 0; i < cmds; ++i)
        g_free(commands[i]);
      g_free(commands);
    }
  }
  fclose(f);

  // variables
  file = g_build_path(G_DIR_SEPARATOR_S, slot, "variables", NULL);
  f = fopen(file, "r");
  g_free(file);

  if (!f) {
    g_warning("couldn't open variable list");
  } else {
    varlist_load(ss->variables, f);
    fclose(f);
  }


  // command history
  file = g_build_path(G_DIR_SEPARATOR_S, slot, "history", NULL);
  f = fopen(file, "r");
  g_free(file);

  if (f) {
    cmd_entry_history_load (&ss->cmdline, f);
    fclose(f);
  }

        return TRUE;
}

static gboolean
session_saved_get_name_old (const gchar * slot, gchar ** name, gchar ** game_name);


/**
 * session_saved_get_name: Gets primary information about slot.
 *
 * Return value: TRUE if slot's configuration founded,
 *               otherwise false.
 **/
gboolean
session_saved_get_name (const gchar* slot, gchar** name, gchar** game_name, gchar ** proxy_name)
{
  gint      res;
  GError*   error = NULL;
  gchar*    filename;
  GKeyFile* file;

  gboolean  upgrade = ! config_check_new_version_session (slot);

  if (upgrade)
    {
      mdebug (DBG_CONFIG, 0,"Detected old-style configuration for slot %s.\n", slot);
      return session_saved_get_name_old (slot, name, game_name);
    }

  file = g_key_file_new ();

  filename = g_build_path(G_DIR_SEPARATOR_S, slot, MUDCFG_CONFIG_SLOT_FILE, NULL);
  res = g_key_file_load_from_file(file, filename, G_KEY_FILE_NONE, &error);
  g_free (filename);
  if (!res)
    {
      * name = NULL;
      * game_name = NULL;
      if (proxy_name) * proxy_name = NULL;
      return FALSE;
    }

  config_load_string (file, MUDCFG_SLOT_MAIN_GROUP, "name", name, NULL);
  config_load_string (file, MUDCFG_SLOT_MAIN_GROUP, "game_name", game_name, NULL);
  if (proxy_name && !config_load_string (file, MUDCFG_SLOT_MAIN_GROUP, "proxy", proxy_name, NULL)) *proxy_name = g_strdup ("Default");

  g_key_file_free (file);

  return TRUE;
}

gboolean
session_saved_get_name_old (const gchar * slot, gchar ** name, gchar ** game_name)
{
  gchar *file;
  FILE *f;
  gchar *k = NULL;
  gchar *v = NULL;

  if (name)
    *name = NULL;
  if (game_name)
    *game_name = NULL;

  file = g_build_path(G_DIR_SEPARATOR_S, slot, "config", NULL);
  f = fopen(file, "r");
  if (!f) {
    g_free(file);
    return FALSE;
  }
  g_free(file);

  while (utils_get_next(f, &k, &v, NULL, NULL, NULL)) {
    if (k) {
      if (!strcmp(k, "name")) {
        if (name)
          *name = v;
      } else if (!strcmp(k, "game_name")) {
        if (name)
          *game_name = v;
      } else {
        g_free(v);
        v = NULL;
      }
      g_free(k);
      k = NULL;
    }
  }
  fclose(f);

        return TRUE;
}

/**
 * session_slot_is_empty: Checks whether specified is empty. (No any files).
 *
 * @slot: Full path to slot.
 *
 * Return value: TRUE if slot is empty,
 *               FALSE otherwise.
 **/
gboolean
session_slot_is_empty (const gchar* slot)
{
  GDir*    dir;
  gboolean ret;

  g_assert (slot);

  if (! g_file_test (slot, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
      return TRUE;

  dir = g_dir_open (slot, 0, NULL);

  if (NULL == dir)
      return TRUE;

  ret = g_dir_read_name (dir) == NULL;

  g_dir_close (dir);

  return ret;
}

/**
 * session_remove_empty_slot: Removes slot from disk.
 *
 * @path: A path to slot.
 *
 **/
void
session_remove_empty_slot (const gchar* path)
{
  gint result;

  g_assert (path);

  result = g_remove (path);

  if (0 != result)
    {
      g_warning ("Cannot delete dir '%s'.", path);
    }

  mdebug (DBG_CONFIG, 0,"Deleting slot %s: %s\n", path, (result ? "FAILED" : "OK"));
}

void session_saved_set_proxy (const gchar * slot, const gchar * proxy_name) {
	gboolean  upgrade = !config_check_new_version_session (slot);
	
	if (upgrade) {
		mdebug (DBG_CONFIG, 0,"Detected old-style configuration for slot %s. No proxy changed for this session.\n", slot);
	} else {
		gchar*    filename;
		GKeyFile* file;
		gint      res;
		GError*   error = NULL;

		file = g_key_file_new ();
		filename = g_build_path (G_DIR_SEPARATOR_S, slot, MUDCFG_CONFIG_SLOT_FILE, NULL);
		res = g_key_file_load_from_file (file, filename, G_KEY_FILE_NONE, &error);
		if (res) {
			if (proxy_name) {
				config_save_string (file, MUDCFG_SLOT_MAIN_GROUP, "proxy", proxy_name);
			} else {
				g_key_file_remove_key (file, MUDCFG_SLOT_MAIN_GROUP, "proxy", &error);
			}
		}
		config_save_key_file (file, filename, NULL);
		g_key_file_free (file);
		g_free (filename);
	}
}

gboolean session_saved_load_icon (const gchar * name, GdkPixbuf ** pix) {
	gchar * icfile = NULL;
	GList * i;
	gboolean found = FALSE;
	GameListItem * gli;

	* pix = NULL;
	if (!config->gamelist) gl_get_games (config->gamelistfile, &config->gamelist, NULL);
	for (i = g_list_first (config->gamelist); i && !found; i = g_list_next (i)) {
		gli = (GameListItem *) i->data;

		found = !g_ascii_strcasecmp (gli->title, name);
	}
	if (found && gli->game_icon) {
		icfile = gl_get_icon_filename (gli->game_icon);
		if (icfile) {
			*pix = gdk_pixbuf_new_from_file (icfile, NULL);
			if (icfile) g_free (icfile);
		}
	}
	return found;
}

