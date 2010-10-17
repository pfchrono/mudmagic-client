/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* data.c:                                                                 *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*                2005-2006  Tomas Mecir   ( kmuddy@kmuddy.net )           *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <mudmagic.h>
#include <interface.h>
#include <cmdentry.h>
#include <protocols.h>
#include <alias_triggers.h>
#include <log.h>
#include <module.h>
#include <directories.h>

#define HISTORY_LEN 20

extern CONFIGURATION *config;

#define RED 65536
#define GREEN 256
#define BLUE 1

static gint bold_colors[8] = {
  128 * RED + 128 * GREEN + 128 * BLUE, // black
  255 * RED,        // red
  255 * GREEN,        // green
  255 * RED + 255 * GREEN,    // yellow
  255 * BLUE,       // blue
  255 * RED + 255 * BLUE,     // magenta
  255 * GREEN + 255 * BLUE,   // cyan
  255 * RED + 255 * GREEN + 255 * BLUE  // white
};
static gint normal_colors[8] = {
  0,          // black
  128 * RED,        // red
  128 * GREEN,        // green
  128 * RED + 128 * GREEN,    // yellow
  128 * BLUE,       // blue
  128 * RED + 128 * BLUE,     // magenta
  128 * GREEN + 128 * BLUE,   // cyan
  192 * RED + 192 * GREEN + 192 * BLUE  // white
};

/* length of a string array
 * equivalent to g_strv_length, which doesn't exist in Gtk 2.4
 */
int strv_length(gchar ** str_array)
{
  int l = 0;
  while (*(str_array++) != NULL)
    ++l;
  return l;
}

void initialize_ansi(SESSION_STATE * session)
{
  session->ansi.bold = FALSE;
  session->ansi.boldfont = FALSE;
  session->ansi.italic = FALSE;
  session->ansi.underline = FALSE;
  session->ansi.blink = FALSE;
  session->ansi.reverse = FALSE;
  session->ansi.fg = 7;
  session->ansi.bg = 0;
  session->ansi.fgcolor = normal_colors[7];
  session->ansi.bgcolor = normal_colors[0];
  session->ansi.size = 12;
  session->ansi.font = 0;
}

/* generate a color code, in the form X#rrggbb, there X is F or B,
 * depending on whether it's a foreground or background color
 */
void colorCode(char *s, unsigned int num, gboolean fg)
{
  s[0] = fg ? 'F' : 'B';
  s[1] = '#';
  int b = num % 256;
  num = num / 256;
  int g = num % 256;
  num = num / 256;
  int r = num % 256;
  s[2] = r / 16 + '0';
  s[3] = r % 16 + '0';
  s[4] = g / 16 + '0';
  s[5] = g % 16 + '0';
  s[6] = b / 16 + '0';
  s[7] = b % 16 + '0';
  int i;
  for (i = 2; i <= 7; i++)
    if (s[i] > '9')
      s[i] = 'A' + (s[i] - 10 - '0');
  s[8] = '\0';
}

GtkTextTag *get_fg_color_tag(GtkTextBuffer * buffer, int color)
{
  /* "foreground" is property name for color */
  char s[10];
  colorCode(s, color, TRUE);
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, s);
  if (!tag) {
    /* tag not found - need to create it */
    tag = gtk_text_tag_new(s);
    /* first character is F/B (foreground/background), hence we need to */
    /* get the color name from the second char, hence that s+1 */
//    g_object_set(tag, "foreground", s + 1, NULL);
    g_object_set(tag, "foreground", g_strdup(s + 1), NULL);
    gtk_text_tag_table_add(table, tag);
  }
  return tag;
}

GtkTextTag *get_bg_color_tag(GtkTextBuffer * buffer, int color)
{
  /* "background" is property name for color */
  char s[10];
  colorCode(s, color, FALSE);
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, s);
  if (!tag) {
    // tag not found - need to create it
    tag = gtk_text_tag_new(s);
    // first character is F/B (foreground/background), hence we need to
    // get the color name from the second char, hence that s+1
 //   g_object_set(tag, "background", s + 1, NULL);
    g_object_set(tag, "background", g_strdup (s + 1), NULL);
    gtk_text_tag_table_add(table, tag);
  }
  return tag;
}

void linkmenu_activate (GtkMenuItem *item, gpointer sess)
{
  SESSION_STATE *session = (SESSION_STATE *) sess;
  gchar *command = (gchar *) g_object_get_data (G_OBJECT (item), "command");
  send_command (session, command, strlen (command));
}

gboolean on_tag_click(GtkTextTag * texttag, GObject * obj,
          GdkEvent * event, GtkTextIter * iter, gpointer sess)
{
  if ((event->type != GDK_BUTTON_PRESS) && (event->type != GDK_BUTTON_RELEASE))
    return FALSE;

  // get object data
  SESSION_STATE *session = (SESSION_STATE *) sess;
  if (session == NULL)
    return FALSE;

  char *action = 0, *type = 0, *menustr = 0;
  gboolean menu = FALSE;
  action = g_object_get_data(G_OBJECT(texttag), "action");
  type = g_object_get_data(G_OBJECT(texttag), "type");
  menustr = g_object_get_data(G_OBJECT(texttag), "menu");
  if (!strcmp (menustr, "yes")) menu = TRUE;

  // it must be a link
  if ((!action) || (!type))
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS) {
    // Display the MXP menu if needed. No other action here.
    
    GdkEventButton *eb = (GdkEventButton *) event;
    if (eb->button != 3) // only for right button
      return FALSE;
    // Display the menu if this is a menu link. Do nothing otherwise.
    if (!menu) return FALSE;
    // TODO: will the objects get free-ed correctly when not needed ?
    GtkWidget *m = gtk_menu_new ();
    gchar **actions = g_strsplit (action, "|", 0);
    gchar **a = actions;
    while (*a) {
      GtkWidget *item = gtk_menu_item_new_with_label (*a);
      gtk_menu_append (GTK_MENU (m), item);
      g_object_set_data (G_OBJECT (item), "command", g_strdup (*a));
      g_signal_connect (G_OBJECT (item), "activate",
          GTK_SIGNAL_FUNC (linkmenu_activate), sess);
      gtk_widget_show (item);
      ++a;
    }
    g_strfreev (actions);
    gtk_menu_popup (GTK_MENU (m), NULL, NULL, NULL, NULL,
        eb->button, eb->time);
    return TRUE;  // don't display the default menu
  }
  
  // if we are here, the event is GDK_BUTTON_RELEASE

  // URL link ?
  if (type && strcmp(type, "url") == 0) {
    //in the future, let them choose which browser
    //they have. For now: IE or Mozilla
    int ret = try_to_execute_url( WEB_BROWSER, action );
    if( !ret )
      interface_display_message("Unable to visit with current web browser\n" );
    return FALSE;
  }
  
  // SEND link ?
  if (type && strcmp(type, "command") == 0) {
    // send the command
    // send the command, first one if it's a menu
    char *act = g_strdup (action);
    if (menu) {  // replace first "|" with " "
      char *pos = strchr (act, '|');
      if (pos) act[pos - act] = '\0';
    }
    send_command (session, act, strlen(act));
    g_free (act);
    return FALSE;
  }

  // image map ?
  char *imagemap = g_object_get_data(G_OBJECT(texttag), "imagemap");
  if (imagemap) {
    // it's an image map - send the location ...
    GdkPixbuf *pixbuf = gtk_text_iter_get_pixbuf (iter);
    if (!pixbuf) return FALSE;
    GdkRectangle rect;
    
    GtkTextView *view = GTK_TEXT_VIEW (interface_get_widget
        (session->tab, "output1"));
    gtk_text_view_get_iter_location (view, iter, &rect);
    int x = rect.x;
    int y = rect.y;

    int ex = ((GdkEventButton *) event)->x;
    int ey = ((GdkEventButton *) event)->y;
    gchar *cmd = g_strdup_printf ("%s?%d,%d", imagemap, ex-x, ey-y);
    send_command (session, cmd, strlen (cmd));
    g_free (cmd);
    return FALSE;
  }
  
  // none of the above - do nothing
  return FALSE;
}


void internal_output_add_text(SESSION_STATE *session, GtkTextView *output,
    gchar * data, gsize len, TEXT_ATTR * ansi)
{
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  gint offset;
  GtkTextTagTable *tagtable;

  g_return_if_fail(session != NULL);

//  gsize read, written;
//  GError *error = NULL;
//  gchar *new_data;

  g_return_if_fail(data != NULL);

  buffer = gtk_text_view_get_buffer (output);

  gtk_text_buffer_get_end_iter(buffer, &end);
  offset = gtk_text_iter_get_offset(&end);

/*
  Conversion sometimes causes weird crashes - disabled
  new_data =
      g_convert(data, len, "UTF-8", "ISO-8859-1", &read, &written,
          &error);
  gtk_text_buffer_insert(buffer, &end, new_data, -1);

  g_free(new_data);
*/
  gtk_text_buffer_insert( buffer, &end, data, len );

  if (ansi == NULL)
    return;   // nothing to do

  // apply some tags on inserted text 
  gtk_text_buffer_get_iter_at_offset(buffer, &start, offset);
  gtk_text_buffer_get_end_iter(buffer, &end);

  tagtable = gtk_text_buffer_get_tag_table(buffer);

  if (ansi->boldfont) {
    gtk_text_buffer_apply_tag_by_name(buffer, "bold", &start,
              &end);
  }
  if (ansi->italic) {
    gtk_text_buffer_apply_tag_by_name(buffer, "italic", &start,
              &end);
  }
  if (ansi->underline) {
    gtk_text_buffer_apply_tag_by_name(buffer, "underline",
              &start, &end);
  }

  if (session->linkCmd) {
    // create the link-tag
    char *name = 0;
    static char tmpln[] = "AAAAAAAAAA-tmplink";
    if (session->linkName)
      name = g_strdup(session->linkName);
    else {
      int i = 0;
      name = strdup(tmpln);
      while (i < 10) {  // generate name for next unnamed link
        tmpln[i]++;
        if (tmpln[i] > 'Z') {
          tmpln[i] = 'A';
          i++;
        } else
          break;
      }
    }
    GtkTextTag *t = gtk_text_tag_new(name);
    g_object_set_data(G_OBJECT(t), "linkname",
          g_strdup(session->linkName));
    g_object_set_data(G_OBJECT(t), "action",
          g_strdup(session->linkCmd));
    g_object_set_data(G_OBJECT(t), "type",
          session->isSendLink ? "command" : "url");
    g_object_set_data(G_OBJECT(t), "menu",
          session->isMenuLink ? "yes" : "no");

    GtkTextTagTable *table =
        gtk_text_buffer_get_tag_table(buffer);
    gtk_text_tag_table_add(table, t);
    gtk_text_buffer_apply_tag(buffer, t, &start, &end);
    g_signal_connect(G_OBJECT(t), "event",
         G_CALLBACK(on_tag_click), session);
    g_free(name);
  }

  GtkTextTag *tag = get_fg_color_tag(buffer, ansi->fgcolor);
  if (tag) gtk_text_buffer_apply_tag(buffer, tag, &start, &end);

  if (ansi->bgcolor != 0) {
    GtkTextTag *tag = get_bg_color_tag(buffer, ansi->bgcolor);
    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
  }
}

void internal_update_ansi_state(TEXT_ATTR * ansi, gchar * data, gsize len)
{
  gsize i;
  gint code = 0;
  // we'll start parsing after ESC[
  for (i = 2; i < len; i++) {
    if (g_ascii_isdigit(data[i])) {
      code = code * 10 + g_ascii_digit_value(data[i]);
    } else {
      switch (code) { // this can be optimized 
      case 0:{  // reset all
          ansi->bold = FALSE;
          ansi->italic = FALSE;
          ansi->underline = FALSE;
          ansi->blink = FALSE;
          ansi->reverse = FALSE;
          ansi->bg = 0;
          ansi->fg = 7;
          ansi->bgcolor = normal_colors[0];
          ansi->fgcolor = normal_colors[7];
        }
        break;
      case 1:{
          ansi->bold = TRUE;
          ansi->fgcolor =
              bold_colors[ansi->fg];
        }
        break;
      case 2:{
          //DIM handled as BOLD OFF
          ansi->bold = FALSE;
          ansi->fgcolor =
              normal_colors[ansi->fg];
        }
        break;
      case 3:{
          ansi->italic = TRUE;
        }
        break;
      case 4:{
          ansi->underline = TRUE;
        }
        break;
      case 5:{
          ansi->blink = TRUE;
        }
        break;
      case 6:{  //RAPID BLINK - same as regular blink
          ansi->blink = TRUE;
        }
        break;
      case 7:{
          ansi->reverse = TRUE;
        }
        break;
      case 21:{ //DOUBLE UNDERLINE - as single underline
          ansi->underline = TRUE;
        }
        break;
      case 22:{
          ansi->bold = FALSE;
          ansi->fgcolor =
              normal_colors[ansi->fg];
        }
        break;
      case 23:{
          ansi->italic = FALSE;
        }
        break;
      case 24:{
          ansi->underline = FALSE;
        }
        break;
      case 25:{
          ansi->blink = FALSE;
        }
        break;
      case 27:{
          ansi->reverse = FALSE;
        }
        break;
      case 30:
      case 31:
      case 32:
      case 33:
      case 34:
      case 35:
      case 36:
      case 37:{
          ansi->fg = code - 30;
          ansi->fgcolor = ansi->bold ?
              bold_colors[ansi->
              fg] :
              normal_colors[ansi->fg];
        }
        break;
      case 39:{
          ansi->fg = 7;
          ansi->fgcolor = ansi->bold ?
              bold_colors[ansi->
              fg] :
              normal_colors[ansi->fg];
        }
        break;
      case 40:
      case 41:
      case 42:
      case 43:
      case 44:
      case 45:
      case 46:
      case 47:{
          ansi->bg = code - 40;
          ansi->bgcolor =
              normal_colors[ansi->bg];
        }
        break;
      case 49:{
          ansi->bg = 0;
          ansi->bgcolor =
              normal_colors[ansi->bg];
        }
        break;
      }
      code = 0;
    }
  }
}

void output_scroll_to_bottom(GtkWidget * tab)
{
  GtkTextView *out1, *out2;
  GtkTextIter iter;

  while (gtk_events_pending())
    gtk_main_iteration();

  // scroll to bottom
  out1 = GTK_TEXT_VIEW(interface_get_widget(tab, "output1"));
  out2 = GTK_TEXT_VIEW(interface_get_widget(tab, "output2"));
  if (!(GTK_WIDGET_VISIBLE(out2))) {
    //only scroll if the second output is not visible
    //that means that we're scrolled to the very bottom ...
    gtk_text_buffer_get_end_iter(gtk_text_view_get_buffer
               (out1), &iter);
    gtk_text_view_scroll_to_iter(out1, &iter, 0.0, TRUE, 0.0,
               1.0);
  }
  //second output ALWAYS gets scrolled to the bottom
  gtk_text_buffer_get_end_iter(gtk_text_view_get_buffer(out2),
             &iter);
  gtk_text_view_scroll_to_iter(out2, &iter, 0.0, TRUE, 0.0, 1.0);
}

void interface_output_append(GtkWidget * tab, gchar * data, gsize len)
{
  SESSION_STATE *session;
  gsize i, j, pos;
  GtkTextView *output;

  g_return_if_fail(tab != NULL && data != NULL);
  session = g_object_get_data(G_OBJECT(tab), "session");
  g_return_if_fail(session != NULL);
  
  // change output window if need be
  output = owindowlist_active_textview (session->windowlist);
  
  // default output is the main window
  if (!output)
    output = GTK_TEXT_VIEW (interface_get_widget(tab, "output1"));
  
  for (i = pos = 0; i < len; i++) {
    if (data[i] == 27) {
      internal_output_add_text(session, output, data + pos, i - pos,
             &session->ansi);
      // looking for the end of ansi code 
      for (j = i; j < len; j++) {
        if (data[j] == 'm')
          break;
      }
      // telnet_process should not let any incopleted ansi code to pass 
      if (j == len) {
        g_warning
            ("incomplete ANSI code found in processed data.");
        return;
      }
      // update ANSI state
      internal_update_ansi_state(&session->ansi,
               data + i, j - i + 1);
      i = j;
      pos = j + 1;
    }
  }
  internal_output_add_text(session, output, data + pos, len - pos,
         &session->ansi);
}

void interface_image_add(GtkWidget * tab, GtkTextIter * iter,
       GdkPixbuf * pixbuf)
{
  if (pixbuf == NULL)
    return;
  GtkTextView *out1 = GTK_TEXT_VIEW(interface_get_widget(tab, "output1"));
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(out1));

  GtkTextIter it;
  if (iter != NULL)
    it = *iter;
  else
    gtk_text_buffer_get_end_iter(buffer, &it);

  gtk_text_buffer_insert_pixbuf(buffer, &it, pixbuf);
  
  // append tag name, if needed
  SESSION_STATE *session = g_object_get_data(G_OBJECT(tab), "session");
  if (!session->imagemapName) return;
  
  GtkTextTag *t = gtk_text_tag_new (session->imagemapName);
  g_object_set_data(G_OBJECT(t), "imagemap",
                    g_strdup(session->imagemapName));

  GtkTextTagTable *table =
      gtk_text_buffer_get_tag_table(buffer);
  gtk_text_tag_table_add(table, t);
  gtk_text_buffer_apply_tag(buffer, t, &it, &it);
  g_signal_connect(G_OBJECT(t), "event",
                   G_CALLBACK(on_tag_click), session);
}

void interface_output_append_line (GtkWidget * tab)
{
  // the horizontal line currently consists of 40 spaces with strikethrough on

  GtkTextView *out1 = GTK_TEXT_VIEW(interface_get_widget(tab, "output1"));
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(out1));
  gchar spaces[41];
  int i, offset;
  
  GtkTextIter start, end;
  gtk_text_buffer_get_end_iter(buffer, &end);
  offset = gtk_text_iter_get_offset(&end);

  for (i = 0; i < 40; i++) spaces[i] = ' ';
  spaces[40] = '\0';
  gtk_text_buffer_insert(buffer, &end, spaces, -1);

  // apply some tags on inserted text 
  gtk_text_buffer_get_iter_at_offset(buffer, &start, offset);
  gtk_text_buffer_get_end_iter(buffer, &end);

  gtk_text_buffer_apply_tag_by_name (buffer, "horzline", &start, &end);
}

GtkTextIter interface_get_current_position(SESSION_STATE * session)
{
  GtkTextView *out1 = GTK_TEXT_VIEW(interface_get_widget(session->tab, "output1"));

  GtkTextBuffer *buffer =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(out1));

  GtkTextIter it;
  gtk_text_buffer_get_end_iter(buffer, &it);
  return it;
}

// input related

void on_input1_activate(GtkEntry * entry, gpointer user_data)
{
/*  GList *history = NULL;
  GList *list;
  gchar *text, *text2;
  gpointer p;

  GtkWidget *combo;
*/

  GtkWidget *wid;
/*
  SESSION_STATE *session;
  session =
      interface_get_session(GTK_WIDGET(GTK_WIDGET(entry)->parent)->
          parent);
  g_return_if_fail(session != NULL);

  //combo = interface_get_widget( GTK_WIDGET(entry), "input1" );
  combo = GTK_WIDGET(entry)->parent;
  //gtk_widget_show( GTK_COMBO(combo)->popwin );

  list = GTK_LIST(GTK_COMBO(combo)->list)->children;

  // add text from entry in history
  text = (gchar *) gtk_entry_get_text(entry);
  if (!session->telnet->echo)
    history = g_list_append(history, g_strdup(text));

  while (list != NULL) {
    text2 =
        GTK_LABEL(GTK_BIN(GTK_ITEM(list->data))->child)->label;
    if (strcmp(text, text2)) {
      history = g_list_append(history, g_strdup(text2));
    }
    if (g_list_length(history) > HISTORY_LEN)
      break;
    list = g_list_next(list);
  }
*/
  // get vertical box "input" 
  wid = GTK_WIDGET(GTK_WIDGET(entry)->parent)->parent;
  // get send button
  wid = interface_get_widget(wid, "button_send");
  // call send button 
  on_button_send_clicked(GTK_BUTTON(wid), NULL);
/*
  if (history != NULL)
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), history);
  //destry history ( we don't need it anymore )
  while (history) {
    p = history->data;
    g_free(p);
    history = g_list_remove(history, p);
  }
  g_list_free(history);
*/
  //do we want to keep the last typed in data in our entry field?
  if (config->keepsent) {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
  } else {
    //clear out the entry bar after each input
    gtk_editable_delete_text(GTK_EDITABLE(entry), 0, -1);
  }
}

gboolean on_input2_key_press_event(GtkWidget * widget, GdkEventKey * event,
           gpointer user_data)
{
  if ((event->state & GDK_SHIFT_MASK) && (event->keyval == 10)) {
    on_button_send_clicked(GTK_BUTTON
               (interface_get_widget
          (widget, "button_send")), NULL);
  }
  return FALSE;
}

void interface_echo_user_input(SESSION_STATE * session, gchar * text)
{
  GtkTextView *out;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  GtkTextMark *mark;
  GtkTextTag *tag;
  GtkTextTagTable *tagtable;

  out = (GtkTextView *) interface_get_widget(GTK_WIDGET(session->tab),
             "output1");
  buffer = gtk_text_view_get_buffer(out);
  tagtable = gtk_text_buffer_get_tag_table(buffer);

  // save position
  gtk_text_buffer_get_end_iter(buffer, &end);
  mark = gtk_text_buffer_create_mark(buffer, NULL, &end, TRUE);

  // insert text
  gtk_text_buffer_insert(buffer, &end, text, -1);

  // Put some color on it
  tag = gtk_text_tag_table_lookup(tagtable, "user_input_tag");
  if (!tag)
    tag = gtk_text_buffer_create_tag(buffer, "user_input_tag",
             "foreground",
             session->ufg_color, NULL);

  gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);
  gtk_text_buffer_get_end_iter(buffer, &end);
  gtk_text_buffer_apply_tag(buffer, tag, &start, &end);

  // delete mark 
  gtk_text_buffer_delete_mark(buffer, mark);

  //scroll to bottom
  output_scroll_to_bottom(session->tab);
}

void interface_echo_message(SESSION_STATE * session, gchar * text)
{
  GtkTextView *out;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  GtkTextMark *mark;
  GtkTextTag *tag;

  out =
      (GtkTextView *) interface_get_widget(GTK_WIDGET(session->tab),
             "output1");
  buffer = gtk_text_view_get_buffer(out);

  // save position
  gtk_text_buffer_get_end_iter(buffer, &end);
  mark = gtk_text_buffer_create_mark(buffer, NULL, &end, TRUE);

  // insert text
  gtk_text_buffer_insert(buffer, &end, text, -1);

  // Put some color on it
  tag =
      get_fg_color_tag(buffer, 128 * RED + 128 * GREEN + 255 * BLUE);

  gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);
  gtk_text_buffer_get_end_iter(buffer, &end);
  gtk_text_buffer_apply_tag(buffer, tag, &start, &end);

  // insert newline
  gtk_text_buffer_insert(buffer, &end, "\n", -1);

  //scroll to bottom
  output_scroll_to_bottom(session->tab);
}

gboolean process_macros(SESSION_STATE * session, char *buff, int len)
{
  if (!buff)
    return FALSE;
  if (!len)
    return FALSE;

  // nothing if command doesn't start in '/'
  if (buff[0] != '/')
    return FALSE;

  if (g_str_has_prefix(buff, "/get")) {
    // process the GET macro
    if (len < 7)
      return FALSE; // need at least 7 chars
    if ((buff[4] != ' ') || (buff[5] != '$')) {
      interface_echo_message(session,
                 "Wrong syntax for get - use /get $name.");
      return TRUE;
    }

    gchar *name = buff + 6;
    gchar *value = varlist_get_value(session->variables, name);
    if (value) {
      gchar *msg =
          g_strdup_printf("$%s = %s", name, value);
      interface_echo_message(session, msg);
      g_free(msg);
    } else {
      interface_echo_message(session,
                 "Such variable does not exist.");
    }
    return TRUE;
  }

  if (g_str_has_prefix(buff, "/set")) {
    // process the SET macro
    if (len < 7)
      return FALSE; // need at least 7 chars
    if ((buff[4] != ' ') || (buff[5] != '$')) {
      interface_echo_message(session,
                 "Wrong syntax for set - use /set $name value.");
      return TRUE;
    }
    gchar *data = buff + 6;
    gchar *value = g_strstr_len(data, len - 6, " ");
    if (!value) {
      interface_echo_message(session,
                 "Wrong syntax for set - use /set $name value.");
      return TRUE;
    }
    int namelen = value - data;
    value++;
    gchar *name = g_strndup(data, namelen);
    varlist_set_value(session->variables, name, value);
    g_free(name);
    return TRUE;
  }

  return FALSE;
}

void send_command(SESSION_STATE * session, char *buff, gsize len)
{
  gchar *sendbuf = NULL;
  gchar *sb = NULL;

  len = strlen(buff);

  //convert semicolons to newlines, if allowed
  if (config->entry_seperator != NULL)
    utils_replace(buff, len, config->entry_seperator[0], '\n');
  len = strlen(buff);

  // split into commands, process and send each command
  gchar **cmds = g_strsplit(buff, "\n", 0);
  int num = strv_length(cmds);
  int i;

  if (num == 0) {   // empty command ...
    sendbuf = g_malloc0(3);
    sendbuf[0] = '\r';
    sendbuf[1] = '\n';
    sendbuf[2] = '\0';
    network_data_send(session->telnet->fd, (unsigned char *) sendbuf, 2);
    g_free(sendbuf);
    return;
  }

  for (i = 0; i < num; ++i) {

    gchar *s = cmds[i];
    len = strlen(s);

    // process macros if needed
    if (!process_macros(session, s, len)) {

      // expand variables
      sb = variables_expand(session->variables, s, len);
      len = strlen(sb);

      // report, log
      if (session->local_echo && session->telnet != NULL
          && !session->telnet->echo) {
        interface_echo_user_input(session, sb);
        interface_echo_user_input(session, "\n");
        if ((session->logging)
            && (session->log_file)) {
          log_write_in_logfile(session->
                   log_file, sb,
                   len);
          log_write_in_logfile(session->
                   log_file,
                   "\n", 1);
        }
      }
      // send command
      //attach \r\n to the end
      sendbuf = g_malloc0(len + 3);
      g_strlcpy(sendbuf, sb, len + 3);
      // this is no longer needed - no \n in the middle of the string
      // utils_LF2CRLF(&sendbuf, &len); // convert \n to \r\n
      sendbuf[len] = '\r';
      sendbuf[len + 1] = '\n';
      sendbuf[len + 2] = '\0';

      // and send it
      network_data_send(session->telnet->fd, (unsigned char *) sendbuf,
            len + 2);
      g_free(sendbuf);
      g_free(sb);
    }
  }

  g_strfreev(cmds);
}

void on_button_send_clicked(GtkButton * button, gpointer user_data)
{
    GtkWidget *wid;
    gchar *text = NULL;
    gchar *buff = NULL;
    int result;

    SESSION_STATE *session;
    session = interface_get_session(GTK_WIDGET(button));
    if (session->single_line) {
            wid = interface_get_widget(GTK_WIDGET(button), "input1_entry");
            if (!wid)
                    g_warning("Can NOT acces input1 combo.");
            text =
                (gchar *)
                gtk_entry_get_text(GTK_ENTRY(wid));
      if (session->telnet == NULL || !session->telnet->echo)
                cmd_entry_update_cache (GTK_WIDGET (button));
    } else {
            GtkTextIter start, end;
            GtkTextBuffer *buffer;
            wid = interface_get_widget(GTK_WIDGET(button), "input2");
            buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wid));
            gtk_text_buffer_get_start_iter(buffer, &start);
            gtk_text_buffer_get_end_iter(buffer, &end);
            text =
                gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
    }

    if (session->telnet->fd == NO_CONNECTION )
    {
            g_warning("no connection");
    }
    else
    {
        gsize size;
        ATM*  match;
        buff = g_strdup(text);  // make a copy of text for modules
        size = strlen(text);
        module_call_all_data_out(config->modules, session, &buff,
                                 &size);
        // send to sever the text modified by modules 
        if (!(match = atm_find_fire (session, buff, size, session->aliases, TRUE, &result))
            &&
            !(match = atm_find_fire (session, buff, size, config->aliases, TRUE, &result)))
        {
            send_command (session, buff, size);
        }
        else
        {
            if (!result)
                interface_show_script_errors (match, "Alias errors:");
        }
        g_free(buff);
    }
}

void process_line(SESSION_STATE * session, gchar * line)
{
    int len = strlen(line);
    int result;
    ATM* match;
    if ((session->logging) && (session->log_file)) {
            log_write_in_logfile(session->log_file, line, len);
            log_write_in_logfile(session->log_file, "\n", 1);
    }
    match = atm_find_fire (session, line, len, session->triggers, FALSE, &result);
    if (match && !result)
        interface_show_script_errors (match, "Script errors:");

    match = atm_find_fire (session, line, len, config->triggers, FALSE, &result);
    if (match && !result)
        interface_show_script_errors (match, "Script errors:");
}

void process_mxp_chunk(SESSION_STATE * session, mxpChunk * chunk,
           char *txt)
{
  int chtype = mxp_chunk_type(chunk);
  unsigned int len = 0;
  if (txt)
    len = strlen(txt);
  char *s = 0, *s2 = 0;
  char *name, *value, *action;
  int left, top, width, height;
  GdkColor color;
  int lineTag;
  gboolean bval;

  // this is used to remember current color, when displaying links
  TEXT_ATTR attr;

  switch (chtype) {
  case 0:   //nothing
    break;
  case 1:{    //text or newline or SBR
      if (txt && (txt[0] == 0x1f))
        interface_output_append(GTK_WIDGET
              (session->tab),
              " ", 1);
      else if (txt && (txt[0] == '\r'))
        interface_output_append(GTK_WIDGET
              (session->tab),
              "\n", 1);
      else
        interface_output_append(GTK_WIDGET
              (session->tab),
              txt, len);
    }
    break;
  case 2: {
    lineTag = *((int *) mxp_chunk_data (chunk));
    if ((lineTag >= 10) && (lineTag <= 12)) {
      // TODO: pass the lineTag to the auto-mapper
      // they mean that the following line is of interest, as follows:
      // 10 = room name
      // 11 = room description
      // 12 = room exits
      
    }
  }
  break;
  case 3: {
    mxp_flag (chunk, &name, &bval);
    // TODO - pass the flag to the auto-mapper, if needed
    // bval is true if this denotes beginning of a specific data type,
    // false if it's the end of it.
    // names that should be passed to the mapper or somesuch:
    // "RoomName" - room name
    // "RoomDesc" - room desc
    // "RoomExit" - exits
    // "RoomNum" - room number
    // "Prompt" - prompt
  }
  break;
  case 4:{    //variable
      mxp_variable(chunk, &name, &value, &bval);
      if (bval) //erase
        varlist_remove_value(session->variables,
                 name);
      else
        varlist_set_value(session->variables, name,
              value);
    }
    break;
  case 5:{    // formatting
      // strikeout not supported at the moment
      mxp_formatting(chunk, &s, &session->ansi.size,
               &session->ansi.fgcolor,
               &session->ansi.bgcolor,
               &session->ansi.boldfont,
               &session->ansi.italic,
               &session->ansi.underline);
      if (session->ansi.font)
        free(session->ansi.font);
      //session->ansi.font = s;
      session->ansi.font = s ? g_strdup (s) : 0;
    }
    break;
  case 6:{    //URL link
      mxp_a_link(chunk, &name, &action, &s);
      session->linkName = name;
      session->linkCmd = action;
      session->isSendLink = 0;

      // strip ANSI codes from here - workaround for broken MUDs
      if (session->linkCmd)
        utils_strip_ansi_codes(session->linkCmd, strlen(session->linkCmd));
      if (session->linkName)
        utils_strip_ansi_codes(session->linkName, strlen(session->linkName));

      // format the link
      if (txt) {
        attr = session->ansi; // remember current formatting
        session->ansi.bold = session->ansi.italic =
            FALSE;
        session->ansi.blink =
            session->ansi.reverse = FALSE;
        session->ansi.underline = TRUE;
        session->ansi.fgcolor =
            64 * RED + 64 * GREEN + 255 * BLUE;
        session->ansi.bgcolor = 0;
        interface_output_append(GTK_WIDGET
              (session->tab),
              txt, len);
        session->ansi = attr; // restore formatting
      }
      session->linkName = session->linkCmd = 0;
    }
    break;
  case 7:{    //send link
      mxp_send_link(chunk, &name, &action, &s, &bval);
      session->linkName = name;
      session->linkCmd = action;
      session->isSendLink = 1;
      session->isMenuLink = bval;

      // strip ANSI codes from here - workaround for broken MUDs
      if (session->linkCmd)
        utils_strip_ansi_codes(session->linkCmd, strlen(session->linkCmd));
      if (session->linkName)
        utils_strip_ansi_codes(session->linkName, strlen(session->linkName));

      // format the link
      if (txt) {
        attr = session->ansi; // remember current formatting
        session->ansi.bold = session->ansi.italic =
            FALSE;
        session->ansi.blink =
            session->ansi.reverse = FALSE;
        session->ansi.underline = TRUE;
        session->ansi.fgcolor =
            64 * RED + 64 * GREEN + 255 * BLUE;
        session->ansi.bgcolor = 0;
        interface_output_append(GTK_WIDGET
              (session->tab),
              txt, len);
        session->ansi = attr; // restore formatting
      }
      session->linkName = session->linkCmd = 0;
    }
    break;
  case 9:{    //send this
      s = (char *) mxp_chunk_data(chunk);
      if (s) {
        // send the string to the MUD
        len = strlen(s);
        network_data_send(session->telnet->fd, (unsigned char *) s,
              len);
      }
    }
    break;
  case 10:{   // horizontal line
    interface_output_append (GTK_WIDGET (session->tab), "\n", 1);
    interface_output_append_line (GTK_WIDGET (session->tab));
    interface_output_append (GTK_WIDGET (session->tab), "\n", 1);
    }
    break;
  case 12:{   // create a window
      mxp_window (chunk, &name, &s, &left, &top, &width, &height);
      // create the window
      owindowlist_set_owindow (session->windowlist, name, s, left, top, width, height);
    }
    break;
  case 14:{   // close a window
      s = (char *) mxp_chunk_data(chunk);
      // close the window
      owindowlist_remove_owindow (session->windowlist, s);
    }
    break;
  case 15:{   // set active window
      s = (char *) mxp_chunk_data(chunk);
      // switch active window - 0 means main
      owindowlist_set_active (session->windowlist, s);
    }
    break;
    
  case 20:{   // image
      mxp_image(chunk, &name, &s);
      mxp_process_image(session, name, s);
      session->imagemapName = NULL;
    }
    break;
  case 21: {   // image map
      s = (char *) mxp_chunk_data(chunk);
      if (s)
        session->imagemapName = s;
    }
    break;
  case 22: {  // gauge
      mxp_gauge (chunk, &s, &s2, &name, &color);
      if (!gaugelist_exists (session->gaugelist, s))
        gaugelist_set_gauge (session->gaugelist, s, s2, color);
    }
    break;
  case 23: {  // status var
      mxp_statusvar (chunk, &s, &s2, &name);
      if (!svlist_exists (session->svlist, s))
        svlist_set_statusvar (session->svlist, s, s2, FALSE);
  }
  break;
    case -1:{   //error
      printf("MXP error: %s\n",
             (char *) mxp_chunk_data(chunk));
    } break;
  case -2:{   //warning
      printf("MXP warning: %s\n",
             (char *) mxp_chunk_data(chunk));
    } break;
  };
}

void process_text(SESSION_STATE * session, gchar * buff, unsigned int len)
{

  // need to split text into lines, and also need to feel it to MXP library
  // both these things can be accompliched simultaneously, as the MXP library
  // automatically splits up lines. As a consequence, having the MXP library
  // is always arbitrary, even if we don't use MXP, as we're relying on its
  // functionality even on non-MXP functionality.

  if ((!buff) || (len == 0)) return;

  mxpChunk *chunk = 0;
  struct MXPINFO *mxp = session->telnet->mxp;

  char *txt;
  static char *line = 0;

  //initialize the line pointer
  if (line == 0) {
    line = malloc(1);
    line[0] = '\0';
  }
  module_call_all_data_in(config->modules, session, &buff, &len);

/* DEBUG CODE:
int i;
printf ("TEXT:");
for (i = 0; i < len; ++i) printf ("%c", buff[i]);
puts ("\n");
fflush (stdout);
*/

  mxp_new_text(mxp, buff, len);
  while (mxp_has_next(mxp)) {

    // TODO - call MXP processor, retrieve data in some sane way
    chunk = mxp_next(mxp);
    txt = mxp_chunk_text(chunk);

    process_mxp_chunk(session, chunk, txt);

    if (txt && (txt[0] == '\r') && (txt[1] == '\n')) {
      //we've got a complete line, pass it to triggers and stuff
      utils_strip_ansi_codes(line, strlen(line));
      process_line(session, line);
      free(line);
      line = malloc(1);
      line[0] = '\0';
    }
    //append new text to the line
    if (txt) {
      txt = g_strconcat(line, txt, NULL);
      free(line);
      line = txt;
      // no need to free (txt), as the MXP library does that for us
    }

  }
  g_free(buff);   // buff no more needed
  buff = NULL;
  len = 0;

  //scroll output to bottom
  output_scroll_to_bottom(session->tab);
}

void on_data_available(gpointer tab, gint fd, GdkInputCondition cond)
{
  GtkWidget *notebook, *top;
  gchar *buff = NULL;
  gsize len = 0;
  SESSION_STATE *session;
  session = g_object_get_data(G_OBJECT(tab), "session");

  telnet_process(session->telnet, &buff, &len);

  //if nothing to process - don't process
  if( len > 0 )
    process_text(session, buff, len);

  // if top != current window change title
  top = gtk_widget_get_toplevel(GTK_WIDGET(tab));
  //if (top != interface_get_active_window()) {
  if (!gtk_window_is_active(GTK_WINDOW(top)))
  {
    gchar *icon;
    gtk_window_set_title(GTK_WINDOW(top), "### MudMagic ###");
    icon = g_build_filename( mudmagic_data_directory(), "interface", "mudmagic2.xpm", NULL);
    gtk_window_set_icon_from_file(GTK_WINDOW(top), icon, NULL);
    g_free(icon);
  }

  if (session->telnet->fd < 0 ) // connection close occurs
  { 
    GtkWidget *wid, *message;
    gchar *label;
    gint response;
    gtk_input_remove(session->input_event_id);
    session->input_event_id = -1;
    wid = g_object_get_data(G_OBJECT(session->tab), "input1_entry");

    g_return_if_fail(wid != NULL);
    if ( ! gtk_entry_get_visibility (GTK_ENTRY(wid)) ) {
      interface_input_shadow(session, FALSE);
      gtk_entry_set_text(GTK_ENTRY(wid), "");
    }

    while (1) {
      wid =
          interface_create_object_by_name
          ("dialog_connection_close");
      if (wid == NULL) {
        g_warning
            ("can't create 'dialog_connection_close");
      }
      message =
          interface_get_widget(wid,
             "connection_close_message");
      if (message == NULL) {
        g_warning
            ("can't get 'dialog_connection_close");
      }
      label =
          g_strdup_printf
          ("Connection to %s:%d has been close.",
           session->game_host, session->game_port);
      gtk_label_set_text(GTK_LABEL(message), label);
      g_free(label);
      response = gtk_dialog_run(GTK_DIALOG(wid));
      gtk_widget_destroy(wid);
      if (response == 0) {  // stay disconnect
        break;
      }
      if (response == 1) {  // try to reconnect
		if (session->pconn) proxy_connection_close (session->pconn);
		session->pconn = proxy_connection_open(
    	  session->game_host, session->game_port, proxy_get_by_name (session->proxy, config->proxies)
	    );
		if (session->pconn) session->telnet->fd = session->pconn->sock; else session->telnet->fd = NO_CONNECTION;

        if (session->telnet->fd == NO_CONNECTION ) {
                          interface_messagebox (GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                  network_errmsg (session->telnet->fd));
          continue;
        }
        session->input_event_id =
            gtk_input_add_full(session->telnet->fd,
                   GDK_INPUT_READ,(GdkInputFunction)on_data_available, NULL, tab, NULL);
        break;
      }
      if (response == 2) {  // close tab
        interface_remove_tab(tab);
        return;
      }

    }
  }
  // if tab != current tab change label color 
  notebook = gtk_widget_get_ancestor(tab, GTK_TYPE_NOTEBOOK);
  if (notebook) {
    GtkWidget *label;
    GtkWidget *current_tab;

    current_tab =
        gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),
                gtk_notebook_get_current_page
                (GTK_NOTEBOOK(notebook))
        );
    if (tab != current_tab) {
      label =
          gtk_notebook_get_tab_label(GTK_NOTEBOOK
                   (notebook),
                   GTK_WIDGET(tab));
      if (label) {
        GtkWidget* icon;
        icon = g_object_get_data (G_OBJECT (label), "label_icon");
        gtk_image_set_from_stock (GTK_IMAGE (icon),
                GTK_STOCK_NO,
                GTK_ICON_SIZE_MENU); 
      }
    }
  }
}
