#ifndef TOOLS_H
#define TOOLS_H

struct _delayed_cmd {
	SESSION_STATE * session;
	gchar * command;
	guint interval;
	GTimer * timer;
	gboolean paused;
	gboolean stop;
	gboolean repeat;
};

typedef struct _delayed_cmd delayed_cmd;
void tools_delayed_command_apply (delayed_cmd * c);

#endif
