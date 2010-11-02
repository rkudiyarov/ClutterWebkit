/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ScrollbarThemeClutter.h"

#include "PlatformMouseEvent.h"
#include "RenderThemeClutter.h"
#include "ScrollView.h"
#include "Scrollbar.h"


namespace WebCore {

static HashSet<Scrollbar*>* gScrollbars;
static void gtkStyleSetCallback(ClutterWidget*, ClutterStyle*, ScrollbarThemeClutter*);

ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
    static ScrollbarThemeClutter theme;
    return &theme;
}

ScrollbarThemeClutter::ScrollbarThemeClutter()
{
    updateThemeProperties();
}

ScrollbarThemeClutter::~ScrollbarThemeClutter()
{
}

void ScrollbarThemeClutter::registerScrollbar(Scrollbar* scrollbar)
{
    if (!gScrollbars)
        gScrollbars = new HashSet<Scrollbar*>;
    gScrollbars->add(scrollbar);
}

void ScrollbarThemeClutter::unregisterScrollbar(Scrollbar* scrollbar)
{
    gScrollbars->remove(scrollbar);
    if (gScrollbars->isEmpty()) {
        delete gScrollbars;
        gScrollbars = 0;
    }
}

void ScrollbarThemeClutter::updateThemeProperties()
{
    notImplemented();
}

bool ScrollbarThemeClutter::hasThumb(Scrollbar* scrollbar)
{
    // This method is just called as a paint-time optimization to see if
    // painting the thumb can be skipped.  We don't have to be exact here.
    return thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeClutter::backButtonRect(Scrollbar* scrollbar, ScrollbarPart part, bool)
{
    if (part == BackButtonEndPart && !m_hasBackButtonEndPart)
        return IntRect();

    int x = scrollbar->x() + m_troughBorderWidth;
    int y = scrollbar->y() + m_troughBorderWidth;
    IntSize size = buttonSize(scrollbar);
    if (part == BackButtonStartPart)
        return IntRect(x, y, size.width(), size.height());

    // BackButtonEndPart (alternate button)
    if (scrollbar->orientation() == HorizontalScrollbar)
        return IntRect(scrollbar->x() + scrollbar->width() - m_troughBorderWidth - (2 * size.width()), y, size.width(), size.height());

    // VerticalScrollbar alternate button
    return IntRect(x, scrollbar->y() + scrollbar->height() - m_troughBorderWidth - (2 * size.height()), size.width(), size.height());
}

IntRect ScrollbarThemeClutter::forwardButtonRect(Scrollbar* scrollbar, ScrollbarPart part, bool)
{
    if (part == ForwardButtonStartPart && !m_hasForwardButtonStartPart)
        return IntRect();

    IntSize size = buttonSize(scrollbar);
    if (scrollbar->orientation() == HorizontalScrollbar) {
        int y = scrollbar->y() + m_troughBorderWidth;
        if (part == ForwardButtonEndPart)
            return IntRect(scrollbar->x() + scrollbar->width() - size.width() - m_troughBorderWidth, y, size.width(), size.height());

        // ForwardButtonStartPart (alternate button)
        return IntRect(scrollbar->x() + m_troughBorderWidth + size.width(), y, size.width(), size.height());
    }

    // VerticalScrollbar
    int x = scrollbar->x() + m_troughBorderWidth;
    if (part == ForwardButtonEndPart)
        return IntRect(x, scrollbar->y() + scrollbar->height() - size.height() - m_troughBorderWidth, size.width(), size.height());

    // ForwardButtonStartPart (alternate button)
    return IntRect(x, scrollbar->y() + m_troughBorderWidth + size.height(), size.width(), size.height());
}

IntRect ScrollbarThemeClutter::trackRect(Scrollbar* scrollbar, bool)
{
    // The padding along the thumb movement axis (from outside to in)
    // is the size of trough border plus the size of the stepper (button)
    // plus the size of stepper spacing (the space between the stepper and
    // the place where the thumb stops). There is often no stepper spacing.
    int movementAxisPadding = m_troughBorderWidth + m_stepperSize + m_stepperSpacing;

    // The fatness of the scrollbar on the non-movement axis.
    int thickness = scrollbarThickness(scrollbar->controlSize());

    int alternateButtonOffset = 0;
    int alternateButtonWidth = 0;
    if (m_hasForwardButtonStartPart) {
        alternateButtonOffset += m_stepperSize;
        alternateButtonWidth += m_stepperSize;
    }
    if (m_hasBackButtonEndPart)
        alternateButtonWidth += m_stepperSize;

    if (scrollbar->orientation() == HorizontalScrollbar) {
        // Once the scrollbar becomes smaller than the natural size of the
        // two buttons, the track disappears.
        if (scrollbar->width() < 2 * thickness)
            return IntRect();
        return IntRect(scrollbar->x() + movementAxisPadding + alternateButtonOffset, scrollbar->y(),
                       scrollbar->width() - (2 * movementAxisPadding) - alternateButtonWidth, thickness);
    }

    if (scrollbar->height() < 2 * thickness)
        return IntRect();
    return IntRect(scrollbar->x(), scrollbar->y() + movementAxisPadding + alternateButtonOffset,
                   thickness, scrollbar->height() - (2 * movementAxisPadding) - alternateButtonWidth);
}

void ScrollbarThemeClutter::paintTrackBackground(GraphicsContext* context, Scrollbar* scrollbar, const IntRect& rect)
{
    notImplemented();
}

void ScrollbarThemeClutter::paintScrollbarBackground(GraphicsContext* context, Scrollbar* scrollbar)
{
    notImplemented();
}

void ScrollbarThemeClutter::paintThumb(GraphicsContext* context, Scrollbar* scrollbar, const IntRect& rect)
{
    notImplemented();
}

IntRect ScrollbarThemeClutter::thumbRect(Scrollbar* scrollbar, const IntRect& unconstrainedTrackRect)
{
    IntRect trackRect = constrainTrackRectToTrackPieces(scrollbar, unconstrainedTrackRect);
    int thumbPos = thumbPosition(scrollbar);
    if (scrollbar->orientation() == HorizontalScrollbar)
        return IntRect(trackRect.x() + thumbPos, trackRect.y() + (trackRect.height() - m_thumbFatness) / 2, thumbLength(scrollbar), m_thumbFatness); 

    // VerticalScrollbar
    return IntRect(trackRect.x() + (trackRect.width() - m_thumbFatness) / 2, trackRect.y() + thumbPos, m_thumbFatness, thumbLength(scrollbar));
}

bool ScrollbarThemeClutter::paint(Scrollbar* scrollbar, GraphicsContext* graphicsContext, const IntRect& damageRect)
{
    notImplemented();

    return false;
}

void ScrollbarThemeClutter::paintButton(GraphicsContext* context, Scrollbar* scrollbar, const IntRect& rect, ScrollbarPart part)
{
    notImplemented();
}

void ScrollbarThemeClutter::paintScrollCorner(ScrollView* view, GraphicsContext* context, const IntRect& cornerRect)
{
    // ScrollbarThemeComposite::paintScrollCorner incorrectly assumes that the
    // ScrollView is a FrameView (see FramelessScrollView), so we cannot let
    // that code run.  For FrameView's this is correct since we don't do custom
    // scrollbar corner rendering, which ScrollbarThemeComposite supports.
    ScrollbarTheme::paintScrollCorner(view, context, cornerRect);
}

bool ScrollbarThemeClutter::shouldCenterOnThumb(Scrollbar*, const PlatformMouseEvent& event)
{
    return (event.shiftKey() && event.button() == LeftButton) || (event.button() == MiddleButton);
}

int ScrollbarThemeClutter::scrollbarThickness(ScrollbarControlSize)
{
    return m_thumbFatness + (m_troughBorderWidth * 2);
}

IntSize ScrollbarThemeClutter::buttonSize(Scrollbar* scrollbar)
{
    if (scrollbar->orientation() == VerticalScrollbar)
        return IntSize(m_thumbFatness, m_stepperSize);

    // HorizontalScrollbar
    return IntSize(m_stepperSize, m_thumbFatness);
}

int ScrollbarThemeClutter::minimumThumbLength(Scrollbar* scrollbar)
{
    return m_minThumbLength;
}

}

