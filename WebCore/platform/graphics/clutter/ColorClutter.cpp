/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Color.h"

#include <clutter/clutter.h>

namespace WebCore {

Color::Color(const ClutterColor& c)
    : m_color(makeRGBA(c.red, c.green, c.blue, c.alpha))
    , m_valid(true)
{
}

Color::operator ClutterColor() const
{
    ClutterColor col;
    if (m_valid) {
        col.red = red();
        col.green = green();
        col.blue = blue();
        col.alpha = alpha();
    } else {
        col.red = 0;
        col.green = 0;
        col.blue = 0;
        col.alpha = 0;
    }
        
    return col;
}

}

// vim: ts=4 sw=4 et
