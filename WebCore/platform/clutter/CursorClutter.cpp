/*
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Christian Dywan <christian@twotoasts.de>
 * Copyright (C) 2010 Igalia S.L.
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
#include "CursorClutter.h"

#include "Image.h"
#include "IntPoint.h"
#include "PlatformRefPtrCairo.h"
#include <wtf/Assertions.h>

#include <clutter/clutter.h>

namespace WebCore {

void Cursor::ensurePlatformCursor() const
{
    if (m_platformCursor || m_type == Cursor::Pointer)
        return;

    switch (m_type) {
    case Cursor::Pointer:
        // A null GdkCursor is the default cursor for the window.
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Cross:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Hand:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::IBeam:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Wait:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Help:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Move:
    case Cursor::MiddlePanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::EastResize:
    case Cursor::EastPanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::NorthResize:
    case Cursor::NorthPanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::NorthEastResize:
    case Cursor::NorthEastPanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::NorthWestResize:
    case Cursor::NorthWestPanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::SouthResize:
    case Cursor::SouthPanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::SouthEastResize:
    case Cursor::SouthEastPanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::SouthWestResize:
    case Cursor::SouthWestPanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::WestResize:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::NorthSouthResize:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::EastWestResize:
    case Cursor::WestPanning:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::NorthEastSouthWestResize:
    case Cursor::NorthWestSouthEastResize:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::ColumnResize:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::RowResize:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::VerticalText:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Cell:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::ContextMenu:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Alias:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Progress:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::NoDrop:
    case Cursor::NotAllowed:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Copy:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::None:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::ZoomIn:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::ZoomOut:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Grab:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Grabbing:
        m_platformCursor = clutter_rectangle_new();
        break;
    case Cursor::Custom:
        m_platformCursor = clutter_rectangle_new();
        break;
    }
}

Cursor::Cursor(const Cursor& other)
    : m_type(other.m_type)
    , m_image(other.m_image)
    , m_hotSpot(other.m_hotSpot)
    , m_platformCursor(other.m_platformCursor)
{
}

Cursor& Cursor::operator=(const Cursor& other)
{
    m_type = other.m_type;
    m_image = other.m_image;
    m_hotSpot = other.m_hotSpot;
    m_platformCursor = other.m_platformCursor;
    return *this;
}

Cursor::~Cursor()
{
    if (m_platformCursor) {
        
    }
}

}
