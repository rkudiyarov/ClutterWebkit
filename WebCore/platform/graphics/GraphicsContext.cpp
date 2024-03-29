/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GraphicsContext.h"

#include "BidiResolver.h"
#include "Font.h"
#include "Generator.h"
#include "GraphicsContextPrivate.h"
#include "ImageBuffer.h"

using namespace std;

namespace WebCore {

class TextRunIterator {
public:
    TextRunIterator()
        : m_textRun(0)
        , m_offset(0)
    {
    }

    TextRunIterator(const TextRun* textRun, unsigned offset)
        : m_textRun(textRun)
        , m_offset(offset)
    {
    }

    TextRunIterator(const TextRunIterator& other)
        : m_textRun(other.m_textRun)
        , m_offset(other.m_offset)
    {
    }

    unsigned offset() const { return m_offset; }
    void increment() { m_offset++; }
    bool atEnd() const { return !m_textRun || m_offset >= m_textRun->length(); }
    UChar current() const { return (*m_textRun)[m_offset]; }
    WTF::Unicode::Direction direction() const { return atEnd() ? WTF::Unicode::OtherNeutral : WTF::Unicode::direction(current()); }

    bool operator==(const TextRunIterator& other)
    {
        return m_offset == other.m_offset && m_textRun == other.m_textRun;
    }

    bool operator!=(const TextRunIterator& other) { return !operator==(other); }

private:
    const TextRun* m_textRun;
    int m_offset;
};

GraphicsContextPrivate* GraphicsContext::createGraphicsContextPrivate()
{
    return new GraphicsContextPrivate;
}

void GraphicsContext::destroyGraphicsContextPrivate(GraphicsContextPrivate* deleteMe)
{
    delete deleteMe;
}

void GraphicsContext::save()
{
    if (paintingDisabled())
        return;

    m_common->stack.append(m_common->state);

    savePlatformState();
}

void GraphicsContext::restore()
{
    if (paintingDisabled())
        return;

    if (m_common->stack.isEmpty()) {
        LOG_ERROR("ERROR void GraphicsContext::restore() stack is empty");
        return;
    }
    m_common->state = m_common->stack.last();
    m_common->stack.removeLast();

    restorePlatformState();
}

void GraphicsContext::setStrokeThickness(float thickness)
{
    m_common->state.strokeThickness = thickness;
    setPlatformStrokeThickness(thickness);
}

void GraphicsContext::setStrokeStyle(const StrokeStyle& style)
{
    m_common->state.strokeStyle = style;
    setPlatformStrokeStyle(style);
}

void GraphicsContext::setStrokeColor(const Color& color, ColorSpace colorSpace)
{
    m_common->state.strokeColor = color;
    m_common->state.strokeColorSpace = colorSpace;
    m_common->state.strokeGradient.clear();
    m_common->state.strokePattern.clear();
    setPlatformStrokeColor(color, colorSpace);
}

void GraphicsContext::setShadow(const FloatSize& offset, float blur, const Color& color, ColorSpace colorSpace)
{
    m_common->state.shadowOffset = offset;
    m_common->state.shadowBlur = blur;
    m_common->state.shadowColor = color;
    setPlatformShadow(offset, blur, color, colorSpace);
}

void GraphicsContext::clearShadow()
{
    m_common->state.shadowOffset = FloatSize();
    m_common->state.shadowBlur = 0;
    m_common->state.shadowColor = Color();
    clearPlatformShadow();
}

bool GraphicsContext::getShadow(FloatSize& offset, float& blur, Color& color) const
{
    offset = m_common->state.shadowOffset;
    blur = m_common->state.shadowBlur;
    color = m_common->state.shadowColor;

    return color.isValid() && color.alpha() && (blur || offset.width() || offset.height());
}

float GraphicsContext::strokeThickness() const
{
    return m_common->state.strokeThickness;
}

StrokeStyle GraphicsContext::strokeStyle() const
{
    return m_common->state.strokeStyle;
}

Color GraphicsContext::strokeColor() const
{
    return m_common->state.strokeColor;
}

ColorSpace GraphicsContext::strokeColorSpace() const
{
    return m_common->state.strokeColorSpace;
}

WindRule GraphicsContext::fillRule() const
{
    return m_common->state.fillRule;
}

void GraphicsContext::setFillRule(WindRule fillRule)
{
    m_common->state.fillRule = fillRule;
}

void GraphicsContext::setFillColor(const Color& color, ColorSpace colorSpace)
{
    m_common->state.fillColor = color;
    m_common->state.fillColorSpace = colorSpace;
    m_common->state.fillGradient.clear();
    m_common->state.fillPattern.clear();
    setPlatformFillColor(color, colorSpace);
}

Color GraphicsContext::fillColor() const
{
    return m_common->state.fillColor;
}

ColorSpace GraphicsContext::fillColorSpace() const
{
    return m_common->state.fillColorSpace;
}

void GraphicsContext::setShouldAntialias(bool b)
{
    m_common->state.shouldAntialias = b;
    setPlatformShouldAntialias(b);
}

bool GraphicsContext::shouldAntialias() const
{
    return m_common->state.shouldAntialias;
}

void GraphicsContext::setStrokePattern(PassRefPtr<Pattern> pattern)
{
    ASSERT(pattern);
    if (!pattern) {
        setStrokeColor(Color::black, DeviceColorSpace);
        return;
    }
    m_common->state.strokeGradient.clear();
    m_common->state.strokePattern = pattern;
    setPlatformStrokePattern(m_common->state.strokePattern.get());
}

void GraphicsContext::setFillPattern(PassRefPtr<Pattern> pattern)
{
    ASSERT(pattern);
    if (!pattern) {
        setFillColor(Color::black, DeviceColorSpace);
        return;
    }
    m_common->state.fillGradient.clear();
    m_common->state.fillPattern = pattern;
    setPlatformFillPattern(m_common->state.fillPattern.get());
}

void GraphicsContext::setStrokeGradient(PassRefPtr<Gradient> gradient)
{
    ASSERT(gradient);
    if (!gradient) {
        setStrokeColor(Color::black, DeviceColorSpace);
        return;
    }
    m_common->state.strokeGradient = gradient;
    m_common->state.strokePattern.clear();
    setPlatformStrokeGradient(m_common->state.strokeGradient.get());
}

void GraphicsContext::setFillGradient(PassRefPtr<Gradient> gradient)
{
    ASSERT(gradient);
    if (!gradient) {
        setFillColor(Color::black, DeviceColorSpace);
        return;
    }
    m_common->state.fillGradient = gradient;
    m_common->state.fillPattern.clear();
    setPlatformFillGradient(m_common->state.fillGradient.get());
}

Gradient* GraphicsContext::fillGradient() const
{
    return m_common->state.fillGradient.get();
}

Gradient* GraphicsContext::strokeGradient() const
{
    return m_common->state.strokeGradient.get();
}

Pattern* GraphicsContext::fillPattern() const
{
    return m_common->state.fillPattern.get();
}

Pattern* GraphicsContext::strokePattern() const
{
    return m_common->state.strokePattern.get();
}

void GraphicsContext::setShadowsIgnoreTransforms(bool ignoreTransforms)
{
    m_common->state.shadowsIgnoreTransforms = ignoreTransforms;
}

bool GraphicsContext::updatingControlTints() const
{
    return m_common->m_updatingControlTints;
}

void GraphicsContext::setUpdatingControlTints(bool b)
{
    setPaintingDisabled(b);
    m_common->m_updatingControlTints = b;
}

void GraphicsContext::setPaintingDisabled(bool f)
{
    m_common->state.paintingDisabled = f;
}

bool GraphicsContext::paintingDisabled() const
{
    return m_common->state.paintingDisabled;
}

void GraphicsContext::drawImage(Image* image, ColorSpace styleColorSpace, const IntPoint& p, CompositeOperator op)
{
    drawImage(image, styleColorSpace, p, IntRect(0, 0, -1, -1), op);
}

void GraphicsContext::drawImage(Image* image, ColorSpace styleColorSpace, const IntRect& r, CompositeOperator op, bool useLowQualityScale)
{
    drawImage(image, styleColorSpace, r, IntRect(0, 0, -1, -1), op, useLowQualityScale);
}

void GraphicsContext::drawImage(Image* image, ColorSpace styleColorSpace, const IntPoint& dest, const IntRect& srcRect, CompositeOperator op)
{
    drawImage(image, styleColorSpace, IntRect(dest, srcRect.size()), srcRect, op);
}

void GraphicsContext::drawImage(Image* image, ColorSpace styleColorSpace, const IntRect& dest, const IntRect& srcRect, CompositeOperator op, bool useLowQualityScale)
{
    drawImage(image, styleColorSpace, FloatRect(dest), srcRect, op, useLowQualityScale);
}

#if !OS(WINCE) || PLATFORM(QT)
void GraphicsContext::drawText(const Font& font, const TextRun& run, const IntPoint& point, int from, int to)
{
    if (paintingDisabled())
        return;

    font.drawText(this, run, point, from, to);
}
#endif

void GraphicsContext::drawBidiText(const Font& font, const TextRun& run, const FloatPoint& point)
{
    if (paintingDisabled())
        return;

    BidiResolver<TextRunIterator, BidiCharacterRun> bidiResolver;
    WTF::Unicode::Direction paragraphDirection = run.ltr() ? WTF::Unicode::LeftToRight : WTF::Unicode::RightToLeft;

    bidiResolver.setStatus(BidiStatus(paragraphDirection, paragraphDirection, paragraphDirection, BidiContext::create(run.ltr() ? 0 : 1, paragraphDirection, run.directionalOverride())));

    bidiResolver.setPosition(TextRunIterator(&run, 0));
    bidiResolver.createBidiRunsForLine(TextRunIterator(&run, run.length()));

    if (!bidiResolver.runCount())
        return;

    FloatPoint currPoint = point;
    BidiCharacterRun* bidiRun = bidiResolver.firstRun();
    while (bidiRun) {

        TextRun subrun = run;
        subrun.setText(run.data(bidiRun->start()), bidiRun->stop() - bidiRun->start());
        subrun.setRTL(bidiRun->level() % 2);
        subrun.setDirectionalOverride(bidiRun->dirOverride(false));

        font.drawText(this, subrun, currPoint);

        bidiRun = bidiRun->next();
        // FIXME: Have Font::drawText return the width of what it drew so that we don't have to re-measure here.
        if (bidiRun)
            currPoint.move(font.floatWidth(subrun), 0.f);
    }

    bidiResolver.deleteRuns();
}

void GraphicsContext::drawHighlightForText(const Font& font, const TextRun& run, const IntPoint& point, int h, const Color& backgroundColor, ColorSpace colorSpace, int from, int to)
{
    if (paintingDisabled())
        return;

    fillRect(font.selectionRectForText(run, point, h, from, to), backgroundColor, colorSpace);
}

void GraphicsContext::drawImage(Image* image, ColorSpace styleColorSpace, const FloatRect& dest, const FloatRect& src, CompositeOperator op, bool useLowQualityScale)
{
    if (paintingDisabled() || !image)
        return;

    float tsw = src.width();
    float tsh = src.height();
    float tw = dest.width();
    float th = dest.height();

    if (tsw == -1)
        tsw = image->width();
    if (tsh == -1)
        tsh = image->height();

    if (tw == -1)
        tw = image->width();
    if (th == -1)
        th = image->height();

    if (useLowQualityScale) {
        save();
        setImageInterpolationQuality(InterpolationNone);
    }
    image->draw(this, FloatRect(dest.location(), FloatSize(tw, th)), FloatRect(src.location(), FloatSize(tsw, tsh)), styleColorSpace, op);
    if (useLowQualityScale)
        restore();
}

void GraphicsContext::drawTiledImage(Image* image, ColorSpace styleColorSpace, const IntRect& rect, const IntPoint& srcPoint, const IntSize& tileSize, CompositeOperator op, bool useLowQualityScale)
{
    if (paintingDisabled() || !image)
        return;
    if (useLowQualityScale) {
        save();
        setImageInterpolationQuality(InterpolationLow);
    }
    image->drawTiled(this, rect, srcPoint, tileSize, styleColorSpace, op);
    if (useLowQualityScale)
        restore();
}

void GraphicsContext::drawTiledImage(Image* image, ColorSpace styleColorSpace, const IntRect& dest, const IntRect& srcRect, Image::TileRule hRule, Image::TileRule vRule, CompositeOperator op, bool useLowQualityScale)
{
    if (paintingDisabled() || !image)
        return;

    if (useLowQualityScale) {
        save();
        setImageInterpolationQuality(InterpolationLow);
    }
    if (hRule == Image::StretchTile && vRule == Image::StretchTile)
        // Just do a scale.
        drawImage(image, styleColorSpace, dest, srcRect, op);
    else
        image->drawTiled(this, dest, srcRect, hRule, vRule, styleColorSpace, op);
    if (useLowQualityScale)
        restore();
}

void GraphicsContext::drawImageBuffer(ImageBuffer* image, ColorSpace styleColorSpace, const IntPoint& p, CompositeOperator op)
{
    drawImageBuffer(image, styleColorSpace, p, IntRect(0, 0, -1, -1), op);
}

void GraphicsContext::drawImageBuffer(ImageBuffer* image, ColorSpace styleColorSpace, const IntRect& r, CompositeOperator op, bool useLowQualityScale)
{
    drawImageBuffer(image, styleColorSpace, r, IntRect(0, 0, -1, -1), op, useLowQualityScale);
}

void GraphicsContext::drawImageBuffer(ImageBuffer* image, ColorSpace styleColorSpace, const IntPoint& dest, const IntRect& srcRect, CompositeOperator op)
{
    drawImageBuffer(image, styleColorSpace, IntRect(dest, srcRect.size()), srcRect, op);
}

void GraphicsContext::drawImageBuffer(ImageBuffer* image, ColorSpace styleColorSpace, const IntRect& dest, const IntRect& srcRect, CompositeOperator op, bool useLowQualityScale)
{
    drawImageBuffer(image, styleColorSpace, FloatRect(dest), srcRect, op, useLowQualityScale);
}

void GraphicsContext::drawImageBuffer(ImageBuffer* image, ColorSpace styleColorSpace, const FloatRect& dest, const FloatRect& src, CompositeOperator op, bool useLowQualityScale)
{
    if (paintingDisabled() || !image)
        return;

    float tsw = src.width();
    float tsh = src.height();
    float tw = dest.width();
    float th = dest.height();

    if (tsw == -1)
        tsw = image->width();
    if (tsh == -1)
        tsh = image->height();

    if (tw == -1)
        tw = image->width();
    if (th == -1)
        th = image->height();

    if (useLowQualityScale) {
        save();
        setImageInterpolationQuality(InterpolationNone);
    }

    image->draw(this, styleColorSpace, dest, src, op, useLowQualityScale);

    if (useLowQualityScale)
        restore();
}

void GraphicsContext::addRoundedRectClip(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight,
    const IntSize& bottomLeft, const IntSize& bottomRight)
{
    if (paintingDisabled())
        return;

    Path path;
    path.addRoundedRect(rect, topLeft, topRight, bottomLeft, bottomRight);
    clip(path);
}

void GraphicsContext::clipOutRoundedRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight,
                                         const IntSize& bottomLeft, const IntSize& bottomRight)
{
    if (paintingDisabled())
        return;

    Path path;
    path.addRoundedRect(rect, topLeft, topRight, bottomLeft, bottomRight);
    clipOut(path);
}

void GraphicsContext::clipToImageBuffer(ImageBuffer* buffer, const FloatRect& rect)
{
    if (paintingDisabled())
        return;
    buffer->clip(this, rect);
}

int GraphicsContext::textDrawingMode()
{
    return m_common->state.textDrawingMode;
}

void GraphicsContext::setTextDrawingMode(int mode)
{
    m_common->state.textDrawingMode = mode;
    if (paintingDisabled())
        return;
    setPlatformTextDrawingMode(mode);
}

void GraphicsContext::fillRect(const FloatRect& rect, Generator& generator)
{
    if (paintingDisabled())
        return;
    generator.fill(this, rect);
}

#if !PLATFORM(SKIA)
void GraphicsContext::setPlatformFillGradient(Gradient*)
{
}

void GraphicsContext::setPlatformFillPattern(Pattern*)
{
}

void GraphicsContext::setPlatformStrokeGradient(Gradient*)
{
}

void GraphicsContext::setPlatformStrokePattern(Pattern*)
{
}
#endif

#if !PLATFORM(CG) && !PLATFORM(SKIA)
// Implement this if you want to go ahead and push the drawing mode into your native context
// immediately.
void GraphicsContext::setPlatformTextDrawingMode(int mode)
{
}
#endif

#if !PLATFORM(QT) && !PLATFORM(CAIRO) && !PLATFORM(SKIA) && !PLATFORM(HAIKU) && !PLATFORM(OPENVG)
void GraphicsContext::setPlatformStrokeStyle(const StrokeStyle&)
{
}
#endif

#if !PLATFORM(SKIA)
void GraphicsContext::setSharedGraphicsContext3D(SharedGraphicsContext3D*, DrawingBuffer*, const IntSize&)
{
}

void GraphicsContext::syncSoftwareCanvas()
{
}

void GraphicsContext::markDirtyRect(const IntRect&)
{
}
#endif


void GraphicsContext::adjustLineToPixelBoundaries(FloatPoint& p1, FloatPoint& p2, float strokeWidth, const StrokeStyle& penStyle)
{
    // For odd widths, we add in 0.5 to the appropriate x/y so that the float arithmetic
    // works out.  For example, with a border width of 3, WebKit will pass us (y1+y2)/2, e.g.,
    // (50+53)/2 = 103/2 = 51 when we want 51.5.  It is always true that an even width gave
    // us a perfect position, but an odd width gave us a position that is off by exactly 0.5.
    if (penStyle == DottedStroke || penStyle == DashedStroke) {
        if (p1.x() == p2.x()) {
            p1.setY(p1.y() + strokeWidth);
            p2.setY(p2.y() - strokeWidth);
        } else {
            p1.setX(p1.x() + strokeWidth);
            p2.setX(p2.x() - strokeWidth);
        }
    }

    if (static_cast<int>(strokeWidth) % 2) { //odd
        if (p1.x() == p2.x()) {
            // We're a vertical line.  Adjust our x.
            p1.setX(p1.x() + 0.5f);
            p2.setX(p2.x() + 0.5f);
        } else {
            // We're a horizontal line. Adjust our y.
            p1.setY(p1.y() + 0.5f);
            p2.setY(p2.y() + 0.5f);
        }
    }
}

}
