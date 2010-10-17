/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* zmp-core.c:                                                             *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdlib.h>
#include <time.h>
#include <mudmagic.h>
#include <network.h>
#include "zmp.h"
#include "telnet.h"
#define MAXARG 32

void zmp_send(gint fd, gsize argc, gchar ** argv)
{
	static gchar sb_start[3] = { IAC, SB, TELOPT_ZMP };
	static gchar sb_end[2] = { IAC, SE };
	static gchar iac_iac[2] = { IAC, IAC };

	gint i;
	gchar *p, *iac;
	network_data_send(fd, sb_start, 3);
	for (i = 0; i < argc; i++) {
		p = argv[i];
		while ((iac = strchr(p, IAC)) != NULL) {
			// send what is until IAC
			network_data_send(fd, p, iac - p);
			// escape IAC how as protocol requests
			network_data_send(fd, iac_iac, 2);
			// advance after iac
			p = iac + 1;
		}
		// send the rest of the argument + NUL
		network_data_send(fd, p, strlen(p) + 1);
	}
	network_data_send(fd, sb_end, 2);

}

EXPORT void zmp_handle(gint fd, gchar * buff, gsize size)
{
	ZMP_COMMAND *command;
	gchar *argv[MAXARG];
	gsize argc = 0;
	gchar *c;

	// sanity check
	g_return_if_fail(size > 1);
	g_return_if_fail(g_ascii_isprint(buff[0]));
	g_return_if_fail(buff[size - 1] == '\0');

	// the 0 arg is the command 
	argv[argc++] = buff;

	// find the handler for command 
	command = zmp_lookup(argv[0]);
	if (command == NULL)
		return;

	// split the buffer after \0 
	c = buff;
	while (argc < MAXARG) {
		while (*c != '\0')
			c++;
		// if we hit the last \0  break
		if (c - buff == size - 1)
			break;
		// what is after \0 it's a new arg
		argv[argc++] = ++c;
	}
	// now call the proper function 
	command->function(fd, argc, argv);
}

static void zmp_handle_ping(gint fd, gsize argc, gchar ** argv)
{
	gchar buffer[64];
	gchar *commands[2];

	time_t t;
	time(&t);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));

	/* send time command */
	commands[0] = "zmp.time";
	commands[1] = buffer;
	zmp_send(fd, 2, commands);
}

static void zmp_handle_check(gint fd, gsize argc, gchar ** argv)
{
	g_return_if_fail(argc == 2);
	if (zmp_match(argv[1])) {
		argv[0] = "zmp.support";
		zmp_send(fd, 2, argv);
	} else {
		argv[0] = "zmp.no-support";
		zmp_send(fd, 2, argv);
	}
}

EXPORT void zmp_init_std()
{
	zmp_register("zmp.ping", zmp_handle_ping);
	zmp_register("zmp.check", zmp_handle_check);
}
