/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* cmdentry.c:                                                             *
*                2005  Vasiliy Shlykov  ( vash@zmail.ru )                 *
*                                                                         *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <interface.h>
#include <cmdentry.h>
#include <alias_triggers.h>
#include <module.h>

#define CMDE_ONELINE_NAME   "input1_entry"
#define CMDE_MLTLINE_NAME   "input2_entry"

typedef struct
{
    gchar*      str;
    GtkWidget*  tree;
} PassToFind;

static void
cmd_entry_completion_create (GtkEntry* entry);

static void
cmd_entry_completion_init (GtkEntryCompletion* completion, COMMAND_LINE* cl);

static SESSION_STATE*
cmd_get_session (GtkWidget* wid)
{
    if ( !strcmp(CMDE_ONELINE_NAME, gtk_widget_get_name(wid)) )
    {
        return interface_get_session(wid->parent->parent);
    }
    else
    {
        return interface_get_session(wid);
    }
}

GtkWidget*
interface_get_cmdentry (GtkWidget* tree)
{
    return interface_get_widget (tree, CMDE_ONELINE_NAME);
}

gboolean
on_match_selected (GtkEntryCompletion   *widget,
                           GtkTreeModel *model,
                           GtkTreeIter  *iter)
{
    Session* ss = interface_get_session (gtk_entry_completion_get_entry (widget));
    if (ss != NULL)
    {
        ss->cmdline.cmd_store = FALSE;
    }
    else
    {
        g_critical ("on_match_selected: session is NULL");
    }

    return FALSE;
}

/**
 * cmd_entry_init: Initialize a command line entry.
 *
 * @widget: a command line entry.
 * @cl: a #COMMAND_LINE.
 *
 **/
void
cmd_entry_init (GtkWidget *widget,
               COMMAND_LINE* cl)
{
    GtkEntry*   entry = GTK_ENTRY (widget);

    if (get_configuration() -> cmd_autocompl)
    {
        GtkTreeIter   iter;
        GtkEntryCompletion* completion = gtk_entry_get_completion (entry);
        GtkTreeModel* model = gtk_entry_completion_get_model (completion);

        if (! gtk_tree_model_get_iter_first (model, &iter))
        {
            cmd_entry_completion_init (completion, cl);
        }
    }
}

void
on_config_changed (GtkObject* source,
                   gpointer data)
{
    cmd_entry_set_history_size (GTK_ENTRY (data), CONFIG (source)->cmd_buf_size);
    cmd_entry_set_autocompletion (GTK_ENTRY (data), CONFIG (source)->cmd_autocompl);
}

/**
 * find_node_with_name: Search a string (case sensitive) into a tree model.
 *
 **/
gboolean
find_node_with_name (GtkTreeModel *model,
                     GtkTreePath *path,
                     GtkTreeIter *iter,
                     gpointer data)
{
    gchar* row;

    gtk_tree_model_get (model, iter, 0, &row, -1);

    if (! strcmp (row, ((PassToFind*)data)->str ) )
    {
        interface_get_session (((PassToFind*)data)->tree) -> cmdline.cmd_store = FALSE;
        return TRUE;
    }

    return FALSE;
}

/**
 * cmd_entry_create: Creates a command line entry widget.
 *
 * Return value: created widget.
 *
 **/
GtkWidget*
cmd_entry_create ()
{
    GtkWidget* entry = gtk_entry_new ();

    cmd_entry_completion_create (GTK_ENTRY(entry));

    gtk_widget_set_name(entry, CMDE_ONELINE_NAME);

    g_signal_connect(G_OBJECT(entry),
                     "activate",
                     G_CALLBACK (on_input1_activate), NULL);

    g_signal_connect(G_OBJECT(entry),
                     "key_press_event",
                     G_CALLBACK (on_input_key_press_event),
                     NULL);

    g_signal_connect(gtk_entry_get_completion (GTK_ENTRY (entry)),
                     "match-selected",
                     G_CALLBACK (on_match_selected),
                     NULL);

/*	commented out by Victor aka Scorpion 
 *	because of program segfault when 'changed' signal emmited 
 *	but the entry destroyed yet. 
 *	Configuration updates need for another solution anyway: 
 *	current one will update last created entry only.
 * /
    g_signal_connect(G_OBJECT(get_configuration()),
                     "changed",
                     G_CALLBACK (on_config_changed),
                     entry);
// */
    return entry;
}

/**
 * cmd_entry_history_add: Adds items to the end of history buffer.
 *
 * @cl: a #COMMAND_LINE.
 * @str: a string.
 *
 **/
static void
cmd_entry_history_add (COMMAND_LINE* cl, gchar* str)
{
    gchar* ret = g_strdup (str);

    if (! g_queue_is_empty (cl->cmd_buf))
    {
        /* Removind "" at the end of list */
        g_queue_pop_tail (cl->cmd_buf);
    }

    g_queue_push_tail (cl->cmd_buf, ret);
    g_queue_push_tail (cl->cmd_buf, "");

    if (g_queue_get_length (cl->cmd_buf) > (get_configuration()->cmd_buf_size + 1))
    {
        g_free (g_queue_pop_head (cl->cmd_buf));
    }
}

/**
 * cmd_entry_update_cache: Updates completion model of the command line.
 *
 * @tree: One of the parent of the command line entry into widget hierarhy.
 *
 **/
void
cmd_entry_update_cache (GtkWidget* tree)
{
    GtkWidget*  wid;
    GtkEntry*   entry;
    gchar       *str;

    GtkTreeModel*       model;
    GtkTreeIter         iter;
    GtkListStore*       store;
    GtkEntryCompletion* complet;
    COMMAND_LINE*       cl;

    wid = interface_get_cmdentry (tree);
    entry = GTK_ENTRY (wid);

    cl = &interface_get_session(tree)->cmdline;

    str = g_strdup (gtk_entry_get_text (entry));
    str = g_strstrip(str);

    PassToFind ptf = {str, tree};

    complet = gtk_entry_get_completion (entry);
    model = gtk_entry_completion_get_model (complet);

    /* Find passed string in completion model */
    gtk_tree_model_foreach (model, &find_node_with_name, &ptf);

    /* Search result stored in cl->cmd_store */
    if (! cl->cmd_store || ! strcmp (str, ""))
    {
        cl->cmd_store = TRUE;

        mdebug (DBG_CMDENTRY, 0, "Completion not updated.\n");
    }
    else
    {
        store = GTK_LIST_STORE (model);

        if (get_configuration()->cmd_buf_size > 0)
        {
            // If history enabled we should keep size of autocompletion buffer
            // equal to hostory size buffer.
            if (get_configuration()->cmd_autocompl
                  && (g_queue_get_length (cl->cmd_buf) - 1) > get_configuration()->cmd_buf_size)
            {
                mdebug (DBG_CMDENTRY, 0, "Removing from completion: %d\n",
                                                g_queue_get_length (cl->cmd_buf));

                if (gtk_tree_model_get_iter_first (model, &iter))
                {
                    gtk_list_store_remove (store, &iter);
                }
            }

            cmd_entry_history_add (cl, str);
        }

        if (get_configuration()->cmd_autocompl)
        {
            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter, 0, str, -1);

#           if DBG_CMDENTRY > 0 && DBG_MAX_LEVEL > 0
            {
                GValue      gvalue = {0, };

                if (gtk_tree_model_get_iter_first (model, &iter))
                {
                    gtk_tree_model_get_value (model, &iter, 0, &gvalue);
                    str = g_value_get_string (&gvalue);
                    mdebug (DBG_CMDENTRY, 1, "String: '%s'\n", str);
                    g_value_unset (&gvalue);

                }
                mdebug (DBG_CMDENTRY, 0, "Updating complete\n");
            }
#           endif
        }
    }

    cl->cmd_current = g_queue_peek_tail_link (cl->cmd_buf);
}

/**
 * cmd_entry_history_down: Shows previuos command from command line history.
 *
 * @session: a SESSION_STATE.
 * @entry: a command line entry.
 *
 **/
static void
cmd_entry_history_up (SESSION_STATE* session, GtkEntry* entry)
{
    GList* link;
    COMMAND_LINE*   cl = &session->cmdline;

    if (g_queue_is_empty (cl->cmd_buf))
    {
        return;
    }

    if (cl->cmd_current != NULL)
    {
        link = g_list_previous (cl->cmd_current);
        if (NULL != link) /* NULL if head of list achieved already */
        {
            cl->cmd_current = link;
        }
    }
    else
    {
        /* Tail string */
        cl->cmd_current = g_queue_peek_tail_link (cl->cmd_buf);
    }

    if (cl->cmd_current != NULL)
    {
        gtk_entry_set_text (entry, (gchar*)cl->cmd_current->data);
    }
}

/**
 * cmd_entry_history_down: Shows next command from command line history.
 *
 * @session: a SESSION_STATE.
 * @entry: a command line entry.
 *
 **/
static void
cmd_entry_history_down (SESSION_STATE* session, GtkEntry* entry)
{
    COMMAND_LINE*   cl = &session->cmdline;

    if (g_queue_is_empty (cl->cmd_buf))
    {
        return;
    }

    if (cl->cmd_current != NULL)
    {
        cl->cmd_current = g_list_next (cl->cmd_current);
    }
    else
    {
        cl->cmd_current = g_queue_peek_tail_link (cl->cmd_buf);
    }

    if (cl->cmd_current != NULL)
    {
        gtk_entry_set_text (entry, (gchar*)cl->cmd_current->data);
    }
}

/**
 * cmd_entry_set_autocompletion: Sets autocompletion mode to command line entry.
 *                               If @enable is true then completion model will
 *                               be initialized by strings from history buffer.
 *
 * @entry: a #GtkEntry.
 * @enable: new state.
 *
 **/
void
cmd_entry_set_autocompletion (GtkEntry* entry, gboolean enable)
{
    GtkEntryCompletion* complet;
    GtkTreeModel*       model;
    GtkTreeIter         iter;

    complet = gtk_entry_get_completion (entry);
    model = gtk_entry_completion_get_model (complet);

    /* ListStore should be empty before initialization */
    if (enable && !gtk_tree_model_get_iter_first(model, &iter) )
    {
        SESSION_STATE* session = cmd_get_session (GTK_WIDGET (entry));
        g_assert (session);
        
        cmd_entry_completion_init (complet, &session->cmdline);
        return;
    }

    if (!enable)
    {
        gtk_list_store_clear (GTK_LIST_STORE (model));
    }
}

/**
 * cmd_entry_set_history_size: Sets history buffer size.
 *
 * @entry: a #GtkEntry.
 * @size: number of a keep command.
 *
 **/
void
cmd_entry_set_history_size (GtkEntry* entry, guint size)
{
    SESSION_STATE* session;
    COMMAND_LINE* cl;

    session = cmd_get_session (GTK_WIDGET (entry));
    g_assert (session);

    cl = &session->cmdline;

    cmd_entry_history_clear (cl, size);

    if (size && g_queue_is_empty (cl->cmd_buf))
    {
        g_queue_push_tail(cl->cmd_buf, "");
        cl->cmd_current = g_queue_peek_tail_link (cl->cmd_buf);
    }
}

gboolean
on_input_key_press_event (GtkWidget*    widget,
                          GdkEventKey*  event,
			  gpointer      user_data)
{
    SESSION_STATE *session;
    GList *l;
    gboolean done = FALSE;
    gint state = event->state;
    gint key = gdk_keyval_to_upper(event->keyval);
    gint res = TRUE;
    ATM* atm = NULL;

    session = cmd_get_session(widget);

    if (!session)
            return FALSE;

    /* History navigation */
    if (get_configuration()->cmd_buf_size > 0)
    {
        if (event->keyval == GDK_Up || event->keyval == GDK_KP_Up)
        {
            cmd_entry_history_up (session, GTK_ENTRY (widget));
            return TRUE;
        }

        if (event->keyval == GDK_Down || event->keyval == GDK_KP_Down)
        {
            cmd_entry_history_down (session, GTK_ENTRY (widget));
            return TRUE;
        }
    }

    l = session->macros;
    while (l && !done)
      {
        atm = (ATM*) l->data;
        if (atm_macro_in_fire (atm, state, key))
          {
            res = atm_execute (session, atm, NULL, 0);
            done = TRUE;
          }
        l = g_list_next(l);
      }

    l = get_configuration()->macros;
    while (l && !done)
      {
        atm = (ATM*) l->data;
        if (atm_macro_in_fire (atm, state, key))
          {
            res = atm_execute (session, atm, NULL, 0);
            done = TRUE;
          }
        l = g_list_next(l);
      }

    if (done && !res)
      {
        interface_show_script_errors (atm, "Script errors:");
      }

    return done;
}

/**
 * cmd_entry_completion_create: Creates and attaches to @entry a #GtkEntryCompletion
 *                              object. Initializes a model for completion.
 *
 * @entry: a #GtkEntry.
 *
 **/
static void
cmd_entry_completion_create (GtkEntry* entry)
{
    GtkTreeModel *completion_model;
    GtkEntryCompletion *completion = gtk_entry_completion_new ();

    gtk_entry_set_completion (entry, completion);
    g_object_unref (completion);

    /* Create a tree model and use it as the completion model */
    completion_model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
                                                                                    
    gtk_entry_completion_set_model (completion, completion_model);
    g_object_unref (completion_model);

    gtk_entry_completion_set_inline_completion (completion, TRUE);
//      gtk_entry_completion_set_popup_completion (completion, FALSE);

    /* Use model column 0 as the text column */
    gtk_entry_completion_set_text_column (completion, 0);
}

/**
 * cmd_entry_completion_init: Initialization of entry completion's model by
 *                            strings from command line history.
 *
 * @completion: a #GtkEntryCompletion.
 * @cl: a #COMMAND_LINE.
 *
 **/
static void
cmd_entry_completion_init (GtkEntryCompletion* completion, COMMAND_LINE* cl)
{
    gint  i,
          size = g_queue_get_length (cl->cmd_buf) - 1;

    gchar* line;

    GtkTreeIter         iter;
    GtkListStore*       store;

    store = GTK_LIST_STORE (gtk_entry_completion_get_model (completion));

    for (i = 0; i < size; ++i)
    {
        line = (gchar*) g_queue_peek_nth (cl->cmd_buf, i);

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, line, -1);
    }
}

/**
 * cmd_entry_history_load: Loads command line history from file. One line into file
 *                         interpret as one command. At end of loaded list empty
 *                         string ("") will added. Maximum length of line into
 *                         file should not exceed 256 symbols.
 *
 * @cl: a #COMMAND_LINE.
 * @f: specified file.
 *
 **/
void
cmd_entry_history_load (COMMAND_LINE* cl, FILE* f)
{
    int status = 0;
    guint line = 0;
    guint maxline = get_configuration () -> cmd_buf_size;
    gchar cmd[256];

    for (; line < maxline; ++line)
    {
        status = fscanf (f, "%s\n", cmd);
        if (status == EOF)
        {
                break;
        }
        g_queue_push_tail (cl->cmd_buf, g_strndup (cmd, 256));
    }

    g_queue_push_tail(cl->cmd_buf, "");
    cl->cmd_current = g_queue_peek_tail_link (cl->cmd_buf);
}

/**
 * cmd_entry_history_save: Saves entry's history to file. Each line of 
 *                         output file will contain a one command.
 *
 * @cl: a #COMMAND_LINE.
 * @f: specified file.
 *
 **/
void
cmd_entry_history_save (COMMAND_LINE* cl, FILE* f)
{
    gint size = g_queue_get_length (cl->cmd_buf) - 1;
    gint i = 0;

    for (; i < size; ++i)
    {
        fprintf (f, "%s\n", (gchar*) g_queue_peek_nth (cl->cmd_buf, i));
    }
}

/**
 * cmd_entry_histort_clear: Clears and free memory occupied by history buffer.
 *
 * @cl: a #COMMAND_LINE.
 * @size: new size of buffer.
 *
 **/
void
cmd_entry_history_clear (COMMAND_LINE* cl, guint size)
{
    gchar* str;

    while (g_queue_get_length(cl->cmd_buf) > size)
    {
        str = (gchar*) g_queue_pop_head (cl->cmd_buf);
        if (strcmp (str, ""))
        {
            g_free (str);
        }
    }
}

/**
 * cmd_entry_set_focus: Sets focus on the command line entry.
 *
 **/
void cmd_entry_set_focus (void)
{
    GtkWidget* tab = interface_get_active_tab ();
    GtkWidget* wid;
    Session*   session;

    g_return_if_fail (tab != NULL);

    session = interface_get_session (tab);
    g_return_if_fail (session != NULL);

    wid = interface_get_widget (tab,
	    session->single_line ? CMDE_ONELINE_NAME : CMDE_MLTLINE_NAME);

    gtk_widget_grab_focus (wid);
}

