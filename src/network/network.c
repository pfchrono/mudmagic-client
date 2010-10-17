/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* network.c:                                                              *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                   *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifdef HAVE_WINDOWS
  #include <winsock.h> 
  #include <unistd.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>  
#endif // HAVE_WINDOWS

#include <mudmagic.h>
#include <interface.h>
#include "network.h"
#include <errno.h>

static const char* network_errmsgs[] =
{
/* CONNECT_OK */            "Success",
/* CONNECT_BAD_SOCKET */    "Unable to create connecting socket.",
/* CONNECT_UNKNOWN_HOST */  "Unknown host, or unable to look up host.",
/* CONNECT_SOCKET_ERROR */  "Socket error.",
/* CONNECT_BAD_FILE */      "Cannot open file.",
/* CONNECT_BAD_URL */       "Bad URL.",
/* CONNECT_CANCELED */      "Operation canceled by user.",
/* CONNECT_BAD_PARAM */     "Bad parameter."
};

const gchar*
network_errmsg (gint error)
{
  error = -error;

  if (sizeof(network_errmsgs)/sizeof(*network_errmsgs) <= error || error < 0)
      return NULL;

  return network_errmsgs[error];
}

gint network_connection_open( const gchar *host, gint port )
{
  gint sock;
  struct sockaddr_in name;
  struct hostent *hostinfo;

  g_return_val_if_fail( (host != NULL) && ( *host != 0 ), CONNECT_UNKNOWN_HOST );

  mdebug (DBG_NETWORK, 0, "connecting to host:%s on port:%d\n", host, port );

  // create the socket 
  if ( ( sock = socket( PF_INET , SOCK_STREAM , 0 ) ) < 0 ) 
    {
      return CONNECT_BAD_SOCKET;
    }

  // fill the socket name
  name.sin_family = AF_INET;
  name.sin_port   = htons( port ) ;
  hostinfo        = gethostbyname( host );

  if ( hostinfo == NULL ) 
    {
      return CONNECT_UNKNOWN_HOST;
    }
  name.sin_addr = *(struct in_addr *) hostinfo->h_addr;

  // connect to the server
  if ( connect ( sock, (struct sockaddr*)&name, sizeof(name) ) < 0 ) 
    {
      return CONNECT_SOCKET_ERROR;
    }
  
  return sock;
}

void network_connection_close( gint fd )
{
  mdebug (DBG_NETWORK, 0, "closing connection on socket :%d\n", fd );
# ifdef HAVE_WINDOWS
  closesocket( fd );
# else
  shutdown( fd, SHUT_RDWR );
# endif // HAVE_WINDOWS
}

gsize network_data_recv( gint fd, guchar *buff, gsize size )
{
  gsize nb = 0; //number of bytes received
 
  g_return_val_if_fail( buff != NULL, -1 );
  memset( buff, 0, strlen(buff) );
  nb = recv( fd, buff, size, 0 ); // read at most size bytes
  if ((0 > nb) && (EAGAIN != errno))
  {
  interface_display_message("Route to host lost.");
        mdebug (DBG_NETWORK, 0, "connection closed by foreign host (%d)\n", fd );
  return nb;
  }

# if DBG_NETWORK > 0
  mdebug(DBG_NETWORK, 0, "RECV %d bytes from socket %d :", nb, fd );
  //utils_dump_data( buff, nb );
  int i;

  for ( i = 0 ; i < (nb < 15 ? nb : 15) ; i++ ) 
        printf("%d ", buff[i] );
  if ( nb >= 15 ) printf(" ... ");
  printf("\n");
# endif // DEBUG
  return nb;
}

// if len is -1 buff must be null-terminated
gsize network_data_send( gint fd, guchar *buff, gsize len )
{
  gsize sent = 0 ;    // how many bytes are sent
  gsize n;

  g_return_val_if_fail( buff != NULL, -1 );
  if ( len == -1 ) len = strlen( buff );

# if DBG_NETWORK > 0
  mdebug(DBG_NETWORK, 0, "SEND %d bytes to socket %d :", len, fd );
  //utils_dump_data( buff, len );
  int i;
  for ( i = 0 ; i < len ; i++ ) 
        printf("%d ", buff[i] );
  printf("\n");
#endif // DEBUG

  // handling partial send  ...
  while ( sent < len )
    {
      n = send( fd, buff, len - sent, 0 ); 
      if ( n == -1 )
        {
            // ... and recoverable or temporary error
            if (errno == EINTR || errno == EAGAIN ) {
                    g_warning( "network_data_send: recoverable/temporary error ");  
                    continue;
            }
            g_warning("network_data_send: error !");  
            return -1;
        }
      else
        {
            buff += n;
            sent += n;
        }
    }
  return sent;
}

#ifdef HAVE_WINDOWS
/**
 * mud_ip_new:
 *
 **/
gboolean mud_ip_parse (const gchar* str, mud_ip_addr* ipaddr)
{
  return FALSE;
}

gboolean mud_ip_local (const mud_ip_addr* ip)
{
  return FALSE;
}
#else

gboolean mud_ip_parse (const gchar* str, mud_ip_addr* ipaddr)
{
  g_assert (ipaddr);

  if (str == NULL)
      return FALSE;

  return inet_aton (str, ipaddr);
}

gboolean mud_ip_local (const mud_ip_addr* ip)
{
  in_addr_t net;
  int a, b, c;

  g_assert (ip);

  //net = inet_netof (*ip);
  net = ip->s_addr;

  a = net & 0xff;
  b = (net >> IN_CLASSC_NSHIFT) & 0xff;

  if (a == 10
          || (a == 172 && b >= 16 && b <= 31)
          || (a == 172 && b == 0)
          || (a == 192 && b == 168)
     )
      return TRUE;

  return FALSE;
}
#endif
