/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "ContentLayerChromium.h"

#include "GraphicsContext3D.h"
#include "LayerRendererChromium.h"
#include "RenderLayerBacking.h"

#if PLATFORM(SKIA)
#include "NativeImageSkia.h"
#include "PlatformContextSkia.h"
#include "skia/ext/platform_canvas.h"
#elif PLATFORM(CG)
#include <CoreGraphics/CGBitmapContext.h>
#endif

namespace WebCore {

ContentLayerChromium::SharedValues::SharedValues(GraphicsContext3D* context)
    : m_context(context)
    , m_contentShaderProgram(0)
    , m_shaderSamplerLocation(-1)
    , m_shaderMatrixLocation(-1)
    , m_shaderAlphaLocation(-1)
    , m_initialized(false)
    , m_npotSupported(false)
{
    // Shaders for drawing the layer contents.
    char vertexShaderString[] =
        "attribute vec4 a_position;   \n"
        "attribute vec2 a_texCoord;   \n"
        "uniform mat4 matrix;         \n"
        "varying vec2 v_texCoord;     \n"
        "void main()                  \n"
        "{                            \n"
        "  gl_Position = matrix * a_position; \n"
        "  v_texCoord = a_texCoord;   \n"
        "}                            \n";

    // Note differences between Skia and Core Graphics versions:
    //  - Skia uses BGRA
    //  - Core Graphics uses RGBA
    char fragmentShaderString[] =
        "precision mediump float;                            \n"
        "varying vec2 v_texCoord;                            \n"
        "uniform sampler2D s_texture;                        \n"
        "uniform float alpha;                                \n"
        "void main()                                         \n"
        "{                                                   \n"
        "  vec4 texColor = texture2D(s_texture, v_texCoord); \n"
#if PLATFORM(SKIA)
        "  gl_FragColor = vec4(texColor.z, texColor.y, texColor.x, texColor.w) * alpha; \n"
#elif PLATFORM(CG)
        "  gl_FragColor = vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha; \n"
#else
#error "Need to implement for your platform."
#endif
        "}                                                   \n";

    m_contentShaderProgram = createShaderProgram(m_context, vertexShaderString, fragmentShaderString);
    if (!m_contentShaderProgram) {
        LOG_ERROR("ContentLayerChromium: Failed to create shader program");
        return;
    }

    m_shaderSamplerLocation = m_context->getUniformLocation(m_contentShaderProgram, "s_texture");
    m_shaderMatrixLocation = m_context->getUniformLocation(m_contentShaderProgram, "matrix");
    m_shaderAlphaLocation = m_context->getUniformLocation(m_contentShaderProgram, "alpha");
    ASSERT(m_shaderSamplerLocation != -1);
    ASSERT(m_shaderMatrixLocation != -1);
    ASSERT(m_shaderAlphaLocation != -1);

    m_npotSupported = GLC(context, context->getString(GraphicsContext3D::EXTENSIONS).contains("GL_OES_texture_npot"));

    m_initialized = true;
}

ContentLayerChromium::SharedValues::~SharedValues()
{
    if (m_contentShaderProgram)
        GLC(m_context, m_context->deleteProgram(m_contentShaderProgram));
}


PassRefPtr<ContentLayerChromium> ContentLayerChromium::create(GraphicsLayerChromium* owner)
{
    return adoptRef(new ContentLayerChromium(owner));
}

ContentLayerChromium::ContentLayerChromium(GraphicsLayerChromium* owner)
    : LayerChromium(owner)
    , m_contentsTexture(0)
{
}

ContentLayerChromium::~ContentLayerChromium()
{
    cleanupResources();
}

void ContentLayerChromium::cleanupResources()
{
    if (layerRenderer()) {
        if (m_contentsTexture) {
            layerRenderer()->deleteLayerTexture(m_contentsTexture);
            m_contentsTexture = 0;
        }
    }
}

void ContentLayerChromium::updateContents()
{
    RenderLayerBacking* backing = static_cast<RenderLayerBacking*>(m_owner->client());
    if (!backing || backing->paintingGoesToWindow())
        return;

    ASSERT(drawsContent());

    ASSERT(layerRenderer());

    // FIXME: Remove this test when tiled layers are implemented.
    m_skipsDraw = false;
    if (!layerRenderer()->checkTextureSize(m_bounds)) {
        m_skipsDraw = true;
        return;
    }

    void* pixels = 0;
    IntRect dirtyRect(m_dirtyRect);
    IntSize requiredTextureSize;
    IntSize bitmapSize;

    requiredTextureSize = m_bounds;
    IntRect boundsRect(IntPoint(0, 0), m_bounds);

    // If the texture needs to be reallocated then we must redraw the entire
    // contents of the layer.
    if (requiredTextureSize != m_allocatedTextureSize)
        dirtyRect = boundsRect;
    else {
        // Clip the dirtyRect to the size of the layer to avoid drawing outside
        // the bounds of the backing texture.
        dirtyRect.intersect(boundsRect);
    }

#if PLATFORM(SKIA)
    const SkBitmap* skiaBitmap = 0;
    OwnPtr<skia::PlatformCanvas> canvas;
    OwnPtr<PlatformContextSkia> skiaContext;
    OwnPtr<GraphicsContext> graphicsContext;

    canvas.set(new skia::PlatformCanvas(dirtyRect.width(), dirtyRect.height(), false));
    skiaContext.set(new PlatformContextSkia(canvas.get()));

    // This is needed to get text to show up correctly.
    // FIXME: Does this take us down a very slow text rendering path?
    skiaContext->setDrawingToImageBuffer(true);

    graphicsContext.set(new GraphicsContext(reinterpret_cast<PlatformGraphicsContext*>(skiaContext.get())));

    // Bring the canvas into the coordinate system of the paint rect.
    canvas->translate(static_cast<SkScalar>(-dirtyRect.x()), static_cast<SkScalar>(-dirtyRect.y()));

    m_owner->paintGraphicsLayerContents(*graphicsContext, dirtyRect);
    const SkBitmap& bitmap = canvas->getDevice()->accessBitmap(false);
    skiaBitmap = &bitmap;
    ASSERT(skiaBitmap);

    SkAutoLockPixels lock(*skiaBitmap);
    SkBitmap::Config skiaConfig = skiaBitmap->config();
    // FIXME: do we need to support more image configurations?
    if (skiaConfig == SkBitmap::kARGB_8888_Config) {
        pixels = skiaBitmap->getPixels();
        bitmapSize = IntSize(skiaBitmap->width(), skiaBitmap->height());
    }
#elif PLATFORM(CG)
    Vector<uint8_t> tempVector;
    int rowBytes = 4 * dirtyRect.width();
    tempVector.resize(rowBytes * dirtyRect.height());
    memset(tempVector.data(), 0, tempVector.size());
    RetainPtr<CGColorSpaceRef> colorSpace(AdoptCF, CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGContextRef> contextCG(AdoptCF, CGBitmapContextCreate(tempVector.data(),
                                                                     dirtyRect.width(), dirtyRect.height(), 8, rowBytes,
                                                                     colorSpace.get(),
                                                                     kCGImageAlphaPremultipliedLast));
    CGContextTranslateCTM(contextCG.get(), 0, dirtyRect.height());
    CGContextScaleCTM(contextCG.get(), 1, -1);

    GraphicsContext graphicsContext(contextCG.get());

    // Translate the graphics context into the coordinate system of the dirty rect.
    graphicsContext.translate(-dirtyRect.x(), -dirtyRect.y());

    m_owner->paintGraphicsLayerContents(graphicsContext, dirtyRect);

    pixels = tempVector.data();
    bitmapSize = dirtyRect.size();
#else
#error "Need to implement for your platform."
#endif

    unsigned textureId = m_contentsTexture;
    if (!textureId)
        textureId = layerRenderer()->createLayerTexture();

    if (pixels)
        updateTextureRect(pixels, bitmapSize, requiredTextureSize,  dirtyRect, textureId);
}

static inline bool isPowerOfTwo(int x)
{
    ASSERT(x >= 0);
    return !(x & (x-1));
}

void ContentLayerChromium::updateTextureRect(void* pixels, const IntSize& bitmapSize,
    const IntSize& requiredTextureSize, const IntRect& updateRect, unsigned textureId, MipmapUse requestMipmap)
{
    if (!pixels)
        return;

    GraphicsContext3D* context = layerRendererContext();
    context->bindTexture(GraphicsContext3D::TEXTURE_2D, textureId);

    bool generateMipmap = (requestMipmap == useMipmap)
                          && (layerRenderer()->contentLayerSharedValues()->npotSupported()
                              || (isPowerOfTwo(updateRect.width()) && isPowerOfTwo(updateRect.height())));

    // If the texture id or size changed since last time then we need to tell GL
    // to re-allocate a texture.
    if (m_contentsTexture != textureId || requiredTextureSize != m_allocatedTextureSize) {
        ASSERT(bitmapSize == requiredTextureSize);
        GLC(context, context->texImage2D(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, requiredTextureSize.width(), requiredTextureSize.height(), 0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, pixels));
        if (generateMipmap) {
            GLC(context, context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER,
                GraphicsContext3D::LINEAR_MIPMAP_LINEAR));
            GLC(context, context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER,
                GraphicsContext3D::LINEAR));
        } else {
            GLC(context, context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER,
                 GraphicsContext3D::LINEAR));
            GLC(context, context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER,
                GraphicsContext3D::LINEAR));
        }

        m_contentsTexture = textureId;
        m_allocatedTextureSize = requiredTextureSize;
    } else {
        ASSERT(updateRect.width() <= m_allocatedTextureSize.width() && updateRect.height() <= m_allocatedTextureSize.height());
        ASSERT(updateRect.width() == bitmapSize.width() && updateRect.height() == bitmapSize.height());
        GLC(context, context->texSubImage2D(GraphicsContext3D::TEXTURE_2D, 0, updateRect.x(), updateRect.y(), updateRect.width(), updateRect.height(), GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, pixels));
    }

    if (generateMipmap)
        GLC(context, context->generateMipmap(GraphicsContext3D::TEXTURE_2D));

    m_dirtyRect.setSize(FloatSize());
    m_contentsDirty = false;
}

void ContentLayerChromium::draw()
{
    if (m_skipsDraw)
        return;

    ASSERT(layerRenderer());
    const ContentLayerChromium::SharedValues* sv = layerRenderer()->contentLayerSharedValues();
    ASSERT(sv && sv->initialized());
    GraphicsContext3D* context = layerRendererContext();
    GLC(context, context->activeTexture(GraphicsContext3D::TEXTURE0));
    GLC(context, context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_contentsTexture));
    layerRenderer()->useShader(sv->contentShaderProgram());
    GLC(context, context->uniform1i(sv->shaderSamplerLocation(), 0));
    drawTexturedQuad(context, layerRenderer()->projectionMatrix(), drawTransform(),
                     bounds().width(), bounds().height(), drawOpacity(),
                     sv->shaderMatrixLocation(), sv->shaderAlphaLocation());
}

}
#endif // USE(ACCELERATED_COMPOSITING)
