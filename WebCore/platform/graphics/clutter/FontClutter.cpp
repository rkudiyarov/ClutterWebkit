/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (c) 2007 Hiroyuki Ikezoe
 * Copyright (c) 2007 Kouhei Sutou
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Xan Lopez <xan@gnome.org>
 * Copyright (C) 2008 Nuanti Ltd.
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
#include "Font.h"

#include "GraphicsContext.h"
#include "SimpleFontData.h"

#include <cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#if defined(USE_FREETYPE)
#include <pango/pangofc-fontmap.h>
#endif

namespace WebCore {

#define IS_HIGH_SURROGATE(u)  ((UChar)(u) >= (UChar)0xd800 && (UChar)(u) <= (UChar)0xdbff)
#define IS_LOW_SURROGATE(u)  ((UChar)(u) >= (UChar)0xdc00 && (UChar)(u) <= (UChar)0xdfff)

static void utf16_to_utf8(const UChar* aText, gint aLength, char* &text, gint &length)
{
  gboolean need_copy = FALSE;
  int i;

  for (i = 0; i < aLength; i++) {
    if (!aText[i] || IS_LOW_SURROGATE(aText[i])) {
      need_copy = TRUE;
      break;
    }
    else if (IS_HIGH_SURROGATE(aText[i])) {
      if (i < aLength - 1 && IS_LOW_SURROGATE(aText[i+1]))
        i++;
      else {
        need_copy = TRUE;
        break;
      }
    }
  }

  if (need_copy) {

    /* Pango doesn't correctly handle nuls.  We convert them to 0xff. */
    /* Also "validate" UTF-16 text to make sure conversion doesn't fail. */

    UChar* p = (UChar*)g_memdup(aText, aLength * sizeof(aText[0]));

    /* don't need to reset i */
    for (i = 0; i < aLength; i++) {
      if (!p[i] || IS_LOW_SURROGATE(p[i]))
        p[i] = 0xFFFD;
      else if (IS_HIGH_SURROGATE(p[i])) {
        if (i < aLength - 1 && IS_LOW_SURROGATE(aText[i+1]))
          i++;
        else
          p[i] = 0xFFFD;
      }
    }

    aText = p;
  }

  glong items_written;
  text = g_utf16_to_utf8(reinterpret_cast<const gunichar2*>(aText), aLength, NULL, &items_written, NULL);
  length = items_written;

  if (need_copy)
    g_free((gpointer)aText);

}

static gchar* convertUniCharToUTF8(const UChar* characters, gint length, int from, int to)
{
    gchar* utf8 = 0;
    gint new_length = 0;
    utf16_to_utf8(characters, length, utf8, new_length);
    if (!utf8)
        return NULL;

    if (from > 0) {
        // discard the first 'from' characters
        // FIXME: we should do this before the conversion probably
        gchar* str_left = g_utf8_offset_to_pointer(utf8, from);
        gchar* tmp = g_strdup(str_left);
        g_free(utf8);
        utf8 = tmp;
    }

    gchar* pos = utf8;
    gint len = strlen(pos);
    GString* ret = g_string_new_len(NULL, len);

    // replace line break by space
    while (len > 0) {
        gint index, start;
        pango_find_paragraph_boundary(pos, len, &index, &start);
        g_string_append_len(ret, pos, index);
        if (index == start)
            break;
        g_string_append_c(ret, ' ');
        pos += start;
        len -= start;
    }
    return g_string_free(ret, FALSE);
}

static void setPangoAttributes(const Font* font, const TextRun& run, PangoLayout* layout)
{
#if defined(USE_FREETYPE)
    if (font->primaryFont()->platformData().m_pattern) {
        PangoFontDescription* desc = pango_fc_font_description_from_pattern(font->primaryFont()->platformData().m_pattern.get(), FALSE);
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);
    }
#elif defined(USE_PANGO)
    if (font->primaryFont()->platformData().m_font) {
        PangoFontDescription* desc = pango_font_describe(font->primaryFont()->platformData().m_font);
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);
    }
#endif

    pango_layout_set_auto_dir(layout, FALSE);

    PangoContext* pangoContext = pango_layout_get_context(layout);
    PangoDirection direction = run.rtl() ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
    pango_context_set_base_dir(pangoContext, direction);
    PangoAttrList* list = pango_attr_list_new();
    PangoAttribute* attr;

    attr = pango_attr_size_new_absolute(font->pixelSize() * PANGO_SCALE);
    attr->end_index = G_MAXUINT;
    pango_attr_list_insert_before(list, attr);

    if (!run.spacingDisabled()) {
        attr = pango_attr_letter_spacing_new(font->letterSpacing() * PANGO_SCALE);
        attr->end_index = G_MAXUINT;
        pango_attr_list_insert_before(list, attr);
    }

    // Pango does not yet support synthesising small caps
    // See http://bugs.webkit.org/show_bug.cgi?id=15610

    pango_layout_set_attributes(layout, list);
    pango_attr_list_unref(list);
}

/**
 * gdk_pango_layout_line_get_clip_region:
 * @line: a #PangoLayoutLine 
 * @x_origin: X pixel where you intend to draw the layout line with this clip
 * @y_origin: baseline pixel where you intend to draw the layout line with this clip
 * @index_ranges: array of byte indexes into the layout, where even members of array are start indexes and odd elements are end indexes
 * @n_ranges: number of ranges in @index_ranges, i.e. half the size of @index_ranges
 * 
 * Obtains a clip region which contains the areas where the given
 * ranges of text would be drawn. @x_origin and @y_origin are the same
 * position you would pass to gdk_draw_layout_line(). @index_ranges
 * should contain ranges of bytes in the layout's text. The clip
 * region will include space to the left or right of the line (to the
 * layout bounding box) if you have indexes above or below the indexes
 * contained inside the line. This is to draw the selection all the way
 * to the side of the layout. However, the clip region is in line coordinates,
 * not layout coordinates.
 * 
 * Return value: a clip region containing the given ranges
 **/
static cairo_region_t*
clutter_pango_layout_line_get_clip_region (PangoLayoutLine *line,
                                       gint             x_origin,
                                       gint             y_origin,
                                       gint            *index_ranges,
                                       gint             n_ranges)
{
  cairo_region_t *clip_region;
  gint i;
  PangoRectangle logical_rect;
  PangoLayoutIter *iter;
  gint baseline;
  
  g_return_val_if_fail (line != NULL, NULL);
  g_return_val_if_fail (index_ranges != NULL, NULL);
  
  clip_region = cairo_region_create ();

  iter = pango_layout_get_iter (line->layout);
  while (pango_layout_iter_get_line (iter) != line)
    pango_layout_iter_next_line (iter);
  
  pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
  baseline = pango_layout_iter_get_baseline (iter);
  
  pango_layout_iter_free (iter);
  
  i = 0;
  while (i < n_ranges)
    {  
      gint *pixel_ranges = NULL;
      gint n_pixel_ranges = 0;
      gint j;

      /* Note that get_x_ranges returns layout coordinates
       */
      pango_layout_line_get_x_ranges (line,
                                      index_ranges[i*2],
                                      index_ranges[i*2+1],
                                      &pixel_ranges, &n_pixel_ranges);
  
      for (j=0; j < n_pixel_ranges; j++)
        {
          cairo_rectangle_int_t rect;
          
          rect.x = x_origin + pixel_ranges[2*j] / PANGO_SCALE - logical_rect.x / PANGO_SCALE;
          rect.y = y_origin - (baseline / PANGO_SCALE - logical_rect.y / PANGO_SCALE);
          rect.width = (pixel_ranges[2*j + 1] - pixel_ranges[2*j]) / PANGO_SCALE;
          rect.height = logical_rect.height / PANGO_SCALE;
          
          cairo_region_union_rectangle (clip_region, &rect);
        }

      g_free (pixel_ranges);
      ++i;
    }

  return clip_region;
}

/**
 * gdk_cairo_region:
 * @cr: a #cairo_t
 * @region: a #GdkRegion
 * 
 * Adds the given region to the current path of @cr.
 *
 * Since: 2.8
 **/
void
clutter_cairo_region (cairo_t   *cr,
		  cairo_region_t *region)
{
  cairo_rectangle_int_t box;
  gint n_boxes, i;

  g_return_if_fail (cr != NULL);
  g_return_if_fail (region != NULL);

  n_boxes = cairo_region_num_rectangles(region);

  for (i = 0; i < n_boxes; i++) {
    cairo_region_get_rectangle(region, i, &box)
    cairo_rectangle (cr,
		     box.x,
		     box.y,
		     box.width,
		     box.height);
  }
}

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

static void cairo_region_shrink(cairo_region_t* region, int dx, int dy)
{
    int nRects = cairo_region_num_rectangles(region);
    // Clear region.
    cairo_region_subtract(region, region);

    for (int i = 0; i < nRects; i++) {
        cairo_rectangle_int_t rect;
        cairo_region_get_rectangle(region, i, &rect);

        if (rect.width <= 2 * dx || rect.height <= 2 * dy)
            continue;

        rect.x += dx;
        rect.y += dy;
        rect.width -= 2 * dx;
        rect.height -= 2 * dy;
        cairo_region_union_rectangle(region, &rect);
    }
}

void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
#if defined(USE_FREETYPE)
    if (!primaryFont()->platformData().m_pattern) {
        drawSimpleText(context, run, point, from, to);
        return;
    }
#endif

    cairo_t* cr = context->platformContext();
    cairo_save(cr);
    cairo_translate(cr, point.x(), point.y());

    PangoLayout* layout = pango_cairo_create_layout(cr);
    setPangoAttributes(this, run, layout);

    gchar* utf8 = convertUniCharToUTF8(run.characters(), run.length(), 0, run.length());
    pango_layout_set_text(layout, utf8, -1);

    // Our layouts are single line
    PangoLayoutLine* layoutLine = pango_layout_get_line_readonly(layout, 0);

    cairo_region_t* partialRegion = 0;

    if (to - from != run.length()) {
        // Clip the region of the run to be rendered
        char* start = g_utf8_offset_to_pointer(utf8, from);
        char* end = g_utf8_offset_to_pointer(start, to - from);
        int ranges[] = {start - utf8, end - utf8};
        partialRegion = clutter_pango_layout_line_get_clip_region(layoutLine, 0, 0, ranges, 1);
        cairo_region_shrink(partialRegion, 0, -pixelSize());
    }

    Color fillColor = context->fillColor();
    float red, green, blue, alpha;

    // Text shadow, inspired by FontMac
    FloatSize shadowOffset;
    float shadowBlur = 0;
    Color shadowColor;
    bool hasShadow = context->textDrawingMode() == cTextFill &&
        context->getShadow(shadowOffset, shadowBlur, shadowColor);

    // TODO: Blur support
    if (hasShadow) {
        // Disable graphics context shadows (not yet implemented) and paint them manually
        context->clearShadow();
        Color shadowFillColor(shadowColor.red(), shadowColor.green(), shadowColor.blue(), shadowColor.alpha() * fillColor.alpha() / 255);
        cairo_save(cr);

        shadowFillColor.getRGBA(red, green, blue, alpha);
        cairo_set_source_rgba(cr, red, green, blue, alpha);

        cairo_translate(cr, shadowOffset.width(), shadowOffset.height());

        if (partialRegion) {
            clutter_cairo_region(cr, partialRegion);
            cairo_clip(cr);
        }

        pango_cairo_show_layout_line(cr, layoutLine);

        cairo_restore(cr);
    }

    fillColor.getRGBA(red, green, blue, alpha);
    cairo_set_source_rgba(cr, red, green, blue, alpha);

    if (partialRegion) {
        clutter_cairo_region(cr, partialRegion);
        cairo_clip(cr);
    }

    pango_cairo_show_layout_line(cr, layoutLine);

    if (context->textDrawingMode() & cTextStroke) {
        Color strokeColor = context->strokeColor();
        strokeColor.getRGBA(red, green, blue, alpha);
        cairo_set_source_rgba(cr, red, green, blue, alpha);
        pango_cairo_layout_line_path(cr, layoutLine);
        cairo_set_line_width(cr, context->strokeThickness());
        cairo_stroke(cr);
    }

    // Re-enable the platform shadow we disabled earlier
    if (hasShadow)
        context->setShadow(shadowOffset, shadowBlur, shadowColor, DeviceColorSpace);

    // Pango sometimes leaves behind paths we don't want
    cairo_new_path(cr);

    if (partialRegion)
        cairo_region_destroy(partialRegion);

    g_free(utf8);
    g_object_unref(layout);

    cairo_restore(cr);
}

// We should create the layout with our actual context but we can't access it from here.
static PangoLayout* getDefaultPangoLayout(const TextRun& run)
{
    static PangoFontMap* map = pango_cairo_font_map_get_default();
#if PANGO_VERSION_CHECK(1,21,5)
    static PangoContext* pangoContext = pango_font_map_create_context(map);
#else
    // Deprecated in Pango 1.21.
    static PangoContext* pangoContext = pango_cairo_font_map_create_context(PANGO_CAIRO_FONT_MAP(map));
#endif
    PangoLayout* layout = pango_layout_new(pangoContext);

    return layout;
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* overflow) const
{
#if defined(USE_FREETYPE)
    if (!primaryFont()->platformData().m_pattern)
        return floatWidthForSimpleText(run, 0, fallbackFonts, overflow);
#endif

    if (run.length() == 0)
        return 0.0f;

    PangoLayout* layout = getDefaultPangoLayout(run);
    setPangoAttributes(this, run, layout);

    gchar* utf8 = convertUniCharToUTF8(run.characters(), run.length(), 0, run.length());
    pango_layout_set_text(layout, utf8, -1);

    int width;
    pango_layout_get_pixel_size(layout, &width, 0);

    g_free(utf8);
    g_object_unref(layout);

    return width;
}

int Font::offsetForPositionForComplexText(const TextRun& run, float xFloat, bool includePartialGlyphs) const
{
#if defined(USE_FREETYPE)
    if (!primaryFont()->platformData().m_pattern)
        return offsetForPositionForSimpleText(run, xFloat, includePartialGlyphs);
#endif
    // FIXME: This truncation is not a problem for HTML, but only affects SVG, which passes floating-point numbers
    // to Font::offsetForPosition(). Bug http://webkit.org/b/40673 tracks fixing this problem.
    int x = static_cast<int>(xFloat);

    PangoLayout* layout = getDefaultPangoLayout(run);
    setPangoAttributes(this, run, layout);

    gchar* utf8 = convertUniCharToUTF8(run.characters(), run.length(), 0, run.length());
    pango_layout_set_text(layout, utf8, -1);

    int index, trailing;
    pango_layout_xy_to_index(layout, x * PANGO_SCALE, 1, &index, &trailing);
    glong offset = g_utf8_pointer_to_offset(utf8, utf8 + index);
    if (includePartialGlyphs)
        offset += trailing;

    g_free(utf8);
    g_object_unref(layout);

    return offset;
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& point, int h, int from, int to) const
{
#if defined(USE_FREETYPE)
    if (!primaryFont()->platformData().m_pattern)
        return selectionRectForSimpleText(run, point, h, from, to);
#endif

    PangoLayout* layout = getDefaultPangoLayout(run);
    setPangoAttributes(this, run, layout);

    gchar* utf8 = convertUniCharToUTF8(run.characters(), run.length(), 0, run.length());
    pango_layout_set_text(layout, utf8, -1);

    char* start = g_utf8_offset_to_pointer(utf8, from);
    char* end = g_utf8_offset_to_pointer(start, to - from);

    if (run.ltr()) {
        from = start - utf8;
        to = end - utf8;
    } else {
        from = end - utf8;
        to = start - utf8;
    }

    PangoLayoutLine* layoutLine = pango_layout_get_line_readonly(layout, 0);
    int x_pos;

    x_pos = 0;
    if (from < layoutLine->length)
        pango_layout_line_index_to_x(layoutLine, from, FALSE, &x_pos);
    float beforeWidth = PANGO_PIXELS_FLOOR(x_pos);

    x_pos = 0;
    if (run.ltr() || to < layoutLine->length)
        pango_layout_line_index_to_x(layoutLine, to, FALSE, &x_pos);
    float afterWidth = PANGO_PIXELS(x_pos);

    g_free(utf8);
    g_object_unref(layout);

    return FloatRect(point.x() + beforeWidth, point.y(), afterWidth - beforeWidth, h);
}

}
