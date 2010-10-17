/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* directories.c:                                                       	  *
*                2004 Calvin Ellis  ( kyndig@mudmagic.com )               *
*                                                                         *
***************************************************************************/
/************************************************************************** 
* LIBGIMP - The GIMP Library						  *
* Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball		  *
*									  *
* Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>				  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdio.h>
#include <glib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib/gstdio.h>
#include "directories.h"

/* Locals */
#define MUDCFG_HOME_DIR         ".mudmagic"
static void utils_path_runtime_fix (gchar **path);
static gchar * utils_env_get_dir (const gchar *mudmagic_env_name, const gchar *env_dir);

/**
 * mudmagic_directory:
 *
 * Returns the user-specific MudMagic settings directory. If the
 * environment variable MUDMAGIC_DIRECTORY exists, it is used. If it is
 * an absolute path, it is used as is.  If it is a relative path, it
 * is taken to be a subdirectory of the home directory. If it is a
 * relative path, and no home directory can be determined, it is taken
 * to be a subdirectory of mudmagic_data_directory().
 *
 * Default directory is: ~/.mudmagic on Unix/Mac, and users specific 
 * Application Settings directory on windows.
 *
 * The usual case is that no MUDMAGIC_DIRECTORY environment variable
 * exists, and then we use the MUDCFG_HOME_DIR subdirectory of the home
 * directory. If no home directory exists, we use a per-user
 * subdirectory of mudmagic_data_directory().  In any case, we always
 * return some non-empty string, whether it corresponds to an existing
 * directory or not.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free(). The returned string is in the encoding used
 * for filenames by the system, which isn't necessarily UTF-8 (never
 * is on Windows).
 *
 * Returns: The user-specific MudMagic settings directory.
 **/
const gchar * mudmagic_directory (void)
{
  static gchar *mudmagic_dir = NULL;

  const gchar  *env_mudmagic_dir;
  const gchar  *home_dir;

  if (mudmagic_dir)
    return mudmagic_dir;

  env_mudmagic_dir = g_getenv ("MUDMAGIC_DIRECTORY");
  home_dir     = g_get_home_dir ();

  if (env_mudmagic_dir)
  {
      if (g_path_is_absolute (env_mudmagic_dir))
        {
          mudmagic_dir = g_strdup (env_mudmagic_dir);
        }
      else
	{
	  if (home_dir)
	    {
	      mudmagic_dir = g_build_filename (home_dir,
                                           env_mudmagic_dir,
                                           NULL);
	    }
	  else
	    {
	      mudmagic_dir = g_build_filename (mudmagic_data_directory (),
                                           env_mudmagic_dir, NULL);
	    }
	}
    }
  else
    {
      if (home_dir)
	{
	  mudmagic_dir = g_build_filename (home_dir, MUDCFG_HOME_DIR, NULL);
	}
      else
	{
	  gchar *user_name = g_strdup (g_get_user_name ());

#ifdef HAVE_WINDOWS
	  gchar *p = user_name;

	  while (*p)
	    {
	      /* Replace funny characters in the user name with an
	       * underscore. The code below also replaces some
	       * characters that in fact are legal in file names, but
	       * who cares, as long as the definitely illegal ones are
	       * caught.
	       */
	      if (!isalnum (*p) && !strchr ("-.,@=", *p))
		*p = '_';
	      p++;
	    }
#endif

#ifndef HAVE_WINDOWS
	  g_message ("warning: no home directory.");
#endif
	  mudmagic_dir = g_build_filename (mudmagic_data_directory (),
				       MUDCFG_HOME_DIR,
                                       NULL);
	  g_free (user_name);
	}
    }

  return mudmagic_dir;
}

#ifdef HAVE_WINDOWS
gchar *
mudmagic_toplevel_directory (void)
{
  /* Figure it out from the executable name */
  static gchar *toplevel = NULL;

  gchar         filename[MAX_PATH];
  gchar        *sep1, *sep2;

  if (toplevel)
    return toplevel;

  if (GetModuleFileName (NULL, filename, sizeof (filename)) == 0)
    g_error ("GetModuleFilename failed");

  /* If the executable file name is of the format
   * <foobar>\bin\*.exe or
   * <foobar>\lib\gimp\GIMP_MAJOR_VERSION.GIMP_MINOR_VERSION\plug-ins\*.exe,
   * use <foobar>. Otherwise, use the directory where the
   * executable is.
   */

  sep1 = strrchr (filename, '\\');

  *sep1 = '\0';

  sep2 = strrchr (filename, '\\');

  if (sep2 != NULL)
    {
      if (g_ascii_strcasecmp (sep2 + 1, "bin") == 0)
	{
	  *sep2 = '\0';
	}
    }

  toplevel = g_strdup (filename);

  return toplevel;
}
#endif

/**
 * mudmagic_data_directory:
 *
 * Returns the top directory for MudMagic data. If the environment
 * variable MUDMAGIC_DATADIR exists, that is used.  It should be an
 * absolute pathname.  Otherwise, on Unix the compile-time defined
 * directory is used.  On Win32, the installation directory as deduced
 * from the executable's name is used.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free(). The returned string is in the encoding used
 * for filenames by the system, which isn't necessarily UTF-8 (never
 * is on Windows).
 *
 * Returns: The top directory for MudMagic data.
 **/
gchar * 
mudmagic_data_directory (void)
{
  static gchar *mudmagic_data_dir = NULL;

  if (!mudmagic_data_dir)
    mudmagic_data_dir = utils_env_get_dir ("MUDMAGIC_DATADIR", DATADIR);

  return mudmagic_data_dir;
}

/**
 * utils_env_get_dir
 * @mudmagic_env_name is the available ENV variable passed through a script
 * @env_dir is the compil-time directory
 * looks for the mudmagic_env first and returns that path if exists, or the
 * compile-time directory if not
 */
static gchar * utils_env_get_dir (const gchar *mudmagic_env_name,
                                  const gchar *env_dir)
{
  const gchar *env;

  env = g_getenv (mudmagic_env_name);
 
  if (env)
  {
      if (! g_path_is_absolute (env))
        g_error ("%s environment variable should be an absolute path.",
                 mudmagic_env_name);
 
      return g_strdup (env);
    }
  else
    {
      gchar *retval = g_strdup (env_dir);
      utils_path_runtime_fix (&retval);
      return retval;
    }
}

/**
 * utils_path_runtime_fix:
 * @path: A pointer to a string (allocated with g_malloc) that is
 *        (or could be) a pathname.
 *
 * On Windows, this function checks if the string pointed to by @path
 * starts with the compile-time prefix, and in that case, replaces the
 * prefix with the run-time one.  @path should be a pointer to a
 * dynamically allocated (with g_malloc, g_strconcat, etc) string. If
 * the replacement takes place, the original string is deallocated,
 * and *@path is replaced with a pointer to a new string with the
 * run-time prefix spliced in.
 *
 * On Unix, does nothing.
 */
static void
utils_path_runtime_fix (gchar **path)
{
#if defined (HAVE_WINDOWS) && defined (PREFIX)
  gchar *p;

  /* Yes, I do mean forward slashes below */
  if (strncmp (*path, PREFIX "/", strlen (PREFIX "/")) == 0)
    {
      /* This is a compile-time entry. Replace the path with the
       * real one on this machine.
       */
      p = *path;
      *path = g_strconcat (mudmagic_toplevel_directory (),
                           "\\",
                           *path + strlen (PREFIX "/"),
                           NULL);
      g_free (p);
    }
  /* Replace forward slashes with backslashes, just for
   * completeness */  
  p = *path;
  while ((p = strchr (p, '/')) != NULL)
    {
      *p = '\\';
      p++;
    }
#elif defined (HAVE_WINDOWS)
  /* without defining PREFIX do something useful too */
  gchar *p = *path;
  if (!g_path_is_absolute (p))
  {
      *path = g_build_filename (mudmagic_toplevel_directory (),
                                *path, NULL);
      g_free (p);
    }
#endif
}

/**
 * removes directory recursively. 
 */
void mud_dir_remove (const char * path) {
	GError * gerr = NULL;
	GDir * d = g_dir_open (path, 0, &gerr);
	const char * n = NULL;

	if (gerr) {
		fprintf (stderr, "%s\n", gerr->message);
		g_error_free (gerr);
	} else {
		struct stat buf;
		while ((n = g_dir_read_name (d))) {
			gchar * x = g_build_path (G_DIR_SEPARATOR_S, path, n, NULL);
			if (g_lstat (x, &buf)) {
				fprintf (stderr, "lstat failed on '%s'\n", x);
			} else {
				if (S_ISDIR (buf.st_mode)) mud_dir_remove (x);
				else if (g_remove (x)) fprintf (stderr, "unable to remove file '%s'\n", x);
			}
			g_free (x);
		}
		g_dir_close (d);
	}
	if (g_rmdir (path)) fprintf (stderr, "unable to remove directory '%s'\n", path);
}

