/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* configuration.h:                                                        *
*                2004 Calvin Ellis  ( kyndig@mudmagic.com )               *
*                2005 Shlykov Vasiliy ( vash@zmail.ru )                   *
*                                                                         *
***************************************************************************/
/**************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef CONFIGURATION_H
#define CONFIGURATION_H 1

#ifndef HAVE_WINDOWS
#  include <config.h>
#endif

#include <stdio.h>
#include <glib/gkeyfile.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkaccelgroup.h>
#include <telnet.h>
#include "proxy.h"

G_BEGIN_DECLS

#define CONFIG_TYPE            (configuration_get_type ())
#define CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONFIG_TYPE, Configuration))
#define CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CONFIG_TYPE, ConfigurationClass))
#define IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONFIG_TYPE))
#define IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CONFIG_TYPE))

#define ATMLanguageCount (2)

typedef struct _Configuration       Configuration;
typedef struct _ConfigurationClass  ConfigurationClass;

typedef struct _Configuration       CONFIGURATION;
typedef struct _Session       SESSION_STATE;
typedef struct _Session       Session;
typedef struct _ATM       ATM;
typedef struct _Language	ATMLanguage;

#include <variables.h>
#include <statusvars.h>
#include <gauges.h>
#include <owindows.h>

struct _Configuration
{
        GtkObject parent_instance;

  int cfg_ver; // version of config file structure.
  GKeyFile*   file;

        gchar*      cfgfile;    // the file configuration name (maybe oold-style)
  gchar*      filename; // the file configuration name (always new-style)
        gchar*      basepath;
        gboolean    upgrade;

  // some fields for easy access
  gchar *gamedir;   // game directory 
  gchar *savedir;   // saves directory
  gchar *macrodir;
        gchar *aliasdir;
        gchar *triggerdir;
  gchar *imagedir;  // images directory - used for MXP

  gchar *gamelistfile;  // name of the game list file (full path).
  gchar *gamelisturl; // URL of game list.
  GList * gamelist; // Game list, may be NULL if not loaded.

  GList *windows;   // maintain by interface module 
  GList *sessions;  // maintain by configuration module
  GList *modules;   // maintain by module module 

  // scripts entries for globals triggers, aliases, macros
  GList *triggers;
  GList *aliases;
  GList *macros;

  gboolean download;  // allow MSP download sounds, and MXP images ?
  gboolean keepsent;  //keep sent text in command input line

  gchar *entry_seperator; // let user input a key to seperate commands: one;two;three

        guint cmd_buf_size;
  gboolean cmd_autocompl;

  GList*      cfg_errors;
  GList * proxies;
  gchar * help_browser;
  gchar * acct_user;
  gchar * acct_passwd;
  time_t gl_last_upd; // time when game list was updated. (time_t) (-1) if no updates yet
  unsigned long int wiz_vis_cols; // vizible colums in the Wizard's game list

#ifndef HAVE_WINDOWS
  gchar *mp3cmd;    // command to play .mp3 files
  gchar *wavcmd;    // command to play .wav files
  gchar *midcmd;    // command to play .mid files
#endif        // HAVE_WINDOWS
};

struct _ConfigurationClass
{
        GtkObjectClass          parent_class;

        void (* configuration) (Configuration *cfg);
};

typedef struct 
{
  gboolean bold;    // the ANSI bright color
  gboolean boldfont;  // bold font, only used by MXP
  gboolean italic;
  gboolean underline;
  gboolean blink;
  gboolean reverse;
  gint fg;    // ANSI fg code
  gint bg;    // ANSI bg code
  gint fgcolor, bgcolor;  //current color, 8 bits for each of R, G, B
  char *font;
  int size;
}TEXT_ATTR;

typedef struct 
{
  gchar *name, *action;
  gboolean isCommand; // true if SEND-link, false if A-link
}LINKINFO;

typedef struct
{
        GQueue      *cmd_buf;
        GList       *cmd_current;
        gboolean    cmd_store;

}COMMAND_LINE;

struct _Session
{
        GtkObjectClass parent_class;

  int cfg_ver; // version of config file structure.
  GKeyFile*   file;
  gboolean    upgrade;

  // base stuff
  gchar *slot;    // directory where the game will be saved 
  gchar *name;    // character name
  gchar *game_name;
  gchar *game_host; // mudserver host
  gint game_port;   // mudserver port

  // client related
  TelnetState *telnet;  // telnet state associated
  guint input_event_id; // used to call a function when some data arrive 
  ProxyConn * pconn;

  gpointer tab;   // input/output Widget tab  

  FILE *log_file;   // logging file 

  // options 
  gboolean single_line; // single/multi line input flag
  gint out_sep_pos; // out separator position (in %)

  gboolean local_echo;
  gboolean logging;
  gboolean sound;
  gchar *font;
  gchar *bg_color;
  gchar *fg_color;
  gchar *ufg_color; // user input color 

  // aliases, triggers, macros
  GList *triggers;
  GList *aliases;
  GList *macros;

  // variables
  VARLIST *variables;

  // status variables and gauges
  SVLIST *svlist;
  GAUGELIST *gaugelist;
  OWINDOWLIST *windowlist;

  // ansi state
  TEXT_ATTR ansi;

  COMMAND_LINE cmdline;

  gchar *linkName, *linkCmd;
  gboolean isSendLink, isMenuLink;
  gchar *imagemapName;
  gchar * proxy;

  // TODO: list of link-tags

  // this will be used by module to store some information in session 
  GHashTable *extra;

  GList*      gerrors;
  // list of timers for delayed commands
  GList * timers;

};

enum ATMType
{
        ATM_ALIAS,
        ATM_TRIGGER,
        ATM_MACRO
};

enum ATMLang
{
		ATM_LANG_NONE = -1,
        ATM_LANG_BASIC = 0,
        ATM_LANG_PYTHON
};

enum ATMAction
{
        ATM_ACTION_TEXT,
        ATM_ACTION_SCRIPT,
        ATM_ACTION_NOISE,
        ATM_ACTION_POPUP
};

struct _Language {
	char name [32];
	int id;
};

extern const ATMLanguage Languages [ATMLanguageCount];

/**
 * _ATM:
 *
 **/
struct _ATM
{
  Session*          session;      // owner of atm, maybe NULL
  Configuration*    config;       // owner of atm, always non-NULL
        gchar*      name;         // name
        int         type;         // ATMType
        int         lang;         // ATMLang
		int	        action;       // ATMAction
        gchar*      text;         // source of script. NULL for non-ATM_ACTION_SCRIPT atm types.
        gchar*      source;       // file name for script and noise action type,
                                  // message body for text and popup action type
        gchar*      raiser;       // regexp or shortcut
        gboolean    disabled;     // temporary disable the action
        GList*      errors;       // list of errors during execution
};


G_END_DECLS


SESSION_STATE *session_new();
void session_delete(SESSION_STATE *);

gboolean
session_slot_is_empty (const gchar* slot);

void
session_remove_empty_slot (const gchar* path);

gboolean
session_saved_get_name(const gchar* slot, gchar** name, gchar** game_name, gchar ** proxy_name);

gchar *session_get_free_slot(CONFIGURATION * config);
void session_save(SESSION_STATE * session);

gboolean
session_load(SESSION_STATE * session, const gchar * slot);

SESSION_STATE *get_current_session ();

GType          configuration_get_type   (void);
GtkObject*     configuration_new        (void);
void         configuration_clear      (Configuration *cfg);

//CONFIGURATION *configuration_init();
void configuration_end(CONFIGURATION * config);

void configuration_delete(CONFIGURATION * config);

gboolean
configuration_save(CONFIGURATION * config);

gboolean
configuration_load(CONFIGURATION * config, const gchar* cfgfile);

gchar*
config_get_log_path (void);

CONFIGURATION *get_configuration (void);

void session_saved_set_proxy (const gchar * slot, const gchar * proxy_name);
gboolean session_saved_load_icon (const gchar* name, GdkPixbuf ** pix);

// savinig atms into proper config file. If ses is NULL it saves to global configuration file
// type is ATMType such as ATM_ALIAS, ATM_TRIGGER or ATM_MACRO
gboolean config_save_atms (GList * atms, SESSION_STATE * ses, int type);

#endif        // CONFIGURATION_H
