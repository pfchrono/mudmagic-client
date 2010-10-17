/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* telnet.c:                                                               *
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
#include <stdarg.h>
#include <glib.h>
#include <mudmagic.h>
#include <interface.h>
#include "telnet.h"
#include "msp.h"
#include "mxp.h"
#include "zmp.h"


static void telnet_send_iac(gint fd, gint count, ...)
{
  gchar buff[16];
  va_list va;
  gint i, byte, len;
  len = 0;
  buff[len++] = IAC;
  va_start(va, count);
  for (i = 0; i < count; i++) {
    byte = va_arg(va, gint);
    buff[len++] = byte;
    if (len >= 16) {
      network_data_send(fd, buff, len);
      len = 0;
    }
    if (byte == IAC) {
      buff[len++] = byte;
      if (len >= 16) {
        network_data_send(fd, buff, len);
        len = 0;
      }
    }
  }
  if (len >= 0) {
    network_data_send(fd, buff, len);
  }
  va_end(va);
}

//this is similar to telnet_send_iac, but it appends IAC SB to the start,
//and IAC SE to the end (can't use telnet_send_iac, as it would corrupt
//the ending IAC SE by doubling the IAC ...
static void telnet_send_subneg(gint fd, gint count, ...)
{
  gchar buff[64];
  va_list va;
  gint i, byte, len;
  len = 0;
  buff[len++] = IAC;
  buff[len++] = SB;
  va_start(va, count);
  for (i = 0; i < count; i++) {
    byte = va_arg(va, gint);
    buff[len++] = byte;
    if (len >= 64) {
      network_data_send(fd, buff, len);
      len = 0;
    }
    if (byte == IAC) {
      buff[len++] = byte;
      if (len >= 64) {
        network_data_send(fd, buff, len);
        len = 0;
      }
    }
  }
  if (len >= 62) {
    network_data_send(fd, buff, len);
    len = 0;
  }
  //send the final IAC SE
  buff[len++] = IAC;
  buff[len++] = SE;
  network_data_send(fd, buff, len);
  va_end(va);
}

// JD - add ttype sendign
void telnet_send_ttype(TELNET_STATE *tn)
{
   gchar buff[64];

   sprintf(buff, "%c%c%c%cMudMagic %s%c%c", IAC, SB, TELOPT_TTYPE,
              TELQUAL_IS, VERSION, IAC, SE );

   network_data_send(tn->fd, buff, strlen(buff));
}

void telnet_send_window_size(TELNET_STATE * tn, gint w, gint h)
{
  g_return_if_fail(tn != NULL);
  g_message("NAWS send width=%d height=%d", w, h);
  telnet_send_subneg(tn->fd, 5, TELOPT_NAWS,
         (w >> 8) & 0xff, w & 0xff, (h >> 8) & 0xff,
         h & 0xff);
}

void telnet_mccp_begin(TELNET_STATE * tn)
{
  g_return_if_fail(tn != NULL);
  if (tn->mccp_ver > 0) {
    tn->zstream = (z_stream *) g_malloc0(sizeof(z_stream));
    if (inflateInit(tn->zstream) != Z_OK) {
      g_warning("failed to initialize z_stream: %s",
          tn->zstream->msg);
    }
  }
}

void telnet_mccp_end(TELNET_STATE * tn)
{
  g_return_if_fail(tn != NULL);
  if (tn->zstream != NULL) {
    inflateEnd(tn->zstream);
    g_free(tn->zstream);
    tn->zstream = NULL;
  }
}

void telnet_process_subnegotiation(TELNET_STATE * tn)
{
  g_return_if_fail(tn != NULL);
  if ((tn->ubuff[0] == TELOPT_COMPRESS2) ||
      (tn->ubuff[0] == TELOPT_COMPRESS)) {
    g_message("MCCP compression begins");
    telnet_mccp_begin(tn);
    // move the rest from dbuff to rbuff to be decompressed
    // and reset dlen to get out from process loop 
    if (tn->dpos < tn->dlen) {
      memcpy(tn->rbuff,
             tn->dbuff + tn->dpos + 1,
             tn->dlen - tn->dpos - 1);
      tn->rlen = tn->dlen - tn->dpos - 1;
      // feed the zstream 
      tn->zstream->next_in = tn->rbuff;
      tn->zstream->avail_in = tn->rlen;

      tn->dpos = tn->dlen = 0;
    }
  }
  if (tn->ubuff[0] == TELOPT_ZMP) {
    zmp_handle(tn->fd, tn->ubuff + 1, tn->ulen - 1);
  }
  if (tn->ubuff[0] == TELOPT_TTYPE )
  {
    telnet_send_ttype(tn);
  }
  // clear the junk buffer 
  memset(tn->ubuff, 0, tn->ulen);
  tn->ulen = 0;
}

void telnet_mccp_decompress(TELNET_STATE * tn)
{
  g_return_if_fail(tn != NULL);
  gint err;
  // prepare buffer out
  memset(tn->dbuff + tn->dlen, 0, BUFFSIZE - tn->dlen);
  tn->zstream->next_out = tn->dbuff + tn->dlen;
  tn->zstream->avail_out = BUFFSIZE - tn->dlen;

  // decompress some data
  err = inflate(tn->zstream, Z_SYNC_FLUSH);
  if ((err == Z_OK) || (err == Z_STREAM_END)) {
    tn->dlen = BUFFSIZE - tn->zstream->avail_out;
    //printf(" >>>some data available %d bytes\n", tn->dlen );
    if (err == Z_STREAM_END) {
	if(tn->dlen + tn->zstream->avail_in < BUFFSIZE) {
        	memmove(tn->dbuff + tn->dlen,
              		tn->zstream->next_in,
              		tn->zstream->avail_in);
       		tn->dlen += tn->zstream->avail_in;
     	} else {
		// ???
     	}
      telnet_mccp_end(tn);
    }
  } else {
    g_warning("failed to inflate: %s", tn->zstream->msg);
  }
}



//should be called when some data is available
void telnet_process(TELNET_STATE * tn, gchar ** buff, gsize * len)
{
  gint i, j, k = 0;

  gchar *pbuff;   // processed data buffer  ( will be passed to caller )
  gsize plen;   // the lenght pbuff               ( will be passed to caller )
  gsize psize;    // the pbuff buffer size
  gsize old_dlen;


  g_return_if_fail(tn != NULL && buff != NULL && len != NULL);

  // get some data from socket
  tn->rlen =
      network_data_recv(tn->fd, tn->rbuff, BUFFSIZE - tn->ulen);


  if (tn->rlen <= 0) {  // connection close or error
//    if (tn->rlen < 0) // error
    network_connection_close(tn->fd);
    tn->fd = NO_CONNECTION;
    // clean the telnet state 
    telnet_reset(tn);
    *buff = NULL;
    *len = 0;
    return;
  }

  if (tn->fd == NO_CONNECTION) {
    // got data after disconnect ...
    // clean the telnet state 
    telnet_reset(tn);
    *buff = NULL;
    *len = 0;
    return;
  }

  plen = 0;
  psize = 2 * BUFFSIZE;
  pbuff = g_malloc0(psize);

  // prepare data buffer
  old_dlen = 0;
  memset(tn->dbuff, 0, tn->dlen);
  tn->dlen = 0;

  if (tn->state == TN_STATE_ANSI) {
    // we have in unprocessed buffer some broken ansi
    memcpy(tn->dbuff, tn->ubuff, tn->ulen);
    old_dlen = tn->dlen = tn->ulen;
    // reset unprocessed buffer
    memset(tn->ubuff, 0, tn->ulen);
    tn->ulen = 0;
  }

  if (tn->zstream != NULL) {  // we have to decompress ?
    tn->zstream->next_in = tn->rbuff;
    tn->zstream->avail_in = tn->rlen;
  } else {
    // there is enough space cos tn->rlen + tn->dlen < BUFFSIZE
    // append bytes from socket 
    memcpy(tn->dbuff + tn->dlen, tn->rbuff, tn->rlen);
    tn->dlen += tn->rlen;

    // resert read buffer
    memset(tn->rbuff, 0, tn->rlen);
    tn->rlen = 0;
  }

  while (TRUE) {    // the main loop ... process until we have no more data
    if ((tn->zstream != NULL) && (tn->zstream->avail_in > 0)) {
      // we have to decompress ?
      telnet_mccp_decompress(tn);
    }
    if ((tn->dlen == old_dlen && tn->dlen > 0)
        || tn->dlen == 0) {
      // break when we don't have more data
      break;
    }
    //that will guarantee that we will have space for all data from dbuff
    if (plen + tn->dlen >= psize) {
      psize += BUFFSIZE;
      if (psize >= 100 * BUFFSIZE) {
        // something verry strage it's happening there
        g_error("too much processed data"); //100 times ? from where ?
      }
      pbuff = g_realloc(pbuff, psize);
    }
    // start telnet processor
    k = 0;
    // process 

    for (tn->dpos = old_dlen; tn->dpos < tn->dlen; tn->dpos++) {
      i = tn->dpos; // we'll use "i" for a cleaner code

      /*
         printf(
         " code = %3d, state = %d, char = %c \n", 
         tn->dbuff[i], tn->state, tn->dbuff[i] 
         ); */
      switch (tn->state) {
      case TN_STATE_NORMAL:{
          switch (tn->dbuff[i]) {
          case 0:{
             k = i + 1;
	   }
           break;
          case IAC:{
              tn->state =
                  TN_STATE_IAC;
            }
            break;
          case ESC:{
              tn->state =
                  TN_STATE_ANSI;
            }
            break;

          default:{
/**************
 patch from forums
 http://www.mudmagic.com/mud-client/boards/developer/14/54/54
*****/
              if (tn->dbuff[i] !=
                  13) {
                pbuff
                    [plen++]
                    =
                    tn->
                    dbuff
                    [i];
              }
/**
	      pbuff[plen++] = tn->dbuff[i];
**/
              k = i + 1;
            }
          }
        }
        break;

      case TN_STATE_ANSI:{
          if (g_ascii_isalpha(tn->dbuff[i])) {  // end ANSI code
            // 'm' for color codes, 'z' for MXP sequences
            if ((tn->dbuff[i] == 'm')
                || (tn->dbuff[i] ==
              'z')) {
              for (j = k; j <= i;
                   j++) {
                pbuff
                    [plen++]
                    =
                    tn->
                    dbuff
                    [j];
              }
            }
            tn->state =
                TN_STATE_NORMAL;
            k = i + 1;
          }
        }
        break;

      case TN_STATE_IAC:{
          switch (tn->dbuff[i]) {
          case IAC:{  // escaped IAC case
              pbuff[plen++] =
                  IAC;
              k = i + 1;
              tn->state =
                  TN_STATE_NORMAL;
            }
            break;
          case WILL:{
              tn->state =
                  TN_STATE_WILL;
            }
            break;
          case WONT:{
              tn->state =
                  TN_STATE_WONT;
            }
            break;
          case DO:{
              tn->state =
                  TN_STATE_DO;
            }
            break;
          case DONT:{
              tn->state =
                  TN_STATE_DONT;
            }
            break;
          case SB:{
              tn->state =
                  TN_STATE_SB;
            }
            break;

         case 249:       // IAC GA
         case 239:       // IAC EOR
                         // signal a prompt.
         default:{
              k = i + 1;
              tn->state =
                  TN_STATE_NORMAL;
            }
          }
        }
        break;

      case TN_STATE_WILL:{
          tn->state = TN_STATE_NORMAL;
          switch (tn->dbuff[i]) {
          case TELOPT_COMPRESS2:{
              if (tn->mccp_ver ==
                  0) {
                telnet_send_iac
                    (tn->
                     fd, 2,
                     DO,
                     tn->
                     dbuff
                     [i]);
                tn->mccp_ver = 2;
                g_message
                    ("MCCP v2 enabled");
              }
            }
            break;
          case TELOPT_COMPRESS:{
              if (tn->mccp_ver ==
                  0) {
                telnet_send_iac
                    (tn->
                     fd, 2,
                     DO,
                     tn->
                     dbuff
                     [i]);
                tn->mccp_ver = 1;
                g_message
                    ("MCCP v1 enabled");
              }
            }
            break;
	  case TELOPT_END_OF_RECORD:{
               telnet_send_iac
                    (tn->fd, 2,
                     DO,
                     tn->
                     dbuff
                     [i]);
                g_message
                     ("EOR enabled");
          }
          break;
          case TELOPT_MSP:{
              if (tn->msp ==
                  NULL) {
                telnet_send_iac
                    (tn->
                     fd, 2,
                     DO,
                     tn->
                     dbuff
                     [i]);
                tn->msp =
                    msp_new
                    (tn->
                     link);
                g_message
                    ("MSP enabled");
              }
            }
            break;
          case TELOPT_MXP:{
              telnet_send_iac
                  (tn->fd, 2, DO,
                   tn->dbuff[i]);
              mxp_enable(tn->
                   mxp);
              g_message
                  ("MXP enabled");
            }
            break;
          case TELOPT_ZMP:{
              zmp_init_std();
              telnet_send_iac
                  (tn->fd, 2, DO,
                   tn->dbuff[i]);
              g_message
                  ("ZMP enabled");
            }
            break;
          case TELOPT_ECHO:{
              telnet_send_iac
                  (tn->fd, 2, DO,
                   tn->dbuff[i]);
              tn->echo = TRUE;
              interface_input_shadow
                  (tn->link,
                   tn->echo);
              g_message
                  ("ECHO enabled");
            }
            break;
          default:{
              telnet_send_iac
                  (tn->fd, 2,
                   DONT,
                   tn->dbuff[i]);
            }
          }
          k = i + 1;
        }
        break;

      case TN_STATE_WONT:{
          tn->state = TN_STATE_NORMAL;
          switch (tn->dbuff[i]) {
          case TELOPT_ECHO:{
              telnet_send_iac
                  (tn->fd, 2,
                   DONT,
                   tn->dbuff[i]);
              tn->echo = FALSE;
              interface_input_shadow
                  (tn->link,
                   tn->echo);
              g_message
                  ("ECHO disabled");
            }
            break;
          default:{
              ;
            }
          }
          k = i + 1;
        }
        break;

      case TN_STATE_DO:{
          tn->state = TN_STATE_NORMAL;
          switch (tn->dbuff[i]) {
          case TELOPT_NAWS:{
              if (tn->naws ==
                  FALSE) {
                gint w =
                    0, h =
                    0;
                telnet_send_iac
                    (tn->
                     fd, 2,
                     WILL,
                     tn->
                     dbuff
                     [i]);
                tn->naws =
                    TRUE;
                g_message
                    ("NAWS enabled !");
                interface_get_output_size
                    (tn->
                     link,
                     &w,
                     &h);
                telnet_send_window_size
                    (tn, w,
                     h);
              }
            }
            break;
            // MXP negotiation is weird, it can happen both like DO->WILL
            // and WILL->DO, hence I need to support both
          case TELOPT_MXP:{
              telnet_send_iac
                  (tn->fd, 2,
                   WILL,
                   tn->dbuff[i]);
              mxp_enable(tn->
                   mxp);
              g_message
                  ("MXP enabled");
            }
            break;
          default:{
              telnet_send_iac
                  (tn->fd, 2,
                   WONT,
                   tn->dbuff[i]);
            }
          }
          k = i + 1;
        }
        break;

      case TN_STATE_DONT:{
          tn->state = TN_STATE_NORMAL;
          switch (tn->dbuff[i]) {
          default:{
              ;
            }
          }
          k = i + 1;
        }
        break;

      case TN_STATE_SB:{
          switch (tn->dbuff[i]) {
          case IAC:{
              tn->state =
                  TN_STATE_SB_IAC;
            }
            break;
          case SE:{ // handle COMPRESS v1
              if ((tn->ulen == 2)
                  && (tn->
                ubuff[0] ==
                TELOPT_COMPRESS)
                  && (tn->
                ubuff[1] ==
                WILL)) {
                tn->state =
                    TN_STATE_NORMAL;
                telnet_process_subnegotiation
                    (tn);
                k = i + 1;
                break;
              }
              // else continue with default 
            }
          default:{
              tn->ubuff[tn->
                  ulen++] =
                  tn->dbuff[i];
              if (tn->ulen >=
                  BUFFSIZE) {
                // Report this !!! 
                g_error
                    ("subrequest too long !!!");
              }
            }
          }
        }
        break;

      case TN_STATE_SB_IAC:{
          switch (tn->dbuff[i]) {
          case SE:{
              tn->state =
                  TN_STATE_NORMAL;
              telnet_process_subnegotiation
                  (tn);
              k = i + 1;
            }
            break;
          default:{
              tn->ubuff[tn->ulen++] = IAC;  // ?corect?
              tn->state =
                  TN_STATE_SB;
            }
          }
        }
        break;
      }
    }
    if (tn->state == TN_STATE_ANSI) {
      // we have a unfinished ANSI 
      memmove(tn->dbuff, tn->dbuff + k, tn->dlen - k);
      tn->dlen -= k;
      memset(tn->dbuff + tn->dlen, 0,
             BUFFSIZE - tn->dlen);
      old_dlen = tn->dlen;
    } else {
      old_dlen = tn->dlen = 0;
    }
    k = 0;
  }

  if (tn->state == TN_STATE_ANSI) { // means we have broken ansi code
    for (j = k; j < tn->dlen; j++) {
      tn->ubuff[tn->ulen++] = tn->dbuff[j];
      if (tn->ulen >= BUFFSIZE)
        g_error("too much broken data !!!");
    }
  }
  // utils_dump_data( pbuff, plen );
  // g_print( "rlen=%d dlen=%d ulen=%d plen=%d\n", tn->rlen, tn->dlen, tn->ulen, plen );

  *buff = pbuff;
  *len = plen;

  // process MSP triggers if there are any
  if (tn->msp != NULL) {
    msp_process(tn->msp, buff, len);
  }

}



TELNET_STATE *telnet_new()
{
  TELNET_STATE *ret;
  ret        = g_new0(TELNET_STATE, 1);
  ret->fd    = NO_CONNECTION;
  ret->naws  = FALSE;
  ret->mxp   = mxp_new(); // create a MXP object - we need it even if MXP is not enabled

  return ret;
}

// should be called when disconnect
void telnet_reset(TELNET_STATE * telnet)
{
  g_return_if_fail(telnet != NULL);
  if (telnet->zstream != NULL) {
    telnet_mccp_end(telnet);
    telnet->zstream = NULL;
  }
  // reset MXP
  mxp_free(telnet->mxp);
  telnet->mxp = mxp_new();
  // reset MXP
  if (telnet->msp != NULL) {
    msp_free(telnet->msp);
    telnet->msp = NULL;
  }
  if (telnet->fd != NO_CONNECTION) {
    network_connection_close(telnet->fd);
    telnet->fd = NO_CONNECTION;
  }

  telnet->state = TN_STATE_NORMAL;
  telnet->mccp_ver = 0;
  telnet->naws = FALSE;
  telnet->echo = FALSE;
  telnet->rlen = telnet->dlen = telnet->ulen = 0;
  telnet->dpos = 0;
  memset(telnet->rbuff, 0, BUFFSIZE);
  memset(telnet->dbuff, 0, BUFFSIZE);
  memset(telnet->ubuff, 0, BUFFSIZE);
}


void telnet_free(TELNET_STATE * telnet)
{
  g_return_if_fail(telnet != NULL);
  telnet_reset(telnet);
  // free the MXP object
  mxp_free(telnet->mxp);
  //free telnet object
  g_free(telnet);
}
