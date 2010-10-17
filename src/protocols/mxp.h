/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* mxp.h:                                                                  *
*                2005  Tomas Mecir   ( kmuddy@kmuddy.net   )              *
*                   *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef MXP_H
#define MXP_H 1

#include <mudmagic.h>
#include <libmxp/libmxp.h>
#include <network.h>

#define TELOPT_MXP 91

#define mxpChunk void
struct MXPINFO;
struct SESSION_STATE;

struct MXPINFO *mxp_new();
void mxp_free(struct MXPINFO *mxp);
void mxp_enable(struct MXPINFO *mxp);
void mxp_new_text(struct MXPINFO *mxp, gchar * buff, int len);
gboolean mxp_has_next(struct MXPINFO *mxp);
mxpChunk *mxp_next(struct MXPINFO *mxp);
gchar *mxp_chunk_text(mxpChunk * chunk);
int mxp_chunk_type(mxpChunk * chunk);
void *mxp_chunk_data(mxpChunk * chunk);
void mxp_formatting(mxpChunk * chunk, char **font, int *size,
        int *fgcolor, int *bgcolor,
        gboolean * bold, gboolean * italic,
        gboolean * underline);
void mxp_flag (mxpChunk *chunk, char **name, gboolean *begin);
void mxp_variable(mxpChunk * chunk, char **name, char **value,
      gboolean * erase);
void mxp_a_link(mxpChunk * chunk, char **name, char **url, char **text);
void mxp_send_link(mxpChunk * chunk, char **name, char **cmd, char **text, gboolean *ismenu);
// only fName and URL are relevant, the rest is not supported
void mxp_image(mxpChunk * chunk, char **fName, char **url);
void mxp_process_image(SESSION_STATE * session, char *name, char *url);
void mxp_statusvar (mxpChunk *chunk, char **var, char **mvar, char **caption);
void mxp_gauge (mxpChunk *chunk, char **var, char **mvar, char **caption,
    GdkColor *color);
void mxp_window (mxpChunk *chunk, char **name, char **title,
    int *left, int *top, int *width, int *height);

#endif        //MXP_H
