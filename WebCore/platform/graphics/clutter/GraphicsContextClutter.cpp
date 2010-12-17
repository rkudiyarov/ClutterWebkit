/*
 * Copyright (C) 2010 Viatcheslav Gachkaylo <vgachkaylo@crystalnix.com>
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
#include "GraphicsContext.h"
#include "GraphicsContextPrivate.h"

#include <cairo.h>

#if OS(WINDOWS)
#include <windows.h>
#include <cairo-win32.h>
#endif

namespace WebCore {

#if OS(WINDOWS)
void GraphicsContext::setDrawScrollOffset(const IntSize& offset)
{
    m_common->m_drawScrollOffset = offset;
}

HDC GraphicsContext::getWindowsContext(const IntRect& dstRect, bool supportAlphaBlend, bool mayCreateBitmap)
{
    // painting through native HDC is only supported for plugin, where mayCreateBitmap is always true
    ASSERT(mayCreateBitmap);

    if (dstRect.isEmpty())
        return 0;

    // Create a bitmap DC in which to draw.
    BITMAPINFO bitmapInfo;
    bitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth         = dstRect.width();
    bitmapInfo.bmiHeader.biHeight        = dstRect.height();
    bitmapInfo.bmiHeader.biPlanes        = 1;
    bitmapInfo.bmiHeader.biBitCount      = 32;
    bitmapInfo.bmiHeader.biCompression   = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage     = 0;
    bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biClrUsed       = 0;
    bitmapInfo.bmiHeader.biClrImportant  = 0;

    void* pixels = 0;
    HBITMAP bitmap = ::CreateDIBSection(0, &bitmapInfo, DIB_RGB_COLORS, &pixels, 0, 0);
    if (!bitmap)
        return 0;

    HDC displayDC = ::GetDC(0);
    HDC bitmapDC = ::CreateCompatibleDC(displayDC);
    ::ReleaseDC(0, displayDC);

    ::SelectObject(bitmapDC, bitmap);

    // Fill our buffer with clear if we're going to alpha blend.
    if (supportAlphaBlend) {
        BITMAP bmpInfo;
        GetObject(bitmap, sizeof(bmpInfo), &bmpInfo);
        int bufferSize = bmpInfo.bmWidthBytes * bmpInfo.bmHeight;
        memset(bmpInfo.bmBits, 0, bufferSize);
    }

    // Make sure we can do world transforms.
    SetGraphicsMode(bitmapDC, GM_ADVANCED);

    // Apply a translation to our context so that the drawing done will be at (0,0) of the bitmap.
    XFORM xform;
    xform.eM11 = 1.0f;
    xform.eM12 = 0.0f;
    xform.eM21 = 0.0f;
    xform.eM22 = 1.0f;
    xform.eDx = -dstRect.x();
    xform.eDy = -dstRect.y();
    ::SetWorldTransform(bitmapDC, &xform);

    return bitmapDC;
}

void GraphicsContext::releaseWindowsContext(HDC hdc, const IntRect& dstRect, bool supportAlphaBlend, bool mayCreateBitmap)
{
    // painting through native HDC is only supported for plugin, where mayCreateBitmap is always true
    ASSERT(mayCreateBitmap);

    if (hdc) {

        if (!dstRect.isEmpty()) {

            HBITMAP bitmap = static_cast<HBITMAP>(GetCurrentObject(hdc, OBJ_BITMAP));
            BITMAP info;
            GetObject(bitmap, sizeof(info), &info);
            ASSERT(info.bmBitsPixel == 32);
            
            unsigned char *invPixmap = new unsigned char[info.bmWidthBytes*info.bmHeight];
            unsigned char *src = (unsigned char*)info.bmBits + (info.bmHeight-1)*info.bmWidthBytes;
            unsigned char *dst = invPixmap;
            for (LONG scanLine = info.bmHeight; scanLine != 0; --scanLine) {
                memcpy(dst, src, info.bmWidthBytes);
                dst += info.bmWidthBytes;
                src -= info.bmWidthBytes;
            }
            
            cairo_surface_t *pixmap = cairo_image_surface_create_for_data(invPixmap,
                                                                          CAIRO_FORMAT_ARGB32,
                                                                          info.bmWidth,
                                                                          info.bmHeight,
                                                                          info.bmWidthBytes);
            
            PlatformGraphicsContext* ctx = this->platformContext();
            int dst_x = dstRect.x() + m_common->m_drawScrollOffset.width();
            int dst_y = dstRect.y() + m_common->m_drawScrollOffset.height();
            cairo_set_source_surface(ctx, pixmap, dst_x, dst_y);
            cairo_rectangle(ctx, dst_x, dst_y, dstRect.width(), dstRect.height());
            cairo_fill(ctx);
            cairo_surface_destroy(pixmap);
            delete[] invPixmap;

            ::DeleteObject(bitmap);
        }

        ::DeleteDC(hdc);
    }
}
#endif // OS(WINDOWS)

}