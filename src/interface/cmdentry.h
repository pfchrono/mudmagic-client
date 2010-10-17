#ifndef __CMDENTRY_H
#define __CMDENTRY_H

#include <gtk/gtkentry.h>

#define DEFAULT_CMD_BUF_SIZE    	48
#define DEFAULT_CMD_AUTOCOMPLETION	1

GtkWidget*
cmd_entry_create ();

void
cmd_entry_update_cache (GtkWidget* tree);

GtkWidget*
interface_get_cmdentry (GtkWidget* tree);

void
cmd_entry_init (GtkWidget *widget, COMMAND_LINE*);

void
cmd_entry_history_save (COMMAND_LINE*, FILE* f);

void
cmd_entry_history_load (COMMAND_LINE*, FILE* f);

void
cmd_entry_set_autocompletion (GtkEntry*, gboolean enable);

void
cmd_entry_set_history_size (GtkEntry*, guint size);

void
cmd_entry_history_clear (COMMAND_LINE*, guint size);

void
cmd_entry_set_focus (void);

#endif

