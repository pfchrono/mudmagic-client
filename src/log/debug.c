/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* debug.c:                                                                *
*                2005  Shlykov Vasiliy ( vash@zmail.ru )                  *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <mudmagic.h>
#include "debug.h"

#ifdef DEBUG

static const char* dbg_modules[] =
{
  "CONFIG",
  "ISCRIPT",
  "A/T/M",
  "NETWORK",
  "GAMELIST",
  "CMDENTRY",
  "CONFIG",
  "ACCELS",
  "MODULE"
};

void mdebug (int module, int level, const char* fmt, ...)
{
  va_list ap;

  if (module <= 0 || module > sizeof(dbg_modules)/sizeof(*dbg_modules))
      return;

  if (level > DBG_MAX_LEVEL)
      return;

  va_start (ap, fmt);

  if (level == 0)
    {
      printf ("%s:\t", dbg_modules[module - 1]);
    }
  else
    {
      while (level--) printf ("\t");
    }

  vprintf (fmt, ap);

  va_end (ap);
}

#endif
