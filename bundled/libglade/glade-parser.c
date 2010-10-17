/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2002  James Henstridge <james@daa.com.au>
 *
 * glade-parser.c: functions for parsing glade-2.0 files
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_NLS
#  include <libintl.h>
#else
#  define textdomain(String) (String)
#  define dgettext(Domain, String) (String)
#endif

#include <libxml/parser.h>

#include "glade/glade.h"

typedef enum {
    PARSER_START,
    PARSER_GLADE_INTERFACE,
    PARSER_REQUIRES,
    PARSER_WIDGET,
    PARSER_WIDGET_PROPERTY,
    PARSER_WIDGET_ATK,
    PARSER_WIDGET_ATK_PROPERTY,
    PARSER_WIDGET_ATK_ACTION,
    PARSER_WIDGET_ATK_RELATION,
    PARSER_WIDGET_AFTER_ATK,
    PARSER_WIDGET_SIGNAL,
    PARSER_WIDGET_AFTER_SIGNAL,
    PARSER_WIDGET_ACCEL,
    PARSER_WIDGET_AFTER_ACCEL,
    PARSER_WIDGET_CHILD,
    PARSER_WIDGET_CHILD_AFTER_WIDGET,
    PARSER_WIDGET_CHILD_PACKING,
    PARSER_WIDGET_CHILD_PACKING_PROPERTY,
    PARSER_WIDGET_CHILD_AFTER_PACKING,
    PARSER_WIDGET_CHILD_PLACEHOLDER,
    PARSER_WIDGET_CHILD_AFTER_PLACEHOLDER,
    PARSER_FINISH,
    PARSER_UNKNOWN
} ParserState;

#ifdef DEBUG
static const gchar *state_names[] = {
    "START",
    "GLADE_INTERFACE",
    "REQUIRES",
    "WIDGET",
    "WIDGET_PROPERTY",
    "WIDGET_ATK",
    "WIDGET_ATK_PROPERTY",
    "WIDGET_ATK_ACTION",
    "WIDGET_ATK_RELATION",
    "WIDGET_AFTER_ATK",
    "WIDGET_SIGNAL",
    "WIDGET_AFTER_SIGNAL",
    "WIDGET_ACCEL",
    "WIDGET_AFTER_ACCEL",
    "WIDGET_CHILD",
    "WIDGET_CHILD_AFTER_WIDGET",
    "WIDGET_CHILD_PACKING",
    "WIDGET_CHILD_PACKING_PROPERTY",
    "WIDGET_CHILD_AFTER_PACKING",
    "WIDGET_CHILD_PLACEHOLDER",
    "WIDGET_CHILD_AFTER_PLACEHOLDER",
    "FINISH",
    "UNKNOWN",
};
#endif

typedef struct _GladeParseState GladeParseState;
struct _GladeParseState {
    ParserState state;

    const gchar *domain;

    guint unknown_depth;    /* handle recursive unrecognised tags */
    ParserState prev_state; /* the last `known' state we were in */

    guint widget_depth;
    GString *content;

    GladeInterface *interface;
    GladeWidgetInfo *widget;

    enum {PROP_NONE, PROP_WIDGET, PROP_ATK, PROP_CHILD } prop_type;
    gchar *prop_name;
    gboolean translate_prop;
    gboolean context_prop;
    GArray *props;

    GArray *signals;
    GArray *atk_actions;
    GArray *relations;
    GArray *accels;
};

static gchar *
alloc_string(GladeInterface *interface, const xmlChar *xmlstring)
{
    gchar *s;
    gchar *string;

    string = g_strdup_printf( "%s", xmlstring );

    s = g_hash_table_lookup(interface->strings, string);
    if (!s) {
	s = g_strdup(string);
	g_hash_table_insert(interface->strings, s, s);
    }
    return s;
}

static gchar * alloc_propname(GladeInterface *interface, const gchar *string)
{
    static GString *norm_str;

    gint i;

    if (!norm_str)
	norm_str = g_string_new_len(NULL, 64);

    /* assign the string to norm_str */
    g_string_assign(norm_str, string);
    /* convert all dashes to underscores */
    for (i = 0; i < norm_str->len; i++)
	if (norm_str->str[i] == '-')
	    norm_str->str[i] = '_';

    return alloc_string(interface, (xmlChar *)norm_str->str );
}

static GladeWidgetInfo *
create_widget_info(GladeInterface *interface, const xmlChar **attrs)
{
    GladeWidgetInfo *info = g_new0(GladeWidgetInfo, 1);
    int i;

    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
	if (!xmlStrcmp(attrs[i], (xmlChar *)"class"))
	    info->classname = alloc_string(interface, attrs[i+1]);
	else if (!xmlStrcmp(attrs[i], (xmlChar *)"id"))
	    info->name = alloc_string(interface, attrs[i+1]);
	else
	    g_warning("unknown attribute `%s' for <widget>.", attrs[i]);
    }
    if (info->classname == NULL || info->name == NULL)
	g_warning("<widget> element missing required attributes!");
    g_hash_table_insert(interface->names, info->name, info);
    return info;
}

static inline void
flush_properties(GladeParseState *state)
{
    if (state->props == NULL)
	return;
    switch (state->prop_type) {
    case PROP_NONE:
	break;
    case PROP_WIDGET:
	if (state->widget->properties)
	    g_warning("we already read all the props for this key.  Leaking");
	state->widget->properties = (GladeProperty *)state->props->data;
	state->widget->n_properties = state->props->len;
	g_array_free(state->props, FALSE);
	break;
    case PROP_ATK:
	if (state->widget->atk_props)
	    g_warning("we already read all the ATK props for this key.  Leaking");
	state->widget->atk_props = (GladeProperty *)state->props->data;
	state->widget->n_atk_props = state->props->len;
	g_array_free(state->props, FALSE);
	break;
    case PROP_CHILD:
	if (state->widget->n_children == 0) {
	    g_warning("no children, but have child properties!");
	    g_array_free(state->props, TRUE);
	} else {
	    GladeChildInfo *info = &state->widget->children[
						state->widget->n_children-1];
	    if (info->properties)
		g_warning("we already read all the child props for this key.  Leaking");
	    info->properties = (GladeProperty *)state->props->data;
	    info->n_properties = state->props->len;
	    g_array_free(state->props, FALSE);
	}
	break;
    }
    state->prop_type = PROP_NONE;
    state->prop_name = NULL;
    state->props = NULL;
}

static inline void
flush_signals(GladeParseState *state)
{
    if (state->signals) {
	state->widget->signals = (GladeSignalInfo *)state->signals->data;
	state->widget->n_signals = state->signals->len;
	g_array_free(state->signals, FALSE);
    }
    state->signals = NULL;
}

static inline void
flush_actions(GladeParseState *state)
{
    if (state->atk_actions) {
	state->widget->atk_actions = (GladeAtkActionInfo *)state->atk_actions->data;
	state->widget->n_atk_actions = state->atk_actions->len;
	g_array_free(state->atk_actions, FALSE);
    }
    state->atk_actions = NULL;
}

static inline void
flush_relations(GladeParseState *state)
{
    if (state->relations) {
	state->widget->relations = (GladeAtkRelationInfo *)state->relations->data;
	state->widget->n_relations = state->relations->len;
	g_array_free(state->relations, FALSE);
    }
    state->relations = NULL;
}

static inline void
flush_accels(GladeParseState *state)
{
    if (state->accels) {
	state->widget->accels = (GladeAccelInfo *)state->accels->data;
	state->widget->n_accels = state->accels->len;
	g_array_free(state->accels, FALSE);
    }
    state->accels = NULL;
}

static inline void
handle_atk_action(GladeParseState *state, const xmlChar **attrs)
{
    gint i;
    GladeAtkActionInfo info = { NULL };

    flush_properties(state);

    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
	if (!xmlStrcmp(attrs[i], (xmlChar *)"action_name"))
	    info.action_name = alloc_string(state->interface, attrs[i+1]);
	else if (!xmlStrcmp(attrs[i], (xmlChar *)"description"))
	    info.description = alloc_string(state->interface,
				    dgettext(state->domain, attrs[i+1]));
	else
	    g_warning("unknown attribute `%s' for <action>.", attrs[i]);
    }
    if (info.action_name == NULL) {
	g_warning("required <atkaction> attribute 'action_name' missing!!!");
	return;
    }
    if (!state->atk_actions)
	state->atk_actions = g_array_new(FALSE, FALSE,
				     sizeof(GladeAtkActionInfo));
    g_array_append_val(state->atk_actions, info);
}

static inline void
handle_atk_relation(GladeParseState *state, const xmlChar **attrs)
{
    gint i;
    GladeAtkRelationInfo info = { NULL };

    flush_properties(state);

    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
	if (!xmlStrcmp(attrs[i], (xmlChar *)"target"))
	    info.target = alloc_string(state->interface, attrs[i+1]);
	else if (!xmlStrcmp(attrs[i], (xmlChar *)"type"))
	    info.type = alloc_string(state->interface, attrs[i+1]);
	else
	    g_warning("unknown attribute `%s' for <signal>.", attrs[i]);
    }
    if (info.target == NULL || info.type == NULL) {
	g_warning("required <atkrelation> attributes ('target' and/or 'type') missing!!!");
	return;
    }
    if (!state->relations)
	state->relations = g_array_new(FALSE, FALSE,
				     sizeof(GladeAtkRelationInfo));
    g_array_append_val(state->relations, info);
}

static inline void
handle_signal(GladeParseState *state, const xmlChar **attrs)
{
    GladeSignalInfo info = { NULL };
    gint i;

    flush_properties(state);

    info.after = FALSE;
    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
	if (!xmlStrcmp(attrs[i], (xmlChar *)"name"))
	    info.name = alloc_string(state->interface, attrs[i+1]);
	else if (!xmlStrcmp(attrs[i], (xmlChar *)"handler"))
	    info.handler = alloc_string(state->interface, attrs[i+1]);
	else if (!xmlStrcmp(attrs[i], (xmlChar *)"after"))
	    info.after = attrs[i+1][0] == 'y';
	else if (!xmlStrcmp(attrs[i], (xmlChar *)"object"))
	    info.object = alloc_string(state->interface, attrs[i+1]);
	else if (!xmlStrcmp(attrs[i], (xmlChar *)"last_modification_time"))
	    /* Do nothing. */;
	else
	    g_warning("unknown attribute `%s' for <signal>.", attrs[i]);
    }
    if (info.name == NULL || info.handler == NULL) {
	g_warning("required <signal> attributes missing!!!");
	return;
    }
    if (!state->signals)
	state->signals = g_array_new(FALSE, FALSE,
				     sizeof(GladeSignalInfo));
    g_array_append_val(state->signals, info);
}

static inline void
handle_accel(GladeParseState *state, const xmlChar **attrs)
{
    GladeAccelInfo info = { 0 };
    gint i;

    flush_properties(state);
    flush_signals(state);
    flush_actions(state);
    flush_relations(state);

    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
	if (!xmlStrcmp(attrs[i], (xmlChar *)"key"))
	    info.key = gdk_keyval_from_name( (gchar *)attrs[i+1]);
	else if (!xmlStrcmp(attrs[i], (xmlChar *)"modifiers")) {
	    const xmlChar *pos = attrs[i+1];

	    info.modifiers = 0;
	    while (pos[0])
		if (!xmlStrncmp(pos, (xmlChar *)"GDK_", 4)) {
		    pos += 4;
		    if (!xmlStrncmp(pos, (xmlChar *)"SHIFT_MASK", 10)) {
			info.modifiers |= GDK_SHIFT_MASK;
			pos += 10;
		    } else if (!xmlStrncmp(pos, (xmlChar *)"LOCK_MASK", 9)) {
			info.modifiers |= GDK_LOCK_MASK;
			pos += 9;
		    } else if (!xmlStrncmp(pos, (xmlChar *)"CONTROL_MASK", 12)) {
			info.modifiers |= GDK_CONTROL_MASK;
			pos += 12;
		    } else if (!xmlStrncmp(pos, (xmlChar *)"MOD", 3) &&
			       !xmlStrncmp(pos+4, (xmlChar *)"_MASK", 5)) {
			switch (pos[3]) {
			case '1':
			    info.modifiers |= GDK_MOD1_MASK; break;
			case '2':
			    info.modifiers |= GDK_MOD2_MASK; break;
			case '3':
			    info.modifiers |= GDK_MOD3_MASK; break;
			case '4':
			    info.modifiers |= GDK_MOD4_MASK; break;
			case '5':
			    info.modifiers |= GDK_MOD5_MASK; break;
			}
			pos += 9;
		    } else if (!xmlStrncmp(pos, (xmlChar *)"BUTTON", 6) &&
			       !xmlStrncmp(pos+7, (xmlChar *)"_MASK", 5)) {
			switch (pos[6]) {
			case '1':
			    info.modifiers |= GDK_BUTTON1_MASK; break;
			case '2':
			    info.modifiers |= GDK_BUTTON2_MASK; break;
			case '3':
			    info.modifiers |= GDK_BUTTON3_MASK; break;
			case '4':
			    info.modifiers |= GDK_BUTTON4_MASK; break;
			case '5':
			    info.modifiers |= GDK_BUTTON5_MASK; break;
			}
			pos += 12;
		    } else if (!xmlStrncmp(pos, (xmlChar *)"RELEASE_MASK", 12)) {
			info.modifiers |= GDK_RELEASE_MASK;
			pos += 12;
		    } else
			pos++;
               } else
                   pos++;
	} else if (!xmlStrcmp(attrs[i], (xmlChar *)"signal"))
	    info.signal = alloc_string(state->interface, attrs[i+1]);
	else
	    g_warning("unknown attribute `%s' for <accelerator>.", attrs[i]);
    }
    if (info.key == 0 || info.signal == NULL) {
	g_warning("required <accelerator> attributes missing!!!");
	return;
    }
    if (!state->accels)
	state->accels = g_array_new(FALSE, FALSE,
				    sizeof(GladeAccelInfo));
    g_array_append_val(state->accels, info);
}

static inline void
handle_child(GladeParseState *state, const xmlChar **attrs)
{
    GladeChildInfo *info;
    gint i;

    /* make sure all of these are flushed */
    flush_properties(state);
    flush_signals(state);
    flush_actions(state);
    flush_relations(state);
    flush_accels(state);

    state->widget->n_children++;
    state->widget->children = g_renew(GladeChildInfo, state->widget->children,
				      state->widget->n_children);
    info = &state->widget->children[state->widget->n_children-1];
    info->internal_child = NULL;
    info->properties = NULL;
    info->n_properties = 0;
    info->child = NULL;

    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
	if (!xmlStrcmp(attrs[i], (xmlChar *)"internal-child"))
	    info->internal_child = alloc_string(state->interface, attrs[i+1]);
	else
	    g_warning("unknown attribute `%s' for <child>.", attrs[i]);
    }
}

static void
glade_parser_start_document(GladeParseState *state)
{
    state->state = PARSER_START;

    state->unknown_depth = 0;
    state->prev_state = PARSER_UNKNOWN;

    state->widget_depth = 0;
    state->content = g_string_sized_new(128);

    state->interface = g_new0(GladeInterface, 1);
    state->interface->names = g_hash_table_new(g_str_hash, g_str_equal);
    state->interface->strings = g_hash_table_new_full(g_str_hash,
						      g_str_equal,
						      (GDestroyNotify)g_free,
						      NULL);
    state->widget = NULL;

    state->prop_type = PROP_NONE;
    state->prop_name = NULL;
    state->translate_prop = FALSE;
    state->props = NULL;

    state->signals = NULL;
    state->accels = NULL;
}

static void
glade_parser_end_document(GladeParseState *state)
{
    g_string_free(state->content, TRUE);

    if (state->unknown_depth != 0)
	g_warning("unknown_depth != 0 (%d)", state->unknown_depth);
    if (state->widget_depth != 0)
	g_warning("widget_depth != 0 (%d)", state->widget_depth);
}

static void
glade_parser_start_element(GladeParseState *state,
			   const xmlChar *name, const xmlChar **attrs)
{
    int i;

    GLADE_NOTE(PARSER, g_message("<%s> in state %s",
				 name, state_names[state->state]));

    switch (state->state) {
    case PARSER_START:
	if (!xmlStrcmp(name, (xmlChar *)"glade-interface")) {
	    state->state = PARSER_GLADE_INTERFACE;
#if 0
	    /* check for correct XML namespace */
	    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
		if (!xmlStrcmp(attrs[i], (xmlChar *)"xmlns") &&
		    !xmlStrcmp(attrs[i+1], (xmlChar *)"...")) {
		    g_warning("bad XML namespace `%s'.", attrs[i+1]);
		} else
		    g_warning("unknown attribute `%s' for <glade-interface>",
			      attrs[i]);
	    }
#endif
	} else {
	    g_warning("Expected <glade-interface>.  Got <%s>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_GLADE_INTERFACE:
	if (!xmlStrcmp(name, (xmlChar *)"requires")) {
	    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
		if (!xmlStrcmp(attrs[i], (xmlChar *)"lib")) {
		    GladeInterface *iface = state->interface;

		    /* add to the list of requirements for this module */
		    iface->n_requires++;
		    iface->requires = g_renew(gchar *, iface->requires,
					      iface->n_requires);
		    iface->requires[iface->n_requires-1] =
			alloc_string(iface, attrs[i+1]);
		} else
		    g_warning("unknown attribute `%s' for <requires>.",
			      attrs[i]);
	    }
	    state->state = PARSER_REQUIRES;
	} else if (!xmlStrcmp(name, (xmlChar *)"widget")) {
	    GladeInterface *iface = state->interface;

	    iface->n_toplevels++;
	    iface->toplevels = g_renew(GladeWidgetInfo *, iface->toplevels,
				       iface->n_toplevels);
	    state->widget = create_widget_info(iface, attrs);
	    iface->toplevels[iface->n_toplevels-1] = state->widget;

	    state->widget_depth++;
	    state->prop_type = PROP_NONE;
	    state->prop_name = NULL;
	    state->props = NULL;
	    state->signals = NULL;
	    state->accels = NULL;

	    state->state = PARSER_WIDGET;
	} else {
	    g_warning("Unexpected element <%s> inside <glade-interface>.",
		      name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_REQUIRES:
	g_warning("<requires> element should be empty.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET:
	if (!xmlStrcmp(name, (xmlChar *)"property")) {
	    gboolean bad_agent = FALSE;

	    if (state->prop_type != PROP_NONE &&
		state->prop_type != PROP_WIDGET)
		g_warning("non widget properties defined here (oh no!)");
	    state->translate_prop = FALSE;
	    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
		if (!xmlStrcmp(attrs[i], (xmlChar *)"name"))
		    state->prop_name = alloc_propname(state->interface,
						      (gchar *)attrs[i+1]);
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"translatable"))
		    state->translate_prop = !xmlStrcmp(attrs[i+1], (xmlChar *)"yes");
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"context"))
		    state->context_prop = !xmlStrcmp(attrs[i+1], (xmlChar *)"yes");
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"agent"))
		    bad_agent = xmlStrcmp(attrs[i], (xmlChar *)"libglade") != 0;
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"comments"))
		    /* Do nothing. */;
		else
		    g_warning("unknown attribute `%s' for <property>.",
			      attrs[i]);
	    }
	    if (bad_agent) {
		/* ignore the property ... */
		state->prev_state = state->state;
		state->state = PARSER_UNKNOWN;
		state->unknown_depth++;
	    } else {
		state->prop_type = PROP_WIDGET;
		state->state = PARSER_WIDGET_PROPERTY;
	    }
	} else if (!xmlStrcmp(name, (xmlChar *)"accessibility")) {
	    flush_properties(state);

	    if (attrs != NULL && attrs[0] != NULL)
		g_warning("<accessibility> element should have no attributes");
	    state->state = PARSER_WIDGET_ATK;
	} else if (!xmlStrcmp(name, (xmlChar *)"signal")) {
	    handle_signal(state, attrs);
	    state->state = PARSER_WIDGET_SIGNAL;
	} else if (!xmlStrcmp(name, (xmlChar *)"accelerator")) {
	    handle_accel(state, attrs);
	    state->state = PARSER_WIDGET_ACCEL;
	} else if (!xmlStrcmp(name, (xmlChar *)"child")) {
	    handle_child(state, attrs);
	    state->state = PARSER_WIDGET_CHILD;
	} else {
	    g_warning("Unexpected element <%s> inside <widget>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_PROPERTY:
	g_warning("<property> element should be empty.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET_ATK:
	if (!xmlStrcmp(name, (xmlChar *)"atkproperty")) {
	    if (state->prop_type != PROP_NONE &&
		state->prop_type != PROP_ATK)
		g_warning("non atk properties defined here (oh no!)");
	    state->prop_type = PROP_ATK;
	    state->translate_prop = FALSE;
	    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
		if (!xmlStrcmp(attrs[i], (xmlChar *)"name"))
		    state->prop_name = alloc_propname(state->interface,
						      (gchar *)attrs[i+1]);
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"translatable"))
		    state->translate_prop = !xmlStrcmp(attrs[i+1], (xmlChar *)"yes");
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"context"))
		    state->context_prop = !xmlStrcmp(attrs[i+1], (xmlChar *)"yes");
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"comments"))
		    /* Do nothing. */;
		else
		    g_warning("unknown attribute `%s' for <atkproperty>.",
			      attrs[i]);
	    }
	    state->state = PARSER_WIDGET_ATK_PROPERTY;
	} else if (!xmlStrcmp(name, (xmlChar *)"atkaction")) {
	    handle_atk_action(state, attrs);
	    state->state = PARSER_WIDGET_ATK_ACTION;
	} else if (!xmlStrcmp(name, (xmlChar *)"atkrelation")) {
	    handle_atk_relation(state, attrs);
	    state->state = PARSER_WIDGET_ATK_RELATION;
	} else {
	    g_warning("Unexpected element <%s> inside <accessibility>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_ATK_PROPERTY:
	if (!xmlStrcmp(name, (xmlChar *)"accessibility")) {
	    state->state = PARSER_WIDGET_ATK;
	} else {
	    g_warning("Unexpected element <%s> inside <atkproperty>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_ATK_ACTION:
	g_warning("<atkaction> element should be empty.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET_ATK_RELATION:
	g_warning("<atkrelation> element should be empty.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET_AFTER_ATK:
	if (!xmlStrcmp(name, (xmlChar *)"signal")) {
	    handle_signal(state, attrs);
	    state->state = PARSER_WIDGET_SIGNAL;
	} else if (!xmlStrcmp(name, (xmlChar *)"accelerator")) {
	    handle_accel(state, attrs);
	    state->state = PARSER_WIDGET_ACCEL;
	} else if (!xmlStrcmp(name, (xmlChar *)"child")) {
	    handle_child(state, attrs);
	    state->state = PARSER_WIDGET_CHILD;
	} else {
	    g_warning("Unexpected element <%s> inside <widget>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_SIGNAL:
	g_warning("<signal> element should be empty.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET_AFTER_SIGNAL:
	if (!xmlStrcmp(name, (xmlChar *)"accelerator")) {
	    handle_accel(state, attrs);
	    state->state = PARSER_WIDGET_ACCEL;
	} else if (!xmlStrcmp(name, (xmlChar *)"child")) {
	    handle_child(state, attrs);
	    state->state = PARSER_WIDGET_CHILD;
	} else {
	    g_warning("Unexpected element <%s> inside <widget>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_ACCEL:
	g_warning("<accelerator> element should be empty.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET_AFTER_ACCEL:
	if (!xmlStrcmp(name, (xmlChar *)"child")) {
	    handle_child(state, attrs);
	    state->state = PARSER_WIDGET_CHILD;
	} else {
	    g_warning("Unexpected element <%s> inside <widget>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_CHILD:
	if (!xmlStrcmp(name, (xmlChar *)"widget")) {
	    GladeWidgetInfo *parent = state->widget;
	    GladeChildInfo *info = &parent->children[parent->n_children-1];

	    if (info->child)
		g_warning("widget pointer already set!! not good");

	    state->widget = create_widget_info(state->interface, attrs);
	    info->child = state->widget;
	    info->child->parent = parent;

	    state->widget_depth++;
	    state->prop_type = PROP_NONE;
	    state->prop_name = NULL;
	    state->props = NULL;
	    state->signals = NULL;
	    state->accels = NULL;

	    state->state = PARSER_WIDGET;
	} else if (!xmlStrcmp(name, (xmlChar *)"placeholder")) {
	    /* this isn't a real child, so knock off  the last ChildInfo */
	    state->widget->n_children--;
	    state->state = PARSER_WIDGET_CHILD_PLACEHOLDER;
	} else {
	    g_warning("Unexpected element <%s> inside <child>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_CHILD_AFTER_WIDGET:
	if (!xmlStrcmp(name, (xmlChar *)"packing")) {
	    state->state = PARSER_WIDGET_CHILD_PACKING;
	} else {
	    g_warning("Unexpected element <%s> inside <child>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_CHILD_PACKING:
	if (!xmlStrcmp(name, (xmlChar *)"property")) {
	    gboolean bad_agent = FALSE;

	    if (state->prop_type != PROP_NONE &&
		state->prop_type != PROP_CHILD)
		g_warning("non child properties defined here (oh no!)");
	    state->translate_prop = FALSE;
	    for (i = 0; attrs && attrs[i] != NULL; i += 2) {
		if (!xmlStrcmp(attrs[i], (xmlChar *)"name"))
		    state->prop_name = alloc_propname(state->interface,
						      (gchar *)attrs[i+1]);
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"translatable"))
		    state->translate_prop = !xmlStrcmp(attrs[i+1], (xmlChar *)"yes");
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"context"))
		    state->context_prop = !xmlStrcmp(attrs[i+1], (xmlChar *)"yes");
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"agent"))
		    bad_agent = xmlStrcmp(attrs[i], (xmlChar *)"libglade") != 0;
		else if (!xmlStrcmp(attrs[i], (xmlChar *)"comments"))
		    /* Do nothing. */;
		else
		    g_warning("unknown attribute `%s' for <property>.",
			      attrs[i]);
	    }
	    if (bad_agent) {
		/* ignore the property ... */
		state->prev_state = state->state;
		state->state = PARSER_UNKNOWN;
		state->unknown_depth++;
	    } else {
		state->prop_type = PROP_CHILD;
		state->state = PARSER_WIDGET_CHILD_PACKING_PROPERTY;
	    }
	} else {
	    g_warning("Unexpected element <%s> inside <child>.", name);
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET_CHILD_PACKING_PROPERTY:
	g_warning("<property> element should be empty.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET_CHILD_AFTER_PACKING:
	g_warning("<child> should have no elements after <packing>.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET_CHILD_PLACEHOLDER:
	g_warning("<placeholder> should be empty.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_WIDGET_CHILD_AFTER_PLACEHOLDER:
	/* this is a placeholder <child> element -- ignore extra elements */
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_FINISH:
	g_warning("There should be no elements here.  Found <%s>.", name);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_UNKNOWN:
	state->unknown_depth++;
	break;
    }
    /* truncate the content string ... */
    g_string_truncate(state->content, 0);
}

static void
glade_parser_end_element(GladeParseState *state, const xmlChar *name)
{
    GladeProperty prop;

    GLADE_NOTE(PARSER, g_message("</%s> in state %s",
				 name, state_names[state->state]));

    switch (state->state) {
    case PARSER_START:
	g_warning("should not be closing any elements in this state");
	break;
    case PARSER_GLADE_INTERFACE:
	if (xmlStrcmp(name, (xmlChar *)"glade-interface") != 0)
	    g_warning("should find </glade-interface> here.  Found </%s>",
		      name);
	state->state = PARSER_FINISH;
	break;
    case PARSER_REQUIRES:
	if (xmlStrcmp(name, (xmlChar *)"requires") != 0)
	    g_warning("should find </requires> here.  Found </%s>", name);
	state->state = PARSER_GLADE_INTERFACE;
	break;
    case PARSER_WIDGET:
    case PARSER_WIDGET_AFTER_ATK:
    case PARSER_WIDGET_AFTER_SIGNAL:
    case PARSER_WIDGET_AFTER_ACCEL:
	if (xmlStrcmp(name, (xmlChar *)"widget") != 0)
	    g_warning("should find </widget> here.  Found </%s>", name);
	flush_properties(state);
	flush_signals(state);
	flush_actions(state);
	flush_relations(state);
	flush_accels(state);
	state->widget = state->widget->parent;
	state->widget_depth--;

	if (state->widget_depth == 0)
	    state->state = PARSER_GLADE_INTERFACE;
	else
	    state->state = PARSER_WIDGET_CHILD_AFTER_WIDGET;
	break;
    case PARSER_WIDGET_PROPERTY:
	if (xmlStrcmp(name, (xmlChar *)"property") != 0)
	    g_warning("should find </property> here.  Found </%s>", name);
	if (!state->props)
	    state->props = g_array_new(FALSE, FALSE, sizeof(GladeProperty));
	prop.name = state->prop_name;
 	if (state->translate_prop && state->content->str[0] != '\0') {
	    if (state->context_prop) 
		prop.value = alloc_string(state->interface,
					  (xmlChar *)g_strip_context(state->content->str,
							   dgettext(state->domain, state->content->str)));
	    else
		prop.value = alloc_string(state->interface,
					  (xmlChar *)dgettext(state->domain, state->content->str));
 	} else {
 	    prop.value = alloc_string(state->interface, (xmlChar *)state->content->str);
  	}
	g_array_append_val(state->props, prop);
	state->prop_name = NULL;
	state->state = PARSER_WIDGET;
	break;
    case PARSER_WIDGET_ATK:
	if (xmlStrcmp(name, (xmlChar *)"accessibility") != 0)
	    g_warning("should find </accessibility> here.  Found </%s>", name);
	flush_properties(state); /* flush the ATK properties */
	state->state = PARSER_WIDGET_AFTER_ATK;
	break;
    case PARSER_WIDGET_ATK_PROPERTY:
	if (xmlStrcmp(name, (xmlChar *)"atkproperty") != 0)
	    g_warning("should find </atkproperty> here.  Found </%s>", name);
	if (!state->props)
	    state->props = g_array_new(FALSE, FALSE, sizeof(GladeProperty));
	prop.name = state->prop_name;
 	if (state->translate_prop && state->content->str[0] != '\0') {
	    if (state->context_prop) 
		prop.value = alloc_string(state->interface,
					  (xmlChar *)g_strip_context(state->content->str,
							   dgettext(state->domain, state->content->str)));
	    else
		prop.value = alloc_string(state->interface,
					  (xmlChar *)dgettext(state->domain, state->content->str));
 	} else {
 	    prop.value = alloc_string(state->interface, (xmlChar *)state->content->str);
  	}
	g_array_append_val(state->props, prop);
	state->prop_name = NULL;
	state->state = PARSER_WIDGET_ATK;
	break;
    case PARSER_WIDGET_ATK_ACTION:
	if (xmlStrcmp(name, (xmlChar *)"atkaction") != 0)
	    g_warning("should find </atkaction> here.  Found </%s>", name);
        state->prop_name = NULL;
        state->state = PARSER_WIDGET_ATK;
        break;
    case PARSER_WIDGET_ATK_RELATION:
	if (xmlStrcmp(name, (xmlChar *)"atkrelation") != 0)
	    g_warning("should find </atkrelation> here.  Found </%s>", name);
        state->prop_name = NULL;
        state->state = PARSER_WIDGET_ATK;
        break;
    case PARSER_WIDGET_SIGNAL:
	if (xmlStrcmp(name, (xmlChar *)"signal") != 0)
	    g_warning("should find </signal> here.  Found </%s>", name);
	state->state = PARSER_WIDGET_AFTER_ATK;
	break;
    case PARSER_WIDGET_ACCEL:
	if (xmlStrcmp(name, (xmlChar *)"accelerator") != 0)
	    g_warning("should find </accelerator> here.  Found </%s>", name);
	state->state = PARSER_WIDGET_AFTER_SIGNAL;
	break;
    case PARSER_WIDGET_CHILD:
	if (xmlStrcmp(name, (xmlChar *)"child") != 0)
	    g_warning("should find </child> here.  Found </%s>", name);
	/* if we are ending the element in this state, then there
	 * hasn't been a <widget> element inside this <child>
	 * element. (If there was, then we would be in
	 * PARSER_WIDGET_CHILD_AFTER_WIDGET state. */
	g_warning("no <widget> element found inside <child>.  Discarding");
	g_free(state->widget->children[
			state->widget->n_children-1].properties);
	state->widget->n_children--;
	state->state = PARSER_WIDGET_AFTER_ACCEL;
	break;
    case PARSER_WIDGET_CHILD_AFTER_WIDGET:
	if (xmlStrcmp(name, (xmlChar *)"child") != 0)
	    g_warning("should find </child> here.  Found </%s>", name);
	state->state = PARSER_WIDGET_AFTER_ACCEL;
	break;
    case PARSER_WIDGET_CHILD_PACKING:
	if (xmlStrcmp(name, (xmlChar *)"packing") != 0)
	    g_warning("should find </packing> here.  Found </%s>", name);
	state->state = PARSER_WIDGET_CHILD_AFTER_PACKING;
	flush_properties(state); /* flush the properties. */
	break;
    case PARSER_WIDGET_CHILD_PACKING_PROPERTY:
	if (xmlStrcmp(name, (xmlChar *)"property") != 0)
	    g_warning("should find </property> here.  Found </%s>", name);
	if (!state->props)
	    state->props = g_array_new(FALSE, FALSE, sizeof(GladeProperty));
	prop.name = state->prop_name;
 	if (state->translate_prop && state->content->str[0] != '\0') {
	    if (state->context_prop) 
		prop.value = alloc_string(state->interface,
					  (xmlChar *)g_strip_context(state->content->str,
							   dgettext(state->domain, state->content->str)));
	    else
		prop.value = alloc_string(state->interface,
					  (xmlChar *)dgettext(state->domain, state->content->str));
 	} else {
 	    prop.value = alloc_string(state->interface, (xmlChar *)state->content->str);
  	}
	g_array_append_val(state->props, prop);
	state->prop_name = NULL;
	state->state = PARSER_WIDGET_CHILD_PACKING;
	break;
    case PARSER_WIDGET_CHILD_AFTER_PACKING:
	if (xmlStrcmp(name, (xmlChar *)"child") != 0)
	    g_warning("should find </child> here.  Found </%s>", name);
	state->state = PARSER_WIDGET_AFTER_ACCEL;
	break;
    case PARSER_WIDGET_CHILD_PLACEHOLDER:
	if (xmlStrcmp(name, (xmlChar *)"placeholder") != 0)
	    g_warning("should find </placeholder> here.  Found </%s>", name);
	state->state = PARSER_WIDGET_CHILD_AFTER_PLACEHOLDER;
	break;
    case PARSER_WIDGET_CHILD_AFTER_PLACEHOLDER:
	if (xmlStrcmp(name, (xmlChar *)"child") != 0)
	    g_warning("should find </child> here.  Found </%s>", name);
	state->state = PARSER_WIDGET_AFTER_ACCEL;
	break;
    case PARSER_FINISH:
	g_warning("should not be closing any elements in this state");
	break;
    case PARSER_UNKNOWN:
	state->unknown_depth--;
	if (state->unknown_depth == 0)
	    state->state = state->prev_state;
	break;
    }
}

static void
glade_parser_characters(GladeParseState *state, const xmlChar *chars, int len)
{
    switch (state->state) {
    case PARSER_WIDGET_PROPERTY:
    case PARSER_WIDGET_ATK_PROPERTY:
    case PARSER_WIDGET_CHILD_PACKING_PROPERTY:
	g_string_append_len(state->content, (gchar *)chars, len);
	break;
    default:
	/* don't care about content in any other states */
	break;
    }
}

static xmlEntityPtr
glade_parser_get_entity(GladeParseState *state, const xmlChar *name)
{
    return xmlGetPredefinedEntity(name);
}

static void
glade_parser_warning(GladeParseState *state, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_WARNING, msg, args);
    va_end(args);
}

static void
glade_parser_error(GladeParseState *state, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_CRITICAL, msg, args);
    va_end(args);
}

static void
glade_parser_fatal_error(GladeParseState *state, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_ERROR, msg, args);
    va_end(args);
}

static xmlSAXHandler glade_parser = {
    (internalSubsetSAXFunc)NULL,                       /* internalSubset */
    (isStandaloneSAXFunc)NULL,                         /* isStandalone */
    (hasInternalSubsetSAXFunc)NULL,                    /* hasInternalSubset */
    (hasExternalSubsetSAXFunc)NULL,                    /* hasExternalSubset */
    (resolveEntitySAXFunc)NULL,                        /* resolveEntity */
    (getEntitySAXFunc)glade_parser_get_entity,         /* getEntity */
    (entityDeclSAXFunc)NULL,                           /* entityDecl */
    (notationDeclSAXFunc)NULL,                         /* notationDecl */
    (attributeDeclSAXFunc)NULL,                        /* attributeDecl */
    (elementDeclSAXFunc)NULL,                          /* elementDecl */
    (unparsedEntityDeclSAXFunc)NULL,                   /* unparsedEntityDecl */
    (setDocumentLocatorSAXFunc)NULL,                   /* setDocumentLocator */
    (startDocumentSAXFunc)glade_parser_start_document, /* startDocument */
    (endDocumentSAXFunc)glade_parser_end_document,     /* endDocument */
    (startElementSAXFunc)glade_parser_start_element,   /* startElement */
    (endElementSAXFunc)glade_parser_end_element,       /* endElement */
    (referenceSAXFunc)NULL,                            /* reference */
    (charactersSAXFunc)glade_parser_characters,        /* characters */
    (ignorableWhitespaceSAXFunc)NULL,               /* ignorableWhitespace */
    (processingInstructionSAXFunc)NULL,             /* processingInstruction */
    (commentSAXFunc)NULL,                              /* comment */
    (warningSAXFunc)glade_parser_warning,              /* warning */
    (errorSAXFunc)glade_parser_error,                  /* error */
    (fatalErrorSAXFunc)glade_parser_fatal_error,       /* fatalError */
};

static void
widget_info_free(GladeWidgetInfo *info)
{
    gint i;

    g_return_if_fail(info != NULL);

    g_free(info->properties);
    g_free(info->atk_props);
    g_free(info->signals);
    g_free(info->atk_actions);
    g_free(info->relations);
    g_free(info->accels);

    for (i = 0; i < info->n_children; i++) {
	g_free(info->children[i].properties);
	widget_info_free(info->children[i].child);
    }
    g_free(info->children);
    g_free(info);
}

/**
 * glade_interface_destroy
 * @interface: the GladeInterface structure.
 *
 * Frees a GladeInterface structure.
 */
void
glade_interface_destroy(GladeInterface *interface)
{
    gint i;

    g_return_if_fail(interface != NULL);

    /* free requirements */
    g_free(interface->requires);

    for (i = 0; i < interface->n_toplevels; i++)
	widget_info_free(interface->toplevels[i]);
    g_free(interface->toplevels);

    g_hash_table_destroy(interface->names);

    /* free the strings hash table.  The destroy notify will take care
     * of the strings. */
    g_hash_table_destroy(interface->strings);

    g_free(interface);
}

/**
 * glade_parser_parse_file
 * @file: the filename of the glade XML file.
 * @domain: the translation domain for the XML file.
 *
 * This function parses a Glade XML interface file to a GladeInterface
 * object (which is libglade's internal representation of the
 * interface data).
 *
 * Generally, user code won't need to call this function.  Instead, it
 * should go through the GladeXML interfaces.
 *
 * Returns: the GladeInterface structure for the XML file.
 */
GladeInterface *
glade_parser_parse_file(const gchar *file, const gchar *domain)
{
    GladeParseState state = { 0 };

    if (!g_file_test(file, G_FILE_TEST_IS_REGULAR)) {
	g_warning("could not find glade file '%s'", file);
	return NULL;
    }

    state.interface = NULL;
    if (domain)
	state.domain = domain;
    else
	state.domain = textdomain(NULL);

    if (xmlSAXUserParseFile(&glade_parser, &state, file) < 0) {
	g_warning("document not well formed");
	if (state.interface)
	    glade_interface_destroy (state.interface);
	return NULL;
    }
    if (state.state != PARSER_FINISH) {
	g_warning("did not finish in PARSER_FINISH state");
	if (state.interface)
	    glade_interface_destroy(state.interface);
	return NULL;
    }
    return state.interface;
}

/**
 * glade_parser_parse_buffer
 * @buffer: a buffer in memory containing XML data.
 * @len: the length of @buffer.
 * @domain: the translation domain for the XML file.
 *
 * This function is similar to glade_parser_parse_file, except that it
 * parses XML data from a buffer in memory.  This could be used to
 * embed an interface into the executable, for instance.
 *
 * Generally, user code won't need to call this function.  Instead, it
 * should go through the GladeXML interfaces.
 *
 * Returns: the GladeInterface structure for the XML buffer.
 */
GladeInterface *
glade_parser_parse_buffer(const gchar *buffer, gint len, const gchar *domain)
{
    GladeParseState state = { 0 };

    state.interface = NULL;
    if (domain)
	state.domain = domain;
    else
	state.domain = textdomain(NULL);

    if (xmlSAXUserParseMemory(&glade_parser, &state, buffer, len) < 0) {
	g_warning("document not well formed!");
	if (state.interface)
	    glade_interface_destroy (state.interface);
	return NULL;
    }
    if (state.state != PARSER_FINISH) {
	g_warning("did not finish in PARSER_FINISH state!");
	if (state.interface)
	    glade_interface_destroy(state.interface);
	return NULL;
    }
    return state.interface;
}

static void
dump_widget(xmlNode *parent, GladeWidgetInfo *info, gint indent)
{
    xmlNode *widget = xmlNewNode(NULL, (xmlChar *)"widget");
    gint i, j;

    xmlSetProp(widget, (xmlChar *)"class", (xmlChar *)info->classname);
    xmlSetProp(widget, (xmlChar *)"id", (xmlChar *)info->name);
    xmlAddChild(parent, widget);
    xmlNodeAddContent(widget, (xmlChar *)"\n");

    for (i = 0; i < info->n_properties; i++) { 
	xmlNode *node;

	for (j = 0; j < indent + 1; j++)
	    xmlNodeAddContent(widget, (xmlChar *)"  ");
	node = xmlNewNode(NULL, (xmlChar *)"property");
	xmlSetProp(node, (xmlChar *)"name", (xmlChar *)info->properties[i].name);
	xmlNodeSetContent(node, (xmlChar *)info->properties[i].value);
	xmlAddChild(widget, node);
	xmlNodeAddContent(widget, (xmlChar *)"\n");
    }

    if (info->n_atk_props != 0) {
	xmlNode *atk;

	for (j = 0; j < indent + 1; j++)
	    xmlNodeAddContent(widget, (xmlChar *)"  ");
	atk = xmlNewNode(NULL, (xmlChar *)"accessibility");
	xmlAddChild(widget, atk);
	xmlNodeAddContent(widget, (xmlChar *)"\n");
	xmlNodeAddContent(atk, (xmlChar *)"\n");

	for (i = 0; i < info->n_atk_props; i++) {
	    xmlNode *node;

	    for (j = 0; j < indent + 2; j++)
		xmlNodeAddContent(atk, (xmlChar *)"  ");
	    node = xmlNewNode(NULL, (xmlChar *)"property");
	    xmlSetProp(node, (xmlChar *)"name", (xmlChar *)info->atk_props[i].name);
	    xmlNodeSetContent(node, (xmlChar *)info->atk_props[i].value);
	    xmlAddChild(atk, node);
	    xmlNodeAddContent(atk, (xmlChar *)"\n");
	}
	for (j = 0; j < indent + 1; j++)
	    xmlNodeAddContent(atk, (xmlChar *)"  ");
    }

    for (i = 0; i < info->n_signals; i++) {
	xmlNode *node;

	for (j = 0; j < indent + 1; j++)
	    xmlNodeAddContent(widget, (xmlChar *)"  ");

	node = xmlNewNode(NULL, (xmlChar *)"signal");
	xmlSetProp(node, (xmlChar *)"name", (xmlChar *)info->signals[i].name);
	xmlSetProp(node, (xmlChar *)"handler", (xmlChar *)info->signals[i].handler);
	if (info->signals[i].after)
	    xmlSetProp(node, (xmlChar *)"after", (xmlChar *)"yes");
	if (info->signals[i].object)
	    xmlSetProp(node, (xmlChar *)"object", (xmlChar *)info->signals[i].object);
	xmlAddChild(widget, node);
	xmlNodeAddContent(widget, (xmlChar *)"\n");
    }

    for (i = 0; i < info->n_accels; i++) {
	xmlNode *node;

	for (j = 0; j < indent + 1; j++)
	    xmlNodeAddContent(widget, (xmlChar *)"  ");

	node = xmlNewNode(NULL, (xmlChar *)"accelerator");
	xmlSetProp(node, (xmlChar *)"key", (xmlChar *)gdk_keyval_name(info->accels[i].key));
	xmlSetProp(node, (xmlChar *)"modifier", (xmlChar *)"something"/*info->accels[i].modifiers*/);
	xmlSetProp(node, (xmlChar *)"signal", (xmlChar *)info->accels[i].signal);
	xmlAddChild(widget, node);
	xmlNodeAddContent(widget, (xmlChar *)"\n");
    }

    for (i = 0; i < info->n_children; i++) {
	xmlNode *child;
	GladeChildInfo *childinfo = &info->children[i];
	gint k;

	for (j = 0; j < indent + 1; j++)
	    xmlNodeAddContent(widget, (xmlChar *)"  ");

	child = xmlNewNode(NULL, (xmlChar *)"child");
	if (childinfo->internal_child)
	    xmlSetProp(child, (xmlChar *)"internal-child", (xmlChar *)childinfo->internal_child);
	xmlAddChild(widget, child);
	xmlNodeAddContent(widget, (xmlChar *)"\n");
	xmlNodeAddContent(child, (xmlChar *)"\n");

	for (k = 0; k < childinfo->n_properties; k++) { 
	    xmlNode *node;

	    for (j = 0; j < indent + 2; j++)
		xmlNodeAddContent(child, (xmlChar *)"  ");
	    node = xmlNewNode(NULL, (xmlChar *)"property");
	    xmlSetProp(node, (xmlChar *)"name", (xmlChar *)childinfo->properties[i].name);
	    xmlNodeSetContent(node, (xmlChar *)childinfo->properties[i].value);
	    xmlAddChild(child, node);
	    xmlNodeAddContent(child, (xmlChar *)"\n");
	}

	for (j = 0; j < indent + 2; j++)
	    xmlNodeAddContent(child, (xmlChar *)"  ");
	dump_widget(child, childinfo->child, indent + 2);
	xmlNodeAddContent(child, (xmlChar *)"\n");

	for (j = 0; j < indent + 1; j++)
	    xmlNodeAddContent(child, (xmlChar *)"  ");
    }

    for (j = 0; j < indent; j++)
	xmlNodeAddContent(widget, (xmlChar *)"  ");
}

/**
 * glade_interface_dump
 * @interface: the GladeInterface
 * @filename: the filename to write the interface data to.
 *
 * This function dumps the contents of a GladeInterface into a file as
 * XML.  It is intended mainly as a debugging tool.
 */
void
glade_interface_dump(GladeInterface *interface, const gchar *filename)
{
    xmlDoc *doc;
    xmlNode *root;
    gint i;

    doc = xmlNewDoc((xmlChar *)"1.0");
    doc->standalone = FALSE;
    xmlCreateIntSubset(doc, (xmlChar *)"glade-interface",
		       NULL, (xmlChar *)"glade-2.0.dtd");
    root = xmlNewNode(NULL, (xmlChar *)"glade-interface");
    xmlDocSetRootElement(doc, root);

    xmlNodeAddContent(root, (xmlChar *)"\n");

    for (i = 0; i < interface->n_requires; i++) {
	xmlNode *node = xmlNewNode(NULL, (xmlChar *)"requires");

	xmlSetProp(node, (xmlChar *)"lib", (xmlChar *)interface->requires[i]);

	xmlNodeAddContent(root, (xmlChar *)"  ");
	xmlAddChild(root, node);
	xmlNodeAddContent(root, (xmlChar *)"\n");
    }

    for (i = 0; i < interface->n_toplevels; i++) {
	xmlNodeAddContent(root, (xmlChar *)"  ");
	dump_widget(root, interface->toplevels[i], 1);
	xmlNodeAddContent(root, (xmlChar *)"\n");
    }

    /* output */
    
    xmlSaveFileEnc(filename, doc, "UTF-8");
    xmlFreeDoc(doc);
}

#if 0
int
main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    if (argc > 1) {
	GladeInterface *interface = glade_parser_parse_file(argv[1]);
	g_message("output: %p", interface);
	if (interface) {
	    glade_interface_dump(interface, "/dev/stdout");
	    glade_interface_destroy(interface);
	}
    } else
	g_message("need filename");
    return 0;
}
#endif
