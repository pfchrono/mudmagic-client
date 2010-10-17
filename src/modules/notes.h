/***************************************************************************
 *  Mud Magic Client                                                       *
 *  Copyright (C) 2004 MudMagic.Com ( hosting@mudmagic.com )               *
 *                2004 Calvin Ellis ( kyndig@mudmagic.com  )               *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NOTES
#define NOTES
#include <glib.h>

typedef struct _NOTE NOTE;
struct _NOTE {
	gchar *title;
	gchar *text;
	gchar *time;
};

#endif //NOTES
