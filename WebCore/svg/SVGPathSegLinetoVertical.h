/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>
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

#ifndef SVGPathSegLinetoVertical_h
#define SVGPathSegLinetoVertical_h

#if ENABLE(SVG)

#include "PlatformString.h"
#include "SVGPathSeg.h"

namespace WebCore {

    class SVGPathSegLinetoVertical : public SVGPathSeg {
    public:
        SVGPathSegLinetoVertical(float y) : SVGPathSeg(), m_y(y) {}

        void setY(float y) { m_y = y; }
        float y() const { return m_y; }

    private:
        float m_y;
    };

    class SVGPathSegLinetoVerticalAbs : public SVGPathSegLinetoVertical {
    public:
        static PassRefPtr<SVGPathSegLinetoVerticalAbs> create(float y) { return adoptRef(new SVGPathSegLinetoVerticalAbs(y)); }

    private:
        SVGPathSegLinetoVerticalAbs(float y);

        virtual unsigned short pathSegType() const { return PATHSEG_LINETO_VERTICAL_ABS; }
        virtual String pathSegTypeAsLetter() const { return "V"; }
    };

    class SVGPathSegLinetoVerticalRel : public SVGPathSegLinetoVertical {
    public:
        static PassRefPtr<SVGPathSegLinetoVerticalRel> create(float y) { return adoptRef(new SVGPathSegLinetoVerticalRel(y)); }

    private:
        SVGPathSegLinetoVerticalRel(float y);

        virtual unsigned short pathSegType() const { return PATHSEG_LINETO_VERTICAL_REL; }
        virtual String pathSegTypeAsLetter() const { return "v"; }
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

// vim:ts=4:noet
