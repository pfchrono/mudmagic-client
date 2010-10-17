/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* debug.h:                                                                *
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
#ifndef __DEBUG_H__MUDCLIENT
#define __DEBUG_H__MUDCLIENT

#define DBG_CONFIG      0 /* should be 1 for debug messages to stdout */
#define DBG_ISCRIPT     0 /* should be 2 for debug messages to stdout */
#define DBG_ATM         0 /* should be 3 for debug messages to stdout */
#define DBG_NETWORK     0 /* should be 4 for debug messages to stdout */
#define DBG_GAMELIST	0 /* should be 5 for debug messages to stdout */
#define DBG_CMDENTRY    0 /* should be 6 for debug messages to stdout */
#define DBG_ACCELS      0 /* should be 7 for debug messages to stdout */
#define DBG_MODULE      0 /* should be 8 for debug messages to stdout */
#define DBG_SOUND	0 /* should be 9 for debug messages to stdout */
#define DBG_MAX_LEVEL 4

#ifdef DEBUG

void mdebug (int module, int level, const char* fmt, ...);

#else // DEBUG

static inline void mdebug (int module, int level, const char* fmt, ...) { }

#endif // DEBUG


#endif // __DEBUG_H__MUDCLIENT

