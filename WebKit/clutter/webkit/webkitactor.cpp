/*
 * Copyright (C) 2008 Openedhand Ltd
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <clutter/clutter.h>

#include "webkitactor.h"

extern "C" {

G_DEFINE_TYPE (WebkitActor, webkit_actor, CLUTTER_TYPE_CAIRO_TEXTURE);

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), WEBKIT_TYPE_ACTOR, WebkitActorPrivate))

enum {
    START_EDITING,
    STOP_EDITING,
    LAST_SIGNAL
};

typedef struct _WebkitActorPrivate {
    WebkitActorRectangle dirty_region;
    guint redraw_idle_id;
} WebkitActorPrivate;

guint32 signals[LAST_SIGNAL] = {0, };

static void
webkit_actor_class_init (WebkitActorClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;

    g_type_class_add_private (klass, sizeof (WebkitActorPrivate));

    signals[START_EDITING] = g_signal_new ("start-editing",
					   G_TYPE_FROM_CLASS (o_class),
					   (GSignalFlags) (G_SIGNAL_NO_RECURSE |
					   G_SIGNAL_RUN_LAST),
					   G_STRUCT_OFFSET (WebkitActorClass,
							    start_editing),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__VOID,
					   G_TYPE_NONE, 0);
    signals[STOP_EDITING] = g_signal_new ("stop-editing",
					  G_TYPE_FROM_CLASS (o_class),
					  (GSignalFlags) (G_SIGNAL_NO_RECURSE |
							  G_SIGNAL_RUN_LAST),
					  G_STRUCT_OFFSET (WebkitActorClass,
							   stop_editing),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE, 0);
}

static void
webkit_actor_init (WebkitActor *actor)
{
}

void
webkit_actor_expose (WebkitActor          *actor,
		     WebkitActorRectangle *rect)
{
    WebkitActorClass *klass = WEBKIT_ACTOR_GET_CLASS (actor);

    if (klass->expose)
        klass->expose (actor, rect);
}

static gboolean
redraw_idle_cb (gpointer data)
{
    WebkitActorPrivate *priv = GET_PRIVATE (data);

    webkit_actor_expose (WEBKIT_ACTOR (data), &priv->dirty_region);

    priv->redraw_idle_id = 0;
    return FALSE;
}

static void
union_rectangles (WebkitActorRectangle *a,
		  WebkitActorRectangle *b,
		  WebkitActorRectangle *dest)
{
    int dest_x, dest_y;

    dest_x = MIN (a->x, b->x);
    dest_y = MIN (a->y, b->y);
    dest->width = MAX (a->x + a->width, b->x + b->width) - dest_x;
    dest->height = MAX (a->y + a->height, b->y + b->height) - dest_y;
    dest->x = dest_x;
    dest->y = dest_y;
}

void
webkit_actor_queue_expose (WebkitActor          *actor,
			   WebkitActorRectangle *rect)
{
    WebkitActorPrivate *priv = GET_PRIVATE (actor);
    
    // If queue_expose is called with a NULL rectangle, draw the damaged
    // region immediately
    if (!rect) {
        if (priv->redraw_idle_id) {
            g_source_remove (priv->redraw_idle_id);
            priv->redraw_idle_id = 0;
            webkit_actor_expose (actor, &priv->dirty_region);
        }
        return;
    }
    
    if (priv->redraw_idle_id == 0) {
	priv->dirty_region.x = rect->x;
	priv->dirty_region.y = rect->y;
	priv->dirty_region.width = rect->width;
	priv->dirty_region.height = rect->height;

	priv->redraw_idle_id = g_idle_add_full (G_PRIORITY_HIGH, redraw_idle_cb, actor, NULL);
    } else {
	union_rectangles (&priv->dirty_region, rect, &priv->dirty_region);
    }
}

void
webkit_actor_focus (WebkitActor *actor)
{
    WebkitActorClass *klass = WEBKIT_ACTOR_GET_CLASS (actor);

    if (klass->focus) {
	klass->focus (actor);
    }    
}

void
webkit_actor_start_editing (WebkitActor *actor)
{
    g_signal_emit (actor, signals[START_EDITING], 0);
}

void
webkit_actor_stop_editing (WebkitActor *actor)
{
    g_signal_emit (actor, signals[STOP_EDITING], 0);
}

}
