/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* strings.c:                                                       	  *
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
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#include <glib.h>
#include <iconv.h>

#include <mudmagic.h>
#include "strings.h"

/**
 * Replaces the search string inside original with the replace string.
 * This should be safe with overlapping elements of text, though I haven't
 * not tested it with elaborately cruel cases.
 * Pan: 1.4
 */
char* string_substitute (const char * original,
                const char * search,
                const char * replace)
{
	size_t slen;		/* length of search */
	size_t rlen;		/* length of replace */
	size_t tlen;		/* length of target (predicted) */
	gint i;
	const char * o;
	const char * pchar;
	char * t;
	char * retval = NULL;

	g_return_val_if_fail (original!=NULL, NULL);
	g_return_val_if_fail (*original!='\0', NULL);
	g_return_val_if_fail (search!=NULL, NULL);
	g_return_val_if_fail (*search!='\0', NULL);
	g_return_val_if_fail (replace!=NULL, NULL);

	slen = strlen (search);
	rlen = strlen (replace);

	/* calculate the length */

	i = 0;
	tlen = 0;
	pchar = original;
	while ((pchar = safe_strstr (pchar, search))) {
		i++;
		pchar += slen;
	}
	tlen = strlen(original) + i*(rlen - slen);

	/**
	***  Make the substitution.
	**/

	o = original;
	t = retval = g_malloc(tlen + 1);
	while ((pchar = safe_strstr (o, search))) {
		(void) memcpy (t, o, (size_t)(pchar-o));
		t += pchar-o;
		(void) memcpy (t, replace, (size_t)rlen);
		t += rlen;
		o = pchar + slen;
	}
	(void) strcpy ( t, o );

	return retval;
}

/* null-safe strstr */
char * safe_strstr (const char * s1, const char * s2)
{
	g_return_val_if_fail (s1!=NULL, NULL);
	g_return_val_if_fail (s2!=NULL, NULL);

	return strstr (s1, s2);
}

void replace_gstr (char ** oldaddr, char    * newval)
{
	char * oldval;

 	g_return_if_fail (oldaddr != NULL);

	oldval = *oldaddr;
	*oldaddr = newval;
	g_free (oldval);
}


