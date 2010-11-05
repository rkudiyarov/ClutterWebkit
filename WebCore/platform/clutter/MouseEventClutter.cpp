/*
* Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "PlatformMouseEvent.h"

#include "SystemTime.h"
#include "Assertions.h"

#include <clutter/clutter.h>

namespace WebCore {

// FIXME: Would be even better to figure out which modifier is Alt instead of always using GDK_MOD1_MASK.

// Keep this in sync with the other platform event constructors
PlatformMouseEvent::PlatformMouseEvent(ClutterButtonEvent* event)
{
    gfloat ux, uy;

    if (clutter_actor_transform_stage_point(event->source,
					    event->x,
					    event->y,
					    &ux, &uy)) {
	m_position = IntPoint((int)ux,
			      (int)uy);
    } else {
	m_position = IntPoint((int)event->x, (int)event->y);
    }

    m_timestamp = event->time;
    // m_globalPosition = IntPoint((int)event->x_root, (int)event->y_root);
    m_shiftKey = event->modifier_state & CLUTTER_SHIFT_MASK;
    m_ctrlKey = event->modifier_state & CLUTTER_CONTROL_MASK;
    m_altKey = event->modifier_state & CLUTTER_MOD1_MASK;
    m_metaKey = false;

    switch (event->type) {
    case CLUTTER_BUTTON_PRESS:
    case CLUTTER_BUTTON_RELEASE:
        m_eventType = MouseEventPressed;
	m_clickCount = event->click_count;
        if (event->type == CLUTTER_BUTTON_RELEASE)
	    m_eventType = MouseEventReleased;

        if (event->button == 1)
            m_button = LeftButton;
        else if (event->button == 2)
            m_button = MiddleButton;
        else if (event->button == 3)
            m_button = RightButton;
        break;

    default:
        ASSERT_NOT_REACHED();
    };
}

PlatformMouseEvent::PlatformMouseEvent(ClutterMotionEvent* event)
{
    gfloat ux, uy;

    if (clutter_actor_transform_stage_point(event->source,
					    event->x,
					    event->y,
					    &ux, &uy)) {
	m_position = IntPoint((int)ux,
			      (int)uy);
    } else {
	m_position = IntPoint((int)event->x, (int)event->y);
    }

    m_timestamp = event->time;
    // m_globalPosition = IntPoint((int)event->x_root, (int)event->y_root);
    m_shiftKey = event->modifier_state & CLUTTER_SHIFT_MASK;
    m_ctrlKey = event->modifier_state & CLUTTER_CONTROL_MASK;
    m_altKey = event->modifier_state & CLUTTER_MOD1_MASK;
    m_metaKey = event->modifier_state & CLUTTER_MOD2_MASK;

    switch (event->type) {
    case CLUTTER_MOTION:
        m_eventType = MouseEventMoved;
        m_button = NoButton;
        m_clickCount = 0;
        break;
    default:
        ASSERT_NOT_REACHED();
    };

    if (event->modifier_state & CLUTTER_BUTTON1_MASK)
        m_button = LeftButton;
    else if (event->modifier_state & CLUTTER_BUTTON2_MASK)
        m_button = MiddleButton;
    else if (event->modifier_state & CLUTTER_BUTTON3_MASK)
        m_button = RightButton;
}
}
