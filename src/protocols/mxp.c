/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* mxp.c:                                                                  *
*                2005  Tomas Mecir   ( kmuddy@kmuddy.net   )              *
*                                                       *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <mudmagic.h>
#include <interface.h>
#include "protocols.h"


extern CONFIGURATION *config;

struct MXPINFO {
  MXPHANDLER h;

  // we also keep the whole list of existing images here
  // not really the best possible solution, but oh well, that's Gtk for you ;)
  GTree *images;
};

static gint mystrcmp(const gchar * a, const gchar * b, gpointer data)
{
  return strcmp(a, b);
}

struct MXPINFO *mxp_new()
{
  char name[] = "Monospace";
  int size = 12;
  int false = 0;
  int true = 1;
  RGB fg, bg;
  fg.r = fg.g = fg.b = 192;
  bg.r = bg.b = bg.b = 0;

  struct MXPINFO *ret = g_new0(struct MXPINFO, 1);
  MXPHANDLER h = mxpCreateHandler();
  ret->h = h;

  mxpSetDefaultText(h, name, size, false, false, false, false, fg,
        bg);
  mxpSetNonProportFont(h, name);
  mxpSetHeaderParams(h, 1, name, size * 3, true, false, true, false,
         fg, bg);
  mxpSetHeaderParams(h, 2, name, size * 2, true, false, false, false,
         fg, bg);
  mxpSetHeaderParams(h, 3, name, size * 3 / 2, true, false, false,
         false, fg, bg);
  mxpSetHeaderParams(h, 4, name, size * 4 / 3, true, false, false,
         false, fg, bg);
  mxpSetHeaderParams(h, 5, name, size + 2, true, false, false, false,
         fg, bg);
  mxpSetHeaderParams(h, 6, name, size, true, false, false, false, fg,
         bg);

  mxpSupportsLink(h, 1);
  mxpSupportsSound(h, 1);
  mxpSupportsImage(h, 1);
  mxpSupportsGauge(h, 1);
  mxpSupportsStatus(h, 1);
  mxpSupportsFrame(h, 1);

  mxpSetClient(h, PACKAGE, VERSION);

  // this would enable zMUD style - MXP on at all times
  // only good for testing MXP on non-MXP MUDs, as I believe that having
  // this on by default is a Bad Idea (TM)
  // mxpSwitchToOpen (h);

  // initialize images
  ret->images = g_tree_new((GCompareFunc) mystrcmp);
  return ret;
}

void delete_image_node(gpointer key, gpointer value, gpointer data)
{
  g_free((GdkPixbuf *) value);
}

void mxp_free(struct MXPINFO *mxp)
{
  mxpDestroyHandler(mxp->h);

  // destroy all images in the list
  g_tree_foreach(mxp->images, (GTraverseFunc) delete_image_node, 0);
  // destroy the tree
  g_tree_destroy(mxp->images);

  g_free(mxp);
}

void mxp_enable(struct MXPINFO *mxp)
{
  mxpSwitchToOpen(mxp->h);
}

void mxp_new_text(struct MXPINFO *mxp, gchar * buff, int len)
{
  mxpProcessText(mxp->h, buff);
}

gboolean mxp_has_next(struct MXPINFO *mxp)
{
  return mxpHasResults(mxp->h);
}

mxpChunk *mxp_next(struct MXPINFO * mxp)
{
  return mxpNextResult(mxp->h);
}

gchar *mxp_chunk_text(mxpChunk * chunk)
{
  gchar *res = 0;
  switch (((mxpResult *) chunk)->type) {
  case 1:{    // text
      res = (char *) ((mxpResult *) chunk)->data;
    } break;
  case 6:{    // URL link
      res =
          ((struct linkStruct *) ((mxpResult *) chunk)->
           data)->text;
    } break;
  case 7:{    // send link
      res =
          ((struct sendStruct *) ((mxpResult *) chunk)->
           data)->text;
    } break;
  default:
    break;
  };
  return res;
}

int mxp_chunk_type(mxpChunk * chunk)
{
  return ((mxpResult *) chunk)->type;
}

void *mxp_chunk_data(mxpChunk * chunk)
{
  return ((mxpResult *) chunk)->data;
}

void mxp_formatting(mxpChunk * chunk, char **font, int *size, int *fgcolor,
        int *bgcolor, gboolean * bold, gboolean * italic,
        gboolean * underline)
{
  struct formatStruct *fs =
      (struct formatStruct *) mxp_chunk_data(chunk);

  if (fs->usemask & USE_BOLD)
    *bold = (fs->attributes && USE_BOLD) ? TRUE : FALSE;
  if (fs->usemask & USE_ITALICS)
    *italic = (fs->attributes && USE_ITALICS) ? TRUE : FALSE;
  if (fs->usemask & USE_UNDERLINE)
    *underline = (fs->attributes
            && USE_UNDERLINE) ? TRUE : FALSE;
  if (fs->usemask & USE_FG) {
    RGB c = fs->fg;
    *fgcolor = c.r * 65536 + c.g * 256 + c.b;
  }
  if (fs->usemask & USE_BG) {
    RGB c = fs->bg;
    *bgcolor = c.r * 65536 + c.g * 256 + c.b;
  }
  if (fs->usemask & USE_FONT)
    *font = fs->font;
  if (fs->usemask & USE_SIZE)
    *size = fs->size;
}

void mxp_flag (mxpChunk *chunk, char **name, gboolean *begin)
{
  struct flagStruct *fs = (struct flagStruct *) mxp_chunk_data(chunk);
  *name = fs->name;
  *begin = fs->begin;
}

void mxp_variable(mxpChunk * chunk, char **name, char **value,
      gboolean * erase)
{
  struct varStruct *vs = (struct varStruct *) mxp_chunk_data(chunk);
  *name = vs->name;
  *value = vs->value;
  *erase = vs->erase;
}

void mxp_a_link(mxpChunk * chunk, char **name, char **url, char **text)
{
  struct linkStruct *ls =
      (struct linkStruct *) mxp_chunk_data(chunk);
  *name = ls->name;
  *url = ls->url;
  *text = ls->text;
}

void mxp_send_link(mxpChunk * chunk, char **name, char **cmd, char **text, gboolean *ismenu)
{
  struct sendStruct *ss =
      (struct sendStruct *) mxp_chunk_data(chunk);
  *name = ss->name;
  *cmd = ss->command;
  *text = ss->text;
  *ismenu = ss->ismenu;
}

void mxp_image(mxpChunk * chunk, char **fName, char **url)
{
  struct imageStruct *is =
      (struct imageStruct *) mxp_chunk_data(chunk);
  *fName = is->fname;
  *url = is->url;
}

void mxp_process_image(SESSION_STATE * session, char *name, char *url)
{
  // there are two possibilities; we either have the image, or we don't
  // if we do, all is well, we display it
  // if we don't, we have to download it, if url is given
  char *localpath;
  int l = strlen(config->imagedir) + strlen(name) + 1;
        int fd;

  localpath = g_new0(char, l + 1);
  strcpy(localpath, config->imagedir);
#ifdef HAVE_WINDOWS
  strcat(localpath, "\\");
#else
  strcat(localpath, "/");
#endif        // HAVE_WINDOWS
  strcat(localpath, name);

  // step 1: see if the pixbuf is already loaded
  GdkPixbuf *pixbuf =
      g_tree_lookup(session->telnet->mxp->images, localpath);
  if (pixbuf) {
    // yes, it is - display it !
    interface_image_add(session->tab, 0, pixbuf);
    g_free(localpath);
    return;
  }
  //step 2: pixbuf not loaded, see if the file exists on the disk
  if (g_file_test(localpath, G_FILE_TEST_EXISTS)) {
    // load the pixbuf, add it to the session
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new_from_file(localpath, NULL);
    if (pixbuf) {
      g_tree_insert(session->telnet->mxp->images, localpath, pixbuf);
      interface_image_add(session->tab, 0, pixbuf);
    }
    g_free(localpath);
    return;
  }
  // step 3: file does not exist, try download, if allowed
  if ((!url) || (!config->download)) {
    // can't download, nothing more to do
    g_free(localpath);
    return;
  }
  GtkTextIter it = interface_get_current_position(session);
  HttpHelper* hh = httphelper_new (name);
  // build up URL by adding file name, if it's not included in URL
  char *urlPath;
  if (g_str_has_suffix(url, name))
    urlPath = strdup(url);
  else {
    int l = strlen(url) + strlen(name) + 1;
    urlPath = g_new0(char, l + 1);
    strcpy(urlPath, url);
#ifdef HAVE_WINDOWS
    strcat(urlPath, "\\");
#else
    strcat(urlPath, "/");
#endif        // HAVE_WINDOWS
    strcat(urlPath, name);
  }
        fd = open (localpath, O_WRONLY | O_CREAT | O_TRUNC, MUD_NEW_FILE_MODE);
        if (fd != -1)
        {
  if (http_download(urlPath, fd, hh) == CONNECT_OK)
  {
    // we have the image loaded, proceed as in the local-image case
    // but use the stored iterator
    GdkPixbuf *pixbuf =
        gdk_pixbuf_new_from_file(localpath, NULL);
    if (pixbuf) {
      g_tree_insert(session->telnet->mxp->images, localpath,
              pixbuf);
      interface_image_add(session->tab, &it, pixbuf);
    }
  } else
    g_message("MXP: image download failed");
        close (fd);
        }
        else
        {
            g_error (strerror (errno));
        }

    g_free (hh);
  g_free(urlPath);
  g_free(localpath);
}

void mxp_statusvar (mxpChunk *chunk, char **var, char **mvar, char **caption)
{
  struct statStruct *ss =
      (struct statStruct *) mxp_chunk_data(chunk);
  *var = ss->variable;
  *mvar = ss->maxvariable;
  *caption = ss->caption;
}

void mxp_gauge (mxpChunk *chunk, char **var, char **mvar, char **caption,
    GdkColor *color)
{
  struct gaugeStruct *gs =
      (struct gaugeStruct *) mxp_chunk_data(chunk);
  *var = gs->variable;
  *mvar = gs->maxvariable;
  *caption = gs->caption;
  color->red = gs->color.r;
  color->green = gs->color.g;
  color->blue = gs->color.b;
}

void mxp_window (mxpChunk *chunk, char **name, char **title,
    int *left, int *top, int *width, int *height)
{
  struct windowStruct *ws =
      (struct windowStruct *) mxp_chunk_data(chunk);
  *name = ws->name;
  *title = ws->title;
  *left = ws->left;
  *top = ws->top;
  *width = ws->width;
  *height = ws->height;
}