/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* variables.c:                                                            *
*                2005 Tomas Mecir  ( kmuddy@kmuddy.net )                  *
*                                                                         *
***************************************************************************/
/**************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/    
#include "mudmagic.h"
    
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "gauges.h"
#include "statusvars.h"

// VARLIST manipulation
static gint mystrcmp(const gchar * a, const gchar * b, gpointer data)
{
  return strcmp(a, b);
}

VARLIST *varlist_new (SESSION_STATE *s)
{
  VARLIST * v = g_new0(VARLIST, 1);
  v->tree =
      g_tree_new_full((GCompareDataFunc) mystrcmp, NULL, g_free,
          (GDestroyNotify) variable_destroy);
  v->sess = s;
  return v;
}

void varlist_destroy(VARLIST * v) 
{
  g_tree_destroy(v->tree);
  v->tree = 0;
} 

gchar * varlist_get_value(VARLIST * v, gchar * name) 
{
  VARIABLE *var = g_tree_lookup(v->tree, name);
  if (var)
    return var->value;
  return NULL;
}

int varlist_get_int_value(VARLIST * v, gchar * name)
{
  gchar *str = varlist_get_value (v, name);
  return str ? atoi (str) : 0;
}

void varlist_remove_value(VARLIST * v, gchar * name) 
{
  if (g_tree_lookup(v->tree, name))
    g_tree_remove(v->tree, (const gchar *) name);
} 

gboolean varlist_exists(VARLIST * v, gchar * name) 
{
  VARIABLE * var = g_tree_lookup(v->tree, name);
  return var ? TRUE : FALSE;
}

VARIABLE * varlist_get_variable(VARLIST * v, gchar * name) 
{
  VARIABLE * var = g_tree_lookup(v->tree, name);
  return var;
}

void varlist_set_value(VARLIST * v, gchar * name, gchar * value) 
{
  // remove old value
  varlist_remove_value(v, name);
  
  // set new value
  VARIABLE * var = variable_new(name);
  variable_set_value(var, value);
  g_tree_insert(v->tree, strdup(name), var);
  
  // update status variables and gauges, if need be
  if (!v->sess) return;
  svlist_handle_variable_change ((SVLIST *) v->sess->svlist, name);
  gaugelist_handle_variable_change ((GAUGELIST *) v->sess->gaugelist, name);
} 

void varlist_load(VARLIST * v, FILE * f) 
{
  char buff[1025], buff2[1025];
  while (!feof(f)) 
  {
    if ((fgets(buff, 1024, f) != NULL)
    && (fgets(buff2, 1024, f) != NULL)) 
    {
      int len = strlen(buff);
      int len2 = strlen(buff2);
      if (len && len2) 
      {
        // get rid of trailing newlines, if any
        if (buff[len - 1] == '\n') 
        {
          buff[len - 1] = '\0';
          --len;
        }
        if (buff2[len2 - 1] == '\n') {
          buff2[len2 - 1] = '\0';
          --len2;
        }
        // create a variable, add it to the list
        varlist_set_value(v, buff, buff2);
      }
    }
  }
}

gboolean save_entry(gpointer * key, gpointer * value, gpointer * data) 
{
  gchar * k = (gchar *) key;
  gchar * v = ((VARIABLE *) value)->value;
  fprintf((FILE *) data, "%s\n", k);
  fprintf((FILE *) data, "%s\n", v);
  return FALSE;   // FALSE means, continue traversing
}

void varlist_save(VARLIST * v, FILE * f) 
{
  g_tree_foreach(v->tree, (GTraverseFunc) save_entry, f);
} 

gchar * variables_expand(VARLIST * v, gchar * string, int len) 
// expand variables, return them in a new string
{
  gchar * varname = g_malloc0(len);
  int vl = 0;
  
      // compute length of resulting string
  int i, l = 0;
  char state = 't';
  for (i = 0; i < len; ++i) {
    switch (state) {
    case 't':{
        if (string[i] == '$')
          state = 's';
        
        else
          ++l;
      }
      break;
    case 's':{
        if (isalnum(string[i])) {
          state = 'v';
          varname[vl++] = string[i];
        }
        
        else {
          state = 't';
          l += 2;
        }
      }
      break;
    case 'v':{
        
            // not part of the word, or at the end of a string - end variable
            if ((!isalnum(string[i]))
          || (i == len - 1)) {
          
              // add length of expanded variable
              if (isalnum(string[i])
            || (i == len - 1))
            varname[vl++] = string[i];
          varname[vl] = '\0';
          gchar * val =
              varlist_get_value(v, varname);
          if (val)
            l += strlen(val);
          
          else
            l += 1 + strlen(varname); //not found - $blah is copied
          vl = 0;
          state =
              (string[i] == '$') ? 's' : 't';
        } else {
          varname[vl++] = string[i];
        }
      }
      break;
    };
  }
  if (state == 's') // trailing $
    ++l;
  
      // allocate memory for new string
      gchar * ret = g_malloc0(l + 1);
  
      // generate string
      // this is a loop similar to the one above, only that we generate,
      // instead of computing length
      state = 't';
  l = 0;
  for (i = 0; i < len; ++i) {
    switch (state) {
    case 't':{
        if (string[i] == '$')
          state = 's';
        
        else
          ret[l++] = string[i];
      }
      break;
    case 's':{
        if (isalnum(string[i])) {
          state = 'v';
          vl = 0;
          varname[vl++] = string[i];
        }
        
        else {
          state = 't';
          ret[l++] = '$';
          ret[l++] = string[i];
        }
      }
      break;
    case 'v':{
        
            // not part of the word, or at the end of a string - end variable
            if ((!isalnum(string[i]))
          || (i == len - 1)) {
          
              // add length of expanded variable
              if (isalnum(string[i])
            || (i == len - 1))
            varname[vl++] = string[i];
          varname[vl] = '\0';
          gchar * val =
              varlist_get_value(v, varname);
          if (val) {
            
                // add expanded variable
            int ll = strlen(val);
            g_strlcpy(ret + l, val,
                 ll + 1);
            l += ll;
          }
          
          else
             {
            
                //not found - $name is copied
            int ll = strlen(varname);
            ret[l++] = '$';
            g_strlcpy(ret + l,
                 varname,
                 ll + 1);
            l += ll;
            } vl = 0;
          state =
              (string[i] == '$') ? 's' : 't';
        } else {
          varname[vl++] = string[i];
        }
      }
      break;
    };
  }
  if (state == 's') // trailing $
    ret[l++] = '$';
  ret[l] = '\0';
  
      // return resulting string
      g_free(varname);
  return ret;
}


// VARIABLE manipulation
VARIABLE * variable_new(gchar * name) 
{
  VARIABLE * var = g_new0(VARIABLE, 1);
  var->value = 0;
  var->name = strdup(name);
  return var;
}

void variable_destroy(VARIABLE * var) 
{
  if (!var)
    return;
  if (var->name)
    g_free(var->name);
  if (var->value)
    g_free(var->value);
  g_free(var);
}

void variable_set_name(VARIABLE * var, gchar * name) 
{
  if (var->name)
    g_free(var->name);
  var->name = g_strdup(name);
}

void variable_set_value(VARIABLE * var, gchar * value) 
{
  if (var->value)
    g_free(var->value);
  var->value = g_strdup(value);
}

gchar * variable_name(VARIABLE * var) 
{
  return var->name;
}

gchar * variable_value(VARIABLE * var) 
{
  return var->value ? var->value : "";
}
