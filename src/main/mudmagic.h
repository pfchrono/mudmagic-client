/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* mudmagic.h:                                                             *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#ifndef MUDMAGIC_H
#define MUDMAGIC_H 1

#include <glib.h>
#include <stdio.h>
#include <string.h>

#ifndef HAVE_WINDOWS
#  include <config.h>
#  define USE_GNOME 1
#else
#  include <win32mud.h>
#endif

/* For clicking on URL's */
#ifdef HAVE_WINDOWS
#define WEB_BROWSER "\"c:\\program files\\internet explorer\\iexplore\" %s"
#else
#define WEB_BROWSER "mozilla %s"
#endif

#define MUDMAGIC_VERSION VERSION

#include <muderr.h>
#include <debug.h>
#include <configuration.h>
#include <utils.h>
#include <theme_select.h>

#ifdef HAVE_WINDOWS
#define EXPORT __declspec (dllexport)
#else
#define EXPORT 
#endif // HAVE_WINDOWS

#endif // MUDMAGIC_H
