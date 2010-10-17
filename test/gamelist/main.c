#include <libxml/parser.h>
//#include <mudmagic.h>
#include <gamelist.h>

void
dump_mud (const MUDInfo* mud)
{
  printf ("Title:\t%s\n", mud->title);
  printf ("Link:\t%s\n", mud->link);
  printf ("Author:\t%s\n", mud->author);
  printf ("Comments:\t%s\n", mud->comments);
  printf ("Pub Date:\t%s\n", mud->pub_date);
  printf ("Intro:\t%s\n", mud->game_intro);
  printf ("Host:\t%s\n", mud->game_host);
  printf ("Port:\t%s\n", mud->game_port);
  printf ("IP:\t%s\n", mud->game_ip);
  printf ("Genre:\t%s\n", mud->game_genre);
  printf ("Base:\t%s\n", mud->game_base);
  printf ("Theme:\t%s\n", mud->game_theme);
  printf ("Description:\t%s\n", mud->description);
}

void
load_mud (GameList* gl, const char* title)
{
  MUDInfo* mud = gamelist_mud_get (gl, title);

  if (mud == NULL)
    {
      printf ("Item %s not found\n", title);
    }
  else
    {
      gboolean res = gamelist_mud_load (mud);

      if (!res)
        {
          printf ("Cannot load %s\n", title);
        }
      else
        {
          printf ("================== MUD '%s' ===================\n", mud->title);
          dump_mud (mud);
        }
    }

}

int main(int argc, char** argv)
{
  GameList* gl;
  gboolean  res;
  char*     test_mud;

  if (argc < 2)
    {
      printf ("Usage: %s file\n", argv[0]);
      exit (1);
    }

  LIBXML_TEST_VERSION;

  gl = gamelist_new ();

  printf ("Loading %s", argv[1]);

  res = gamelist_load (gl, argv[1]);

  if (!res)
    {
      gchar* err = mud_error_list_join (gl->errors, "\n");
      fprintf (stderr, "Errors during loading:\n %s\n", err);
      g_free (err);
    }
  else
    {
	  printf ("Title : %s\n",	gl->title);
	  printf ("Description : %s\n", gl->description);
	  printf ("Link : %s\n",	gl->link);
	  printf ("Last Build Date : %s\n", gl->last_build_date);
	  printf ("Generator : %s\n", gl->generator);
    }

  if (argc >= 3)
    test_mud = argv[2];
  else
    {
      printf ("Input MUD to test: ");
      scanf ("%s", test_mud);
    }

  load_mud (gl, test_mud);
 
  gamelist_free (gl);
  return 0;
}
