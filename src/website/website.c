/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* website.c:                                                              *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                2005  Shlykov Vasiliy ( vash@vasiliyshlykov.org )        *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <zlib.h>
#include <glib/gstdio.h>

#include <mudmagic.h>
#include <network.h>
#include <protocols.h>


//decompress .gz gamelist file
static gboolean uncompress_file (int input, int output, MudError** error);

gboolean
website_update_games_database(HttpHelper* hh,
                    const gchar* localfile, const gchar* fileurl, MudError** error)
{
  gchar* tmpfile_templ = "mmXXXXXX";
  gchar* tmpfile_name = NULL;
  int tmpfile = 0;
  int gmfile = 0;
  int ret = TRUE;

  GError* gerror = NULL;

  tmpfile = g_file_open_tmp (tmpfile_templ, &tmpfile_name, &gerror);
  if (tmpfile == -1)
    {
      g_free (tmpfile_name);
      *error = mud_cnv (gerror);
      return FALSE;
    }
  mdebug (DBG_GAMELIST, 0, "Using temp file: %s\n", tmpfile_name);

  ret = http_download (fileurl, tmpfile, hh);

  if( ret != CONNECT_OK )
    {
      *error = mud_error_new (MUD_NETWORK_ERROR, ret, network_errmsg (ret));
      ret = FALSE;
    }
  else
    {
      gmfile = open (localfile, O_WRONLY | O_CREAT | O_TRUNC, MUD_NEW_FILE_MODE);
      if (gmfile == -1)
        {
          *error = mud_error_new (MUD_NETWORK_ERROR, errno, strerror (errno));
          close (tmpfile);
          ret = FALSE;
        }
      else
        {
          lseek (tmpfile, (off_t) 0, SEEK_SET);
          mdebug (DBG_GAMELIST, 0, "Uncompressing to %s...\n", localfile);
          ret = uncompress_file (tmpfile, gmfile, error);
        }
    }
  g_remove (tmpfile_name);
  // close (tmpfile); // closed in uncompress_file
  close (gmfile);

  g_free (tmpfile_name);

  return ret;
}

#define GZBUFSIZE    16384

static gboolean
uncompress_file (int input, int output, MudError** error)
{
  int    len;
  char   buf[GZBUFSIZE];
  gzFile gzin = gzdopen (input, "rb");

  if (gzin == NULL)
    {
      int gzerr;
      const char* gzmsg = gzerror (gzin, &gzerr);
      *error = mud_error_new (MUD_GAMELIST_ERROR, gzerr, gzmsg);
      len = -1;
      close (input);
      return FALSE;
    }

  do
    {
      len = gzread (gzin, buf, GZBUFSIZE);

      if (len < 0)
        {
          int gzerr;
          const char* gzmsg = gzerror (gzin, &gzerr);
          *error = mud_error_new (MUD_GAMELIST_ERROR, gzerr, gzmsg);
          len = -1;
        }
      else if (len > 0)
        {
          if (write (output, buf, len) != len)
            {
              *error = mud_error_new (MUD_GAMELIST_ERROR, errno, strerror (errno));
              len = -1;
            }
        }

    } while (len > 0);

  gzclose (gzin);

  return len == 0;
}

