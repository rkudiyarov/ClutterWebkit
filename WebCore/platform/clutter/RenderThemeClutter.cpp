/*
 * Copyright (C) 2007 Apple Inc.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2009 Kenneth Rohde Christiansen
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
 *
 */

#include "config.h"
#include "RenderThemeClutter.h"

#include "AffineTransform.h"
#include "CSSValueKeywords.h"
#include "GOwnPtr.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "GtkVersioning.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "MediaControlElements.h"
#include "NotImplemented.h"
#include "PlatformMouseEvent.h"
#include "RenderBox.h"
#include "RenderObject.h"
#include "Scrollbar.h"
#include "TimeRanges.h"
#include "UserAgentStyleSheets.h"
#include <wtf/text/CString.h>

#if ENABLE(PROGRESS_TAG)
#include "RenderProgress.h"
#endif

namespace WebCore {

using namespace HTMLNames;

#if ENABLE(VIDEO)
static HTMLMediaElement* getMediaElementFromRenderObject(RenderObject* o)
{
    Node* node = o->node();
    Node* mediaNode = node ? node->shadowAncestorNode() : 0;
    if (!mediaNode || (!mediaNode->hasTagName(videoTag) && !mediaNode->hasTagName(audioTag)))
        return 0;

    return static_cast<HTMLMediaElement*>(mediaNode);
}

#endif

PassRefPtr<RenderTheme> RenderThemeClutter::create()
{
    return adoptRef(new RenderThemeClutter());
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    static RenderTheme* rt = RenderThemeClutter::create().releaseRef();
    return rt;
}

RenderThemeClutter::RenderThemeGtk()
    , m_panelColor(Color::white)
    , m_sliderColor(Color::white)
    , m_sliderThumbColor(Color::white)
    , m_mediaIconSize(16)
    , m_mediaSliderHeight(14)
    , m_mediaSliderThumbWidth(12)
    , m_mediaSliderThumbHeight(12)
    , m_fullscreenButton(0)
    , m_muteButton(0)
    , m_unmuteButton(0)
    , m_playButton(0)
    , m_pauseButton(0)
    , m_seekBackButton(0)
    , m_seekForwardButton(0)
{
    notImplemented();
}

RenderThemeClutter::~RenderThemeGtk()
{
    notImplemented();
}

static bool supportsFocus(ControlPart appearance)
{
    switch (appearance) {
    case PushButtonPart:
    case ButtonPart:
    case TextFieldPart:
    case TextAreaPart:
    case SearchFieldPart:
    case MenulistPart:
    case RadioPart:
    case CheckboxPart:
    case SliderHorizontalPart:
    case SliderVerticalPart:
        return true;
    default:
        return false;
    }
}

bool RenderThemeClutter::supportsFocusRing(const RenderStyle* style) const
{
    return supportsFocus(style->appearance());
}

bool RenderThemeClutter::controlSupportsTints(const RenderObject* o) const
{
    return isEnabled(o);
}

int RenderThemeClutter::baselinePosition(const RenderObject* o) const
{
    if (!o->isBox())
        return 0;

    // FIXME: This strategy is possibly incorrect for the GTK+ port.
    if (o->style()->appearance() == CheckboxPart
        || o->style()->appearance() == RadioPart) {
        const RenderBox* box = toRenderBox(o);
        return box->marginTop() + box->height() - 2;
    }

    return RenderTheme::baselinePosition(o);
}

void RenderThemeClutter::setCheckboxSize(RenderStyle* style) const
{
    notImplemented();
}

bool RenderThemeClutter::paintCheckbox(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    notImplemented();
    return false;
}

void RenderThemeClutter::setRadioSize(RenderStyle* style) const
{
    notImplemented();
}

bool RenderThemeClutter::paintRadio(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    notImplemented();
    return false;
}

void RenderThemeClutter::adjustButtonStyle(CSSStyleSelector* selector, RenderStyle* style, WebCore::Element* e) const
{
    // Some layout tests check explicitly that buttons ignore line-height.
    if (style->appearance() == PushButtonPart)
        style->setLineHeight(RenderStyle::initialLineHeight());
}

bool RenderThemeClutter::paintButton(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    notImplemented();
    return false;
}

void RenderThemeClutter::adjustMenuListStyle(CSSStyleSelector* selector, RenderStyle* style, WebCore::Element* e) const
{
    style->resetBorder();
    style->resetPadding();
    style->setHeight(Length(Auto));
    style->setWhiteSpace(PRE);
    notImplemented();
}

bool RenderThemeClutter::paintMenuList(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    notImplemented();
    return false;
}

void RenderThemeClutter::adjustTextFieldStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    style->resetBorder();
    style->resetPadding();
    style->setHeight(Length(Auto));
    style->setWhiteSpace(PRE);
    notImplemented();
}

bool RenderThemeClutter::paintTextField(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    notImplemented();
    return false;
}

bool RenderThemeClutter::paintTextArea(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    return paintTextField(o, i, r);
}

void RenderThemeClutter::adjustSearchFieldResultsButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    adjustSearchFieldCancelButtonStyle(selector, style, e);
}

bool RenderThemeClutter::paintSearchFieldResultsButton(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    return paintSearchFieldResultsDecoration(o, i, rect);
}

void RenderThemeClutter::adjustSearchFieldResultsDecorationStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    style->resetBorder();
    style->resetPadding();

    // FIXME: This should not be hard-coded.
    IntSize size = IntSize(14, 14);
    style->setWidth(Length(size.width(), Fixed));
    style->setHeight(Length(size.height(), Fixed));
}

static IntRect centerRectVerticallyInParentInputElement(RenderObject* object, const IntRect& rect)
{
    IntRect centeredRect(rect);
    Node* input = object->node()->shadowAncestorNode(); // Get the renderer of <input> element.
    if (!input->renderer()->isBox()) 
        return centeredRect;

    // If possible center the y-coordinate of the rect vertically in the parent input element.
    // We also add one pixel here to ensure that the y coordinate is rounded up for box heights
    // that are even, which looks in relation to the box text.
    IntRect inputContentBox = toRenderBox(input->renderer())->absoluteContentBox();
    centeredRect.setY(inputContentBox.y() + (inputContentBox.height() - centeredRect.height() + 1) / 2);
    return centeredRect;
}

bool RenderThemeClutter::paintSearchFieldResultsDecoration(RenderObject* object, const PaintInfo& i, const IntRect& rect)
{
    notImplemented();
    return false;
}

void RenderThemeClutter::adjustSearchFieldCancelButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    style->resetBorder();
    style->resetPadding();

    // FIXME: This should not be hard-coded.
    IntSize size = IntSize(14, 14);
    style->setWidth(Length(size.width(), Fixed));
    style->setHeight(Length(size.height(), Fixed));
}

bool RenderThemeClutter::paintSearchFieldCancelButton(RenderObject* object, const PaintInfo& i, const IntRect& rect)
{
    // TODO: Brightening up the image on hover is desirable here, I believe.
    notImplemented();
    return false;
}

void RenderThemeClutter::adjustSearchFieldStyle(CSSStyleSelector* selector, RenderStyle* style, Element* e) const
{
    adjustTextFieldStyle(selector, style, e);
}

bool RenderThemeClutter::paintSearchField(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    return paintTextField(o, i, rect);
}

bool RenderThemeClutter::paintSliderTrack(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    notImplemented();

    return false;
}

void RenderThemeClutter::adjustSliderTrackStyle(CSSStyleSelector*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(0);
}

bool RenderThemeClutter::paintSliderThumb(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    notImplemented();
    return false;
}

void RenderThemeClutter::adjustSliderThumbStyle(CSSStyleSelector*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(0);
}

void RenderThemeClutter::adjustSliderThumbSize(RenderObject* o) const
{
    notImplemented();
}

Color RenderThemeClutter::platformActiveSelectionBackgroundColor() const
{
    ClutterColor white = {0xff, 0xff, 0xff,0xff};
    return white;
}

Color RenderThemeClutter::platformInactiveSelectionBackgroundColor() const
{
    ClutterColor white = {0xff, 0xff, 0xff,0xff};
    return white;
}

Color RenderThemeClutter::platformActiveSelectionForegroundColor() const
{
    ClutterColor white = {0xff, 0xff, 0xff,0xff};
    return white;
}

Color RenderThemeClutter::platformInactiveSelectionForegroundColor() const
{
    ClutterColor white = {0xff, 0xff, 0xff,0xff};
    return white;
}

Color RenderThemeClutter::activeListBoxSelectionBackgroundColor() const
{
    ClutterColor white = {0xff, 0xff, 0xff,0xff};
    return white;
}

Color RenderThemeClutter::inactiveListBoxSelectionBackgroundColor() const
{
    ClutterColor white = {0xff, 0xff, 0xff,0xff};
    return white;
}

Color RenderThemeClutter::activeListBoxSelectionForegroundColor() const
{
    ClutterColor white = {0xff, 0xff, 0xff,0xff};
    return white;
}

Color RenderThemeClutter::inactiveListBoxSelectionForegroundColor() const
{
    ClutterColor white = {0xff, 0xff, 0xff,0xff};
    return white;
}

double RenderThemeClutter::caretBlinkInterval() const
{
    notImplemented();

    return 0.0;
}

void RenderThemeClutter::systemFont(int, FontDescription&) const
{
    // If you remove this notImplemented(), replace it with an comment that explains why.
    notImplemented();
}

Color RenderThemeClutter::systemColor(int cssValueId) const
{
    switch (cssValueId) {
    case CSSValueButtontext:
        return Color(gtk_widget_get_style(gtkButton())->fg[GTK_STATE_NORMAL]);
    case CSSValueCaptiontext:
        return Color(gtk_widget_get_style(gtkEntry())->fg[GTK_STATE_NORMAL]);
    default:
        return RenderTheme::systemColor(cssValueId);
    }
}


void RenderThemeClutter::platformColorsDidChange()
{
#if ENABLE(VIDEO)
    notImplemented();
#endif
    RenderTheme::platformColorsDidChange();
}

#if ENABLE(VIDEO)
String RenderThemeClutter::extraMediaControlsStyleSheet()
{
    return String(mediaControlsGtkUserAgentStyleSheet, sizeof(mediaControlsGtkUserAgentStyleSheet));
}

bool RenderThemeClutter::paintMediaFullscreenButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    notImplemented();
    return false;
}

bool RenderThemeClutter::paintMediaMuteButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    notImplemented();
    return false;}

bool RenderThemeClutter::paintMediaPlayButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    notImplemented();
    return false;}

bool RenderThemeClutter::paintMediaSeekBackButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    notImplemented();
    return false;
}

bool RenderThemeClutter::paintMediaSeekForwardButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    notImplemented();
    return false;
}

bool RenderThemeClutter::paintMediaSliderTrack(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    notImplemented();
    return false;
}

bool RenderThemeClutter::paintMediaSliderThumb(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    notImplemented();
    return false;
}
#endif

#if ENABLE(PROGRESS_TAG)
double RenderThemeClutter::animationRepeatIntervalForProgressBar(RenderProgress*) const
{
    // FIXME: It doesn't look like there is a good way yet to support animated
    // progress bars with the Mozilla theme drawing code.
    return 0;
}

double RenderThemeClutter::animationDurationForProgressBar(RenderProgress*) const
{
    // FIXME: It doesn't look like there is a good way yet to support animated
    // progress bars with the Mozilla theme drawing code.
    return 0;
}

void RenderThemeClutter::adjustProgressBarStyle(CSSStyleSelector*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(0);
}

bool RenderThemeClutter::paintProgressBar(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    notImplemented();
    return false;
}
#endif

}
