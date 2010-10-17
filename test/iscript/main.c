/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* main.c:                                                                 *
*                2005 Shlykov Vasiliy ( vash@zmail.ru )                   *
*                                                                         *
*                Simple test unit for iscript.                            *
*                Usage: basic test.bas [external values]                  *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iscript.h>

#define BUF_SIZE 4096

char buf[BUF_SIZE];
char obuf[BUF_SIZE];

void err_handler (int line , int code, const char* msg, void* data)
{
  printf ("ERROR:%d: %s.\n", line, msg);
}

void func_0_arg (IScript* is, int argc, const char** argv, void** ret)
{
  fprintf (stderr, "FUNC 0 ARG\n");
}

void func_1_arg (IScript* is, int argc, const char** argv, void** ret)
{
  fprintf (stderr, "FUNC 1 ARG: '%s'\n", argv[0]);
}

void func_2_arg (IScript* is, int argc, const char** argv, void** ret)
{
  fprintf (stderr, "FUNC 2 ARG: '%s', '%s'\n", argv[0], argv[1]);
}

IScriptExtFuncInfo func_0_arg_info =
{
	"MESSAGEBOX0",
	0,
	&func_0_arg,
	0
};

IScriptExtFuncInfo func_1_arg_info =
{
	"MESSAGEBOX1",
	1,
	&func_1_arg,
	0
};

IScriptExtFuncInfo func_2_arg_info =
{
	"MESSAGEBOX2",
	2,
	&func_2_arg,
	0
};

int main (int argc, char** argv)
{
  FILE* in;
  int c = 0,
      vc = 1,
      ret;
  char  varname[10];
  IScript* is;
  size_t i = 0;

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s basic-file [$arguments]\n", argv[0]);
      exit (1);
    }

  in = fopen (argv[1], "r");

  if (!in)
    {
      perror ("Couldn't open file");
      exit (2);
    }

  for (; i < BUF_SIZE; ++i)
    {
      c = fgetc (in);

      if (c != EOF)
        {
          buf[i] = c;
        }
      else
        {
          buf[i] = '\0';
          break;
        }
    }

  is = iscript_new ();
  iscript_init (is);
  iscript_set_err_handler (is, &err_handler, NULL);

  for (i = 2; i < argc; i++)
    {
      sprintf (varname, "%d", vc++);
      iscript_ext_var_add (is, varname, argv[i]);
    }

  iscript_ext_func_add (is, &func_0_arg_info);
  iscript_ext_func_add (is, &func_1_arg_info);
  iscript_ext_func_add (is, &func_2_arg_info);

  ret = iscript_run (is, buf, obuf, BUF_SIZE);
  iscript_free (is);

  printf ("%s", obuf);

  fclose (in);

  exit (!ret);
}
