/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* directories.h:                                                          *
*                2004 Calvin Ellis  ( kyndig@mudmagic.com )               *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <mudmagic.h>

#ifndef DATADIR
#define DATADIR "."
#endif

const gchar * mudmagic_directory (void);
gchar * mudmagic_data_directory (void);
void mud_dir_remove (const char * path);


#ifdef HAVE_WINDOWS
gchar * mudmagic_toplevel_directory (void);
#endif

