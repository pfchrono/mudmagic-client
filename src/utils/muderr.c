/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* muderr.c:                                                               *
*                2005 Shlykov Vasiliy ( vash@zmail.ru )                   *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <muderr.h>

GQuark  MUD_NETWORK_ERROR;
GQuark  MUD_GAMELIST_ERROR;

void init_muderr (void)
{
  MUD_NETWORK_ERROR = g_quark_from_string ("MUD_NETWORK_ERROR");
  MUD_GAMELIST_ERROR = g_quark_from_string ("MUD_GAMELIST_ERROR");
}

