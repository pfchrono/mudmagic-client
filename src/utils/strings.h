/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* utils.h:                                                                *
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
#include "mudmagic.h"
char* string_substitute (const char  * original,
                      const char  * search,
                      const char  * replace);

char* safe_strstr (const char * s1,
                  const char * s2);
void replace_gstr (char ** target_free_old, char * assign_from_me);
