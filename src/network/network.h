/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* network.h:                                                              *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                2005  Shlykov Vasiliy ( vash@vasiliyshlykov.org )        *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef NETWORK_H
#define NETWORK_H 1
#include <glib/gtypes.h>

#ifndef HAVE_WINDOWS
# include <sys/socket.h>
# include <netinet/in.h>
typedef struct in_addr mud_ip_addr;
#else
typedef int mud_ip_addr;
#endif

#define NO_CONNECTION   -1

#define MUD_NEW_FILE_MODE 0644
//#define MUD_NEW_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/**
 * ConnectionStatus: Status of network operation.
 *
 * Keep sync with #network_errmsg[].
 **/
enum ConnectionStatus
{
  CONNECT_OK = 0,
  CONNECT_BAD_SOCKET   = -1,
  CONNECT_UNKNOWN_HOST = -2,
  CONNECT_SOCKET_ERROR = -3,
  CONNECT_BAD_FILE     = -4,
  CONNECT_BAD_URL      = -5,
  CONNECT_CANCELED     = -6,
  CONNECT_BAD_PARAM    = -7
};

const gchar*
network_errmsg (gint error);

gint network_connection_open( const gchar *host, gint port );
void network_connection_close( gint fd ); 

gsize network_data_recv( gint fd, guchar *buff, gsize size );
gsize network_data_send( gint fd, guchar *buff, gsize size );



gboolean mud_ip_parse (const gchar* ipaddr, mud_ip_addr* dest);
//gboolean mud_ip_cmp (const mud_ip_addr* ip1, const mud_ip_addr* ip2);
gboolean mud_ip_local (const mud_ip_addr* ip);

#ifdef HAVE_WINDOWS
static inline gboolean
mud_ip_cmp (const mud_ip_addr* ip1, const mud_ip_addr* ip2)
{
  return FALSE;
}
#else
static inline gboolean
mud_ip_cmp (const mud_ip_addr* ip1, const mud_ip_addr* ip2)
{
  g_assert (ip1);
  g_assert (ip2);

  return ip1->s_addr == ip2->s_addr;
}
#endif

#endif //NETWORK_H

