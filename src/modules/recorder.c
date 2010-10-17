/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* recorder.c:                                                             *
*                2005  Shlykov Vasiliy ( vash@zmail.ru       )            *
*                                                                         *
***************************************************************************/
/**************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <gdk/gdkkeysyms.h>
#include <mudmagic.h>
#include <interface.h>
#include <mudaccel.h>
#include <directories.h>

// there will be no exporting
#undef EXPORT
#define EXPORT

#define SESSION_ITEM_NAME "module_recorder_obj"

#define RECORDER_START_LABEL "Start _record"
#define RECORDER_STOP_LABEL  "Stop _record"

#define RESPONSE_ALIAS    -100
#define RESPONSE_MACRO    -101
#define RESPONSE_TRIGGER  -102
#define RESPONSE_CONTINUE -103

typedef struct _Recorder Recorder;

struct _Recorder
{
    GList*     commands;
    GtkWidget* dialog;
    gboolean   destroyed;
};


static MudAccelGroup* recorder_accel_group = NULL;

/*Local*/
void on_button_recorder_clicked (GtkButton *button, gpointer user_data);

/**
 * recorder_new:
 *
 **/
Recorder*
recorder_new (void)
{
  return g_new0 (Recorder, 1);
}

/**
 * recorder_free:
 *
 **/
void
recorder_free (Recorder *rec)
{
  GList* it;

  if (rec == NULL)
      return;

  for (it = g_list_first (rec->commands); it; it = g_list_next (it))
    {
      g_free (it->data);
    }

  g_list_free (rec->commands);

  return g_free (rec);
}

/* set_recorder_button : (local scope only)
 * only 2.4+ has the tool_button features, for windows
 * we need to use the older method to handle the changing of the recorder button
 */
void set_recorder_button( GtkToolButton* item, Recorder *rec )
{
#ifdef GTK_GREATER_THAN 
      gtk_tool_button_set_label (item, rec ? RECORDER_STOP_LABEL : RECORDER_START_LABEL);
      gtk_tool_button_set_stock_id (item, rec ? "gtk-media-stop" : "gtk-media-record");
#else
  GtkWidget *icon;
  GtkWidget *button; 
  gchar *icon_location;
  gchar *label;
  GtkWidget* toolbar = interface_get_main_toolbar();
  GList* it = gtk_container_get_children (GTK_CONTAINER (toolbar));
  gint x = 0;
  gint current_position = 0;

  if( rec )
  {
  icon_location = g_build_filename( mudmagic_data_directory(), "interface", "record_stop.png", NULL );
      icon = gtk_image_new_from_file( icon_location );
  label = RECORDER_STOP_LABEL;
  }
  else
  {
  icon_location = g_build_filename( mudmagic_data_directory(), "interface", "record.png", NULL );
      icon = gtk_image_new_from_file( icon_location );
  label = RECORDER_START_LABEL;
  }

  //ugly hack - but destroy the old button first and readd it
  //fix this eventually - as it just continuously adds new
  //values to the toolbar and doesn't actually remove the orig
  gtk_widget_hide( GTK_WIDGET(item) );

  button = gtk_toolbar_append_element(
                    GTK_TOOLBAR (toolbar), 
                GTK_TOOLBAR_CHILD_BUTTON,       /* a type of element */
                NULL,             /* pointer to widget */
                label,            /* label */
                    "Start recording",        /* tooltip */
                NULL,             /* tooltip private string */
                icon,             /* icon */
                G_CALLBACK(on_button_recorder_clicked),   /* signal */
                "clicked"           /* data for signal */
);
//              current_position );
  gtk_label_set_use_underline(
                GTK_LABEL(((GtkToolbarChild*)(g_list_last(
                        GTK_TOOLBAR (toolbar)->children)->data))->label
                ), TRUE
  ); 
  gtk_widget_set_name( button, "button_recorder" );
  gtk_widget_show( button );
#endif
}

/**
 * recorder_attach:
 *
 **/
static inline void
recorder_attach (Session* ss, Recorder* rec)
{
  g_hash_table_insert (ss->extra, SESSION_ITEM_NAME, rec);
}

/**
 * recorder_get:
 *
 **/
static inline Recorder*
recorder_get (Session* ss)
{
  return (Recorder*) g_hash_table_lookup (ss->extra, SESSION_ITEM_NAME);
}

/**
 * recorder_detach:
 *
 **/
static inline Recorder*
recorder_detach (Session* ss)
{
  Recorder* rec = recorder_get (ss);
  if (rec)
      g_hash_table_remove (ss->extra, SESSION_ITEM_NAME);
  return rec;
}

/**
 * prepare_atm_body:
 *
 **/
static gchar*
prepare_atm_body (GList* commands)
{
  static const gchar template[] = "PRINT \"%s\"\n";

  GList* it;
  gsize  len = 0;
  gchar* res = NULL;
  gchar* sit = NULL;

  for (it = g_list_first (commands); it; it = g_list_next (it))
    {
      len += g_utf8_strlen ((gchar*)it->data, -1);
      len += sizeof (template) - 2;
    }

  res = sit = g_new0 (gchar, len + 1);

  for (it = g_list_first (commands); it; it = g_list_next (it))
    {
      sit += sprintf (sit, template, (gchar*)it->data);
    }

  *sit = '\0';

  return res;
}

/**
 * show_atm_dialog:
 *
 **/
static void
show_atm_dialog (gint response, gboolean global, const gchar* body)
{
// FIXME atm edit dialog call
/*
  if (global)
    {
      switch (response)
        {
          case RESPONSE_ALIAS : interface_open_global_aliases (body); break;
          case RESPONSE_MACRO : interface_open_global_macros (body); break;
          case RESPONSE_TRIGGER : interface_open_global_triggers (body); break;
        }
    }
  else
    {
      switch (response)
        {
          case RESPONSE_ALIAS : interface_open_local_aliases (body); break;
          case RESPONSE_MACRO : interface_open_local_macros (body); break;
          case RESPONSE_TRIGGER : interface_open_local_triggers (body); break;
        }
    }
*/
}

/**
 * show_dialog:
 *
 **/
static gint
show_dialog (GtkWidget* window, gboolean* global, Recorder* rec)
{
  GtkWidget* hbox,
           * vbox,
           * stock,
           * button,
           * align;
  gint responce;

  rec->dialog = gtk_dialog_new_with_buttons ("Create Alias/Macro/Trigger",
                                    GTK_WINDOW (window),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    "_Continue",
                                    RESPONSE_CONTINUE,
                                    "_Alias",
                                    RESPONSE_ALIAS,
                                    "_Trigger",
                                    RESPONSE_TRIGGER,
                                    "_Macro",
                                    RESPONSE_MACRO,
                                    GTK_STOCK_CANCEL,
                                    GTK_RESPONSE_CANCEL,
                                    NULL);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (rec->dialog)->vbox), hbox, FALSE, FALSE, 0);

  stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), stock, FALSE, FALSE, 0);

  align = gtk_alignment_new (0.5, 0.5, 0, 0);

  vbox = gtk_vbox_new (TRUE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_container_add (GTK_CONTAINER (align), vbox);

  gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);

  button = gtk_radio_button_new_with_label (NULL, "Global");
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);

  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (button)),
                                            "Local");
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);

  gtk_widget_show_all (hbox);
  responce = gtk_dialog_run (GTK_DIALOG (rec->dialog));

  /** dialog may be destroyed if session emergent closed*/
  if (rec->dialog != NULL)
    {
      g_assert (global);
      *global = ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

      gtk_widget_destroy (rec->dialog);
    }
  else
    {
      responce = GTK_RESPONSE_CANCEL;
    }

  return responce;
}

/**
 * on_button_recorder_clicked:
 *
 **/
EXPORT
void on_button_recorder_clicked (GtkButton *button, gpointer user_data)
{
  GtkToolButton* item = GTK_TOOL_BUTTON (button);
  Session*  ss = interface_get_active_session ();
  Recorder* rec = NULL;

  if (ss == NULL)
      return;

  rec = recorder_get (ss);

  if (rec != NULL)
    {
      gboolean global;
      gint     response;

      response = show_dialog (interface_get_active_window (), &global, rec);

      if (response == GTK_RESPONSE_CANCEL)
        {
          if (!rec->destroyed)
              recorder_detach (ss);
          recorder_free (rec);
          rec = NULL;
        }
      else if (response == RESPONSE_CONTINUE)
        {
        }
      else
        {
          gchar* body = prepare_atm_body (rec->commands);

          mdebug (DBG_MODULE, 0, "Result: response %d, global %s\n",
                        response, global ? "TRUE" : "FALSE");
          show_atm_dialog (response, global, body);

          recorder_free (recorder_detach (ss));
          g_free (body);
          rec = NULL;
        }

  set_recorder_button(item, rec);
  }
  else
  {
      rec = recorder_new ();
      recorder_attach (ss, rec);

  set_recorder_button(item, rec);
  }
}

static GtkWidget*
recorder_get_toolbar_button (GtkWidget* toolbar)
{
  GList* it = gtk_container_get_children (GTK_CONTAINER (toolbar));

  for (it = g_list_first(it); it != NULL; it = g_list_next (it))
  {
      if (! strcmp (gtk_widget_get_name (GTK_WIDGET (it->data)), "button_recorder"))
      {
          return GTK_WIDGET(it->data);
      }
  }
  return NULL;
}

EXPORT void
on_accel_fired (MudAccelGroup* group, gpointer user_data)
{
  GtkWidget* button = recorder_get_toolbar_button (
                            interface_get_main_toolbar ()
                            );

  on_button_recorder_clicked ((GtkButton*) button, NULL);
}

EXPORT void module_recorder_load (gchar *filename)
{
  MudAccel* accel = mud_accel_new (GDK_R, GDK_CONTROL_MASK, GTK_ACCEL_LOCKED,
                                        &on_accel_fired, NULL);

  recorder_accel_group = mud_accel_group_new ();

  mud_accel_group_connect (recorder_accel_group,
          accel);

  interface_add_global_accel_group (recorder_accel_group);
}

EXPORT void module_recorder_unload (void)
{
  interface_remove_global_accel_group (recorder_accel_group);

  g_object_unref (recorder_accel_group);
}

EXPORT void module_recorder_toolbar_modify (GtkWidget* toolbar)
{
#ifdef GTK_GREATER_THAN 
  Session* ss = interface_get_active_session ();
  GtkToolItem* item;

  item = gtk_tool_button_new_from_stock ("gtk-media-record");
  gtk_tool_button_set_label (GTK_TOOL_BUTTON(item), RECORDER_START_LABEL);
  gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON(item), TRUE);

  g_signal_connect (item, "clicked", G_CALLBACK(on_button_recorder_clicked), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1 );  
  GList* it = gtk_container_get_children (GTK_CONTAINER (toolbar));

  gtk_widget_set_name (GTK_WIDGET (item), "button_recorder");
  gtk_widget_set_sensitive (GTK_WIDGET (item), ss != NULL);
  gtk_widget_show (GTK_WIDGET (item));
#else
  GtkWidget *icon;
  GtkWidget *button; 
  gchar *icon_location;

  icon_location = g_build_filename( mudmagic_data_directory(), "interface", "record.png", NULL );
  icon = gtk_image_new_from_file( icon_location );
  button = gtk_toolbar_append_element(
           GTK_TOOLBAR (toolbar), 
       GTK_TOOLBAR_CHILD_BUTTON,    /* a type of element */
       NULL,            /* pointer to widget */
       RECORDER_START_LABEL,      /* label */
           "Start recording",       /* tooltip */
       NULL,            /* tooltip private string */
       icon,            /* icon */
       G_CALLBACK(on_button_recorder_clicked),  /* signal */
       "clicked"          /* data for signal */
  );

  gtk_label_set_use_underline(
                GTK_LABEL(((GtkToolbarChild*)(g_list_last(
                        GTK_TOOLBAR (toolbar)->children)->data))->label
                ), TRUE
  ); 
  gtk_widget_set_name( button, "button_recorder" );
  gtk_widget_show( button );
#endif
}

EXPORT void module_recorder_toolbar_reset (GtkWidget *toolbar)
{
  GList *l;
  l = gtk_container_get_children( GTK_CONTAINER( toolbar ) );
  while ( l ) {
    if (!strcmp( gtk_widget_get_name(GTK_WIDGET(l->data)), "button_recorder")){
      gtk_widget_destroy( GTK_WIDGET(l->data ) );
    }
    l = g_list_next( l );
  }
  mdebug (DBG_MODULE, 0, ">>> toolbar_reset");
}

EXPORT void module_recorder_data_out (Session* ss, gchar** data, gsize* size)
{
  Recorder* rec = recorder_get (ss);

  if (rec != NULL)
    {
      mdebug (DBG_MODULE, 0, "module_recorder_data_out: '%s'()\n", *data, *size);

      rec->commands = g_list_append (rec->commands, g_strndup (*data, *size));
    }
}

EXPORT void module_recorder_session_changed (Session* ss)
{
  GtkWidget* tb = interface_get_main_toolbar ();
  GtkWidget* button;
  Recorder*  rec;

  g_assert (tb);

  button = recorder_get_toolbar_button (tb);
  rec = recorder_get (ss);

  g_assert (button);

  mdebug (DBG_MODULE, 0, "module_recorder_session_changed\n");

  set_recorder_button(GTK_TOOL_BUTTON(button), rec);
}

EXPORT void module_recorder_session_open (Session* ss)
{
  GtkWidget* tb = interface_get_main_toolbar ();
  GtkWidget* button;
  Recorder*  rec;

  rec = recorder_get (ss);

  g_assert (tb);

  button = recorder_get_toolbar_button (tb);
  g_assert (button);

  mdebug (DBG_MODULE, 0, "module_recorder_session_open\n");

  gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);

  set_recorder_button(GTK_TOOL_BUTTON(button), rec);
}

EXPORT void module_recorder_session_close (Session* ss)
{
  GtkWidget* tb = interface_get_main_toolbar ();

  GtkWidget* button;
  Recorder*  rec = recorder_get (ss);
  gboolean   last_session = g_list_length (get_configuration()->sessions) < 2;

  if (rec != NULL)
    {
      recorder_detach (ss);

      if (rec->dialog != NULL)
        {
          gtk_widget_destroy (rec->dialog);
          rec->dialog = NULL;
          rec->destroyed = TRUE;
        }
      else
        {
          /** rec will destroyed after show_dialog */
          recorder_free (rec);
        }
    }

  g_assert (tb);

  button = recorder_get_toolbar_button (
                            interface_get_main_toolbar ()
                            );
  mdebug (DBG_MODULE, 0, "module_recorder_session_close: last: %d\n", last_session);

  gtk_widget_set_sensitive (GTK_WIDGET((GtkButton*)button), !last_session);
  set_recorder_button(GTK_TOOL_BUTTON((GtkButton*)button), rec);
}

