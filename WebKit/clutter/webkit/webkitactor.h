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

#ifndef _WEBKIT_ACTOR_H_
#define _WEBKIT_ACTOR_H_

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_ACTOR (webkit_actor_get_type ())
#define WEBKIT_ACTOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBKIT_TYPE_ACTOR, WebkitActor))
#define WEBKIT_ACTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), WEBKIT_TYPE_ACTOR, WebkitActorClass))
#define WEBKIT_ACTOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WEBKIT_TYPE_ACTOR, WebkitActorClass))

typedef struct _WebkitActorRectangle {
    int x;
    int y;
    int width;
    int height;
} WebkitActorRectangle;

typedef struct _WebkitActorPoint {
    int x;
    int y;
} WebkitActorPoint;

typedef struct _WebkitActor {
    ClutterCairoTexture parent;
} WebkitActor;

typedef struct _WebkitActorClass {
    ClutterCairoTextureClass parent_class;

    void (* expose) (WebkitActor          *actor,
		     WebkitActorRectangle *rect);
    void (* focus) (WebkitActor *actor);
    void (* start_editing) (WebkitActor *actor);
    void (* stop_editing) (WebkitActor *actor);
} WebkitActorClass;

GType webkit_actor_get_type (void);
void webkit_actor_expose (WebkitActor          *actor,
			  WebkitActorRectangle *rect);
void webkit_actor_queue_expose (WebkitActor          *actor,
				WebkitActorRectangle *rect);
void webkit_actor_focus (WebkitActor *actor);
void webkit_actor_start_editing (WebkitActor *actor);
void webkit_actor_stop_editing (WebkitActor *actor);
G_END_DECLS

#endif