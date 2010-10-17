/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* utils.c:                                                       	  *
*                2004 Calvin Ellis  ( kyndig@mudmagic.com )               *
*		 2005 Mart Raudsepp ( leio@users.sf.net )		  *
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
// for mkdir 
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_WINDOWS
#include <io.h>
#endif

#include <mudmagic.h>
#include "strings.h"
#include "utils.h"

/**
 * utils_join_strs: Join strings placed in #GList to one.
 *
 * Return value: Newly allocated string.
 *
 **/
gchar*
utils_join_strs (GList* list, const gchar* delimiter)
{
  GList* it;
  gchar* res;
  gsize  len = 0,
         dlen = strlen (delimiter);
    
  if (list == NULL)
      return NULL;

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      len += strlen ((char*) it->data) + dlen;
    }

  res = g_new0 (gchar, len + 1);
  *res = '\0';

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      strcat (res, (char*) it->data);
      strcat (res, delimiter);
    }

  return res;
}

gchar*
utils_join_gerrors (GList* list, const gchar* delimiter)
{
  GList* it;
  gchar* res;
  GError*gerr;
  gsize  len = 0,
         dlen = strlen (delimiter);
    
  if (list == NULL)
      return NULL;

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      gerr = (GError*) it->data;
      g_assert (gerr);
      len += strlen (gerr->message) + dlen;
    }

  res = g_new0 (gchar, len + 1);
  *res = '\0';

  for (it = g_list_first (list); it; it = g_list_next (it))
    {
      gerr = (GError*) it->data;
      strcat (res, gerr->message);
      strcat (res, delimiter);
    }

  return res;
}

void
utils_clear_gerrors (GList** gerrors)
{
  GList* it;

  if (*gerrors == NULL)
      return;

  for (it = g_list_first (*gerrors); it; it = g_list_next (it) )
    {
      g_error_free ((GError*)it->data);
    }

  g_list_free (*gerrors);
  *gerrors = NULL;
}

void
utils_clear_errors (GList** errors)
{
  GList* it;

  if (*errors == NULL)
      return;

  for (it = g_list_first (*errors); it; it = g_list_next (it) )
    {
      g_free ((gchar*)it->data);
    }

  g_list_free (*errors);
  *errors = NULL;
}

#if !GLIB_CHECK_VERSION(2,8,0)
gboolean
g_file_set_contents (const gchar *filename,
		     const gchar *contents,
		     gssize	     length,
		     GError	   **error)
{
  FILE* f = fopen (filename, "w");
  
  if (f == NULL)
     return FALSE;
	 
  fprintf (f, "%s", contents);
  
  fclose (f);
  return TRUE;
}
#endif
/**
 * utils_check_subpath: Checks whether @path include the @basepath.
 *
 * @basepath:   Primary path.
 * @path:       Path.
 *
 * Return value: If @basepath is subpath of @path, then remainder of @path
 *               returns, otherwise returns @path
 *
 **/
const char*
utils_check_subpath (const gchar* basepath, const gchar* path)
{
  gchar* convbase = NULL;
  gchar* convpath = NULL;

  const gchar* ret = path;

  g_assert (basepath);
  g_assert (path);

  if (g_utf8_strlen (path, -1) < g_utf8_strlen (basepath, -1))
      return path;
 
  //Under windows case should be ignored in file names.
#ifdef HAVE_WINDOWS
  convbase = g_utf8_casefold (basepath, g_utf8_strlen (basepath, -1));
  convpath = g_utf8_casefold (path, g_utf8_strlen (path, -1));
#else
  convbase = (gchar*) basepath;
  convpath = (gchar*) path;
#endif

  if (g_str_has_prefix (convpath, convbase))
    {
      ret = path + g_utf8_strlen (basepath, -1);
	  if (*ret == G_DIR_SEPARATOR)
	      ret++;
    }

#ifdef HAVE_WINDOWS
  g_free (convbase);
  g_free (convpath);
#endif

  return ret;
}

gboolean utils_mkdir(gchar * name)
{
	g_return_val_if_fail(name, FALSE);
	if (!g_file_test(name, G_FILE_TEST_IS_DIR)) {
#ifdef	HAVE_WINDOWS
		if (mkdir(name) == -1) {
#else
		if (mkdir(name, 0777) == -1) {
#endif
			g_warning("can NOT create directory %s.", name);
			return FALSE;
		}
	}
	return TRUE;
}

// str2int
gint utils_atoi(const gchar * buff, gsize len)
{
	gint i, ret = 0;
	if (len == -1)
		len = strlen(buff);
	for (i = 0; i < len; i++) {
		if (!g_ascii_isdigit(buff[i]))
			return ret;
		ret = ret * 10 + (buff[i] - '0');
	}
	return ret;
}


gsize utils_strip_ansi_codes(gchar * data, gsize len)
{
	gint i, k;
	gboolean ansi = FALSE;
	if (data == NULL || len == 0)
		return 0;	// nothing to strip 

	if (len == -1)
		len = strlen(data);	// len==-1 means data is 0-terminated
	for (i = 0, k = 0; i < len; i++) {
		if (ansi) {
			if (g_ascii_isalpha(data[i]))
				ansi = FALSE;
		} else {
			if (data[i] == 27) {
				ansi = TRUE;
			} else {
				data[k++] = data[i];
			}
		}
	}
	if (k < i)
		data[k] = '\0';
	return k;
}

void utils_dump_data(gchar * buff, gsize len)
{
	gsize i;
	g_return_if_fail(buff != NULL);
	if (len == -1)
		len = strlen(buff);
	printf("%s\n",
	       "====================data dump====================");
	for (i = 0; i < len; i++)
		printf("%c", buff[i]);
	printf("\n%s\n",
	       "====================end  dump====================");
}

// convert \n to \r\n
void utils_LF2CRLF(gchar ** data, gsize * len)
{
	gint i, j, n = 0;
	gchar *newdata = NULL;
	if (data == NULL || *data == NULL || len == NULL || *len == 0)
		return;
	// count how many \n we have
	for (i = 0; i < *len; i++) {
		if ((*data)[i] == '\n') {
			n++;
		}
	}
	if (n > 0) {
		newdata = g_malloc0(*len + n);
		for (i = 0, j = 0; i < *len; i++) {
			if ((*data)[i] == '\n') {
				newdata[j++] = '\r';
			}
			newdata[j++] = (*data)[i];
		}
		g_free(*data);	// release the old data
		*data = newdata;	// set the new data
		*len += n;	// update data lenght
	}

}

// replace character A with character B, giving character A twice will
// result in one A being put to the output
void utils_replace(gchar * data, gsize len, gchar A, gchar B)
{
	gint i, j, sz = 0;
	gboolean haveA = FALSE;
	if (data == NULL || len == 0)
		return;
	sz = len;

	for (i = 0; i < sz; ++i) {
		if (data[i] == A) {
			if (haveA) {
				data[i - 1] = A;
				for (j = i; j < sz - 1; ++j)
					data[j] = data[j + 1];
				data[sz - 1] = '\0';
				--i;	//need to process i-th character again ...
				--sz;	//size gets decreased ... 
				haveA = FALSE;
			} else {
				haveA = TRUE;
				data[i] = B;
			}
		} else
			haveA = FALSE;
	}
}


gboolean try_to_execute_url(const char * template, const char * url)
{
	int i;
	int argc = 0;
	char * freeme;
	char ** argv = NULL;
	gboolean ok = TRUE;

	/* sanity clause */
	if( template == NULL || url == NULL )
		return FALSE;

	// g_return_val_if_fail (is_nonempty_string(template), FALSE);
	// g_return_val_if_fail (is_nonempty_string(url), FALSE);

	/* make sure the backslashes are escaped */
	freeme = string_substitute (template, "\\", "\\\\");
	template = freeme;
	g_message( "about to parse the command [%s]", template);

	/* parse the command line */
	if (ok) {
		GError * err = NULL;
		g_shell_parse_argv (template, &argc, &argv, &err);
		if (err != NULL) {
			g_warning( "Error parsing \"web browser\" command line: %s", err->message);
			g_warning("The command line was: %s", template);
			g_error_free (err);
			ok = FALSE;
		}
	}

	/* substitute in the URL _after_ parsing the command line:
	   path slashes and backslashes in the URL confuse g_shell_parse_argv. */
	for (i=0; i<argc; ++i)
		if (strstr (argv[i], "%s") != NULL) /* filename */
			replace_gstr (&argv[i], string_substitute (argv[i], "%s", url));

	/* spawn off the external editor */
	if (ok) {
		GError * err = NULL;
		g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err);
		if (err != NULL) {
			g_warning("Error starting external browser: %s", err->message);
			g_error_free (err);
			ok = FALSE;
		}
	}

	/* cleanup */
	g_free (freeme);
	g_strfreev (argv);
	return ok;
}

#ifndef HAVE_WINDOWS

gchar*
utils_get_home_dir (void)
{
  return g_get_home_dir ();
}

#else // HAVE_WINDOWS
#define _WIN32_IE 0x0500
#include <shlobj.h>

gchar*
utils_get_home_dir (void)
{
  TCHAR path[MAX_PATH];
  BOOL  ret;

  ret = SHGetSpecialFolderPath (0, path, CSIDL_APPDATA, TRUE);

  return ret ? g_strdup(path) : NULL;
}
#endif // HAVE_WINDOWS


int utils_play_file (char * name) {
#ifdef HAVE_WINDOWS
	gchar * cmd;
	int ret;
	
	mciSendString ("close noise", NULL, 0, NULL);
	cmd = g_strdup_printf (
		"open \"%s\" alias noise",
		name
	); 
	ret = mciSendString (cmd, NULL, 0, NULL);
	g_free (cmd);
	if (!ret) ret = mciSendString("play noise", NULL, 0, NULL);
	return ret;
#else 
	char cmd [1024], * exe = NULL;
	int ret = 0;
	GError * err = NULL;

	if (g_str_has_suffix (name, ".mp3")) exe = get_configuration ()->mp3cmd;
	else if (g_str_has_suffix (name, ".wav")) exe = get_configuration ()->wavcmd;
	else if (g_str_has_suffix (name, ".mid")) exe = get_configuration ()->midcmd;
	else fprintf (stderr, "atm_execute_noise: I don't know how to play '%s'\n", name);
	if (exe) {
		g_snprintf (cmd, 1024, "%s \"%s\"", exe, name);
		ret = !g_spawn_command_line_async (cmd, &err);
		if (err) fprintf (stderr, "atm_execute_noise: error spawning command (%s)\n", err->message);
	}
	return ret;
#endif // HAVE_WINDOWS

}
