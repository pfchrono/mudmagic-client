/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* website.h:                                                              *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
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
#ifndef WEBSITE_H
#define WEBSITE_H 1

gboolean
website_update_games_database (HttpHelper* hh,
				const gchar* localfile,
				const gchar* fileurl,
				MudError** error);

#endif				/* WEBSITE_H */
