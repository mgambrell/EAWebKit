/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Igalia S.L.
 Copyright (C) 2012 Adobe Systems Incorporated

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "BitmapTextureGL.h"

#include "Extensions3D.h"
#include "FilterOperations.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "LengthFunctions.h"
#include "NotImplemented.h"
#include "TextureMapperShaderProgram.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/TemporaryChange.h>

#if USE(CAIRO)
#include "CairoUtilities.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <wtf/text/CString.h>
 //MBG HACK
#include "cairo/cairo-gl.h"
#include "EAWebKit\EAWebKit.h"
#include "GraphicsLayer.h"
#include "GLContext.h"
#include "ImageBuffer.h"
#endif

#if !USE(TEXMAP_OPENGL_ES_2)
// FIXME: Move to Extensions3D.h.
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#endif

namespace WebCore {

BitmapTextureGL* toBitmapTextureGL(BitmapTexture* texture)
{
    if (!texture || !texture->isBackedByOpenGL())
        return 0;

    return static_cast<BitmapTextureGL*>(texture);
}

BitmapTextureGL::BitmapTextureGL(PassRefPtr<GraphicsContext3D> context3D)
    : m_id(0)
    , m_fbo(0)
    , m_rbo(0)
    , m_depthBufferObject(0)
    , m_shouldClear(true)
    , m_context3D(context3D)
{
}


//MBG - ADDED BitmapTextureGL VERSION TO DO MORE STUFF ON GPU
void BitmapTextureGL::updateContents(TextureMapper* textureMapper, GraphicsLayer* sourceLayer, const IntRect& targetRect, const IntPoint& offset, UpdateContentsFlag updateContentsFlag)
{
  //FOR REFERENCE: do whatever we did before
  //BitmapTexture::updateContents(textureMapper, sourceLayer, targetRect, offset, updateContentsFlag);
  //return;

  //FOR OPTIMIZATION: paint directly to our managed ImageBuffer

  //what is the meaning of this???
  if(updateContentsFlag == UpdateCannotModifyOriginalImageData)
    abort();

  //this should only happen if we're using tiles and since they default to 2k in size, it wont happen
  if(targetRect.x() != offset.x()
    || targetRect.y() != offset.y())
  {
    abort();
  }

  ////--------------------------
  ////THIS SHOULD WORK, BUT DOESNT. WHY?

  ////paint the needed content to our imageBuffer in the needed spot.
  ////confirmed: clipping and clearing are needed for proper results
  ////confirmed: context save/restore are needed
  ////assumed: the interpolation/text stuff was copied from the base implementation's
  //GraphicsContext* context = imageBuffer->context();
  //context->save();
  //context->setImageInterpolationQuality(textureMapper->imageInterpolationQuality());
  //context->setTextDrawingMode(textureMapper->textDrawingMode());
  //context->clip(targetRect);
  //context->clearRect(targetRect);
  //sourceLayer->paintGraphicsLayerContents(*context, targetRect);
  //context->restore();
  ////--------------------------

  //--------------------------
  //USE THIS INSTEAD.. AN INTERMEDIATE BUFFER. BLEH. AT LEAST IT'S ALL ON GPU

  //1. draw the same way as BitmapTexture does, but to "Accelerated" surface
  std::unique_ptr<ImageBuffer> tempImageBuffer = ImageBuffer::create(targetRect.size(), 1.0f, WebCore::ColorSpaceDeviceRGB, WebCore::RenderingMode::Accelerated);
  GraphicsContext* tempContext = tempImageBuffer->context();
  tempContext->setImageInterpolationQuality(textureMapper->imageInterpolationQuality());
  tempContext->setTextDrawingMode(textureMapper->textDrawingMode());
  tempContext->clearRect(FloatRect(0,0,targetRect.width(),targetRect.height()));
  IntRect sourceRect(targetRect);
  sourceRect.setLocation(offset);
  tempContext->translate(-offset.x(), -offset.y());
  sourceLayer->paintGraphicsLayerContents(*tempContext, sourceRect);

  //2. Copy it onto our own image
  ImagePaintingOptions opts(CompositeOperator::CompositeCopy);
  imageBuffer->context()->drawImageBuffer(tempImageBuffer.get(), WebCore::ColorSpaceDeviceRGB,targetRect,opts);

  //--------------------------

  //QUESTIONABLE CONTEXT
  //well, I debugged it and thought this would be needed because I saw it do work.
  //but it doesn't seem to be important. However, that must be because in the cases I tested there were no pending primitives.
  //I'm leaving this one because I think it must be important.
  //In any event, we have to assume the current GL context is cleared when cairo_device_flush() is called, so pay attention to what comes next
  cairo_device_flush((cairo_device_t*)EA::WebKit::g_cairoDevice);

  //QUESTIONABLE CONTEXT
  //this does not seem necessary, even though I think it should be necessary
  //perhaps TextureMapper and associated types are diligent about setting their context current
  //m_context3D->makeContextCurrent();
}

bool BitmapTextureGL::canReuseWith(const IntSize& contentsSize, Flags)
{
    return contentsSize == m_textureSize;
}

#if OS(DARWIN)
#define DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#else
#define DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE GraphicsContext3D::UNSIGNED_BYTE
#endif

static void swizzleBGRAToRGBA(uint32_t* data, const IntRect& rect, int stride = 0)
{
    stride = stride ? stride : rect.width();
    for (int y = rect.y(); y < rect.maxY(); ++y) {
        uint32_t* p = data + y * stride;
        for (int x = rect.x(); x < rect.maxX(); ++x)
            p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
    }
}

// If GL_EXT_texture_format_BGRA8888 is supported in the OpenGLES
// internal and external formats need to be BGRA
static bool driverSupportsExternalTextureBGRA(GraphicsContext3D* context)
{
    if (context->isGLES2Compliant()) {
        static bool supportsExternalTextureBGRA = context->getExtensions()->supports("GL_EXT_texture_format_BGRA8888");
        return supportsExternalTextureBGRA;
    }

    return true;
}

static bool driverSupportsSubImage(GraphicsContext3D* context)
{
    if (context->isGLES2Compliant()) {
        static bool supportsSubImage = context->getExtensions()->supports("GL_EXT_unpack_subimage");
        return supportsSubImage;
    }

    return true;
}

//MBG - changed this to just be a wrapper on ImageBuffer. this makes it ready-to-go for rendering onto an ImageBuffer
void BitmapTextureGL::didReset()
{
  m_shouldClear = true;

  if (m_textureSize == contentSize())
    return;

  if(m_fbo)
    m_context3D->deleteFramebuffer(m_fbo);

  m_textureSize = contentSize();

  imageBuffer = ImageBuffer::create(m_textureSize, 1.0f, WebCore::ColorSpaceDeviceRGB, WebCore::RenderingMode::Accelerated);
  m_id = imageBuffer->GetData().m_texture;

  //MBG - BIG PROBLEMS HAPPEN IF I DO THIS!
  //at least for now, probably because it's messing up the current GL context or something
  //createFboIfNeeded();

  //MBG - REMINDER - IS THIS IMPORTANT? MAYBE, FOR THE FILTERING... BUT...
  //THIS IS ONLY USED FOR COMPOSITING AND COMPOSITING SHOULD RUN AT 1X, SO...?
  //check createCairoGLSurface() to see what it does by default.
  //m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
  //m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
  //m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
  //m_context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
}

void BitmapTextureGL::updateContentsNoSwizzle(const void* srcData, const IntRect& targetRect, const IntPoint& sourceOffset, int bytesPerLine, unsigned bytesPerPixel, Platform3DObject glFormat)
{
    m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, m_id);
    // For ES drivers that don't support sub-images.
    if (driverSupportsSubImage(m_context3D.get())) {
        // Use the OpenGL sub-image extension, now that we know it's available.
        m_context3D->pixelStorei(GL_UNPACK_ROW_LENGTH, bytesPerLine / bytesPerPixel);
        m_context3D->pixelStorei(GL_UNPACK_SKIP_ROWS, sourceOffset.y());
        m_context3D->pixelStorei(GL_UNPACK_SKIP_PIXELS, sourceOffset.x());
    }

    m_context3D->texSubImage2D(GraphicsContext3D::TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, srcData);

    // For ES drivers that don't support sub-images.
    if (driverSupportsSubImage(m_context3D.get())) {
        m_context3D->pixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        m_context3D->pixelStorei(GL_UNPACK_SKIP_ROWS, 0);
        m_context3D->pixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    }
}

void BitmapTextureGL::updateContents(const void* srcData, const IntRect& targetRect, const IntPoint& sourceOffset, int bytesPerLine, UpdateContentsFlag updateContentsFlag)
{
    Platform3DObject glFormat = GraphicsContext3D::RGBA;
    m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, m_id);

    const unsigned bytesPerPixel = 4;
    char* data = reinterpret_cast<char*>(const_cast<void*>(srcData));
    Vector<char> temporaryData;
    IntPoint adjustedSourceOffset = sourceOffset;

    // Texture upload requires subimage buffer if driver doesn't support subimage and we don't have full image upload.
    bool requireSubImageBuffer = !driverSupportsSubImage(m_context3D.get())
        && !(bytesPerLine == static_cast<int>(targetRect.width() * bytesPerPixel) && adjustedSourceOffset == IntPoint::zero());

    // prepare temporaryData if necessary
    if ((!driverSupportsExternalTextureBGRA(m_context3D.get()) && updateContentsFlag == UpdateCannotModifyOriginalImageData) || requireSubImageBuffer) {
        temporaryData.resize(targetRect.width() * targetRect.height() * bytesPerPixel);
        data = temporaryData.data();
        const char* bits = static_cast<const char*>(srcData);
        const char* src = bits + sourceOffset.y() * bytesPerLine + sourceOffset.x() * bytesPerPixel;
        char* dst = data;
        const int targetBytesPerLine = targetRect.width() * bytesPerPixel;
        for (int y = 0; y < targetRect.height(); ++y) {
            memcpy(dst, src, targetBytesPerLine);
            src += bytesPerLine;
            dst += targetBytesPerLine;
        }

        bytesPerLine = targetBytesPerLine;
        adjustedSourceOffset = IntPoint(0, 0);
    }

    if (driverSupportsExternalTextureBGRA(m_context3D.get()))
        glFormat = GraphicsContext3D::BGRA;
    else
        swizzleBGRAToRGBA(reinterpret_cast_ptr<uint32_t*>(data), IntRect(adjustedSourceOffset, targetRect.size()), bytesPerLine / bytesPerPixel);

    updateContentsNoSwizzle(data, targetRect, adjustedSourceOffset, bytesPerLine, bytesPerPixel, glFormat);
}

void BitmapTextureGL::updateContents(Image* image, const IntRect& targetRect, const IntPoint& offset, UpdateContentsFlag updateContentsFlag)
{
    if (!image)
        return;
    NativeImagePtr frameImage = image->nativeImageForCurrentFrame();
    if (!frameImage)
        return;

    int bytesPerLine;
    const char* imageData;

#if USE(CAIRO)
    cairo_surface_t* surface = frameImage.get();
    imageData = reinterpret_cast<const char*>(cairo_image_surface_get_data(surface));
    bytesPerLine = cairo_image_surface_get_stride(surface);

    //MBG - added this, so we can support copying GL surfaces
    //it's not ideal, but it helps me change one piece at a time
    RefPtr<cairo_surface_t> tempSurface;
    if(!imageData)
    {
      tempSurface = copyCairoImageSurface(surface);
      imageData = reinterpret_cast<const char*>(cairo_image_surface_get_data(tempSurface.get()));
      bytesPerLine = cairo_image_surface_get_stride(tempSurface.get());
    }

#endif

    updateContents(imageData, targetRect, offset, bytesPerLine, updateContentsFlag);
}

static unsigned getPassesRequiredForFilter(FilterOperation::OperationType type)
{
    switch (type) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SEPIA:
    case FilterOperation::SATURATE:
    case FilterOperation::HUE_ROTATE:
    case FilterOperation::INVERT:
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::OPACITY:
        return 1;
    case FilterOperation::BLUR:
    case FilterOperation::DROP_SHADOW:
        // We use two-passes (vertical+horizontal) for blur and drop-shadow.
        return 2;
    default:
        return 0;
    }
}

PassRefPtr<BitmapTexture> BitmapTextureGL::applyFilters(TextureMapper* textureMapper, const FilterOperations& filters)
{
    if (filters.isEmpty())
        return this;

    TextureMapperGL* texmapGL = static_cast<TextureMapperGL*>(textureMapper);
    RefPtr<BitmapTexture> previousSurface = texmapGL->currentSurface();
    RefPtr<BitmapTexture> resultSurface = this;
    RefPtr<BitmapTexture> intermediateSurface;
    RefPtr<BitmapTexture> spareSurface;

    m_filterInfo = FilterInfo();

    for (size_t i = 0; i < filters.size(); ++i) {
        RefPtr<FilterOperation> filter = filters.operations()[i];
        ASSERT(filter);

        int numPasses = getPassesRequiredForFilter(filter->type());
        for (int j = 0; j < numPasses; ++j) {
            bool last = (i == filters.size() - 1) && (j == numPasses - 1);
            if (!last) {
                if (!intermediateSurface)
                    intermediateSurface = texmapGL->acquireTextureFromPool(contentSize());
                texmapGL->bindSurface(intermediateSurface.get());
            }

            if (last) {
                toBitmapTextureGL(resultSurface.get())->m_filterInfo = BitmapTextureGL::FilterInfo(filter, j, spareSurface);
                break;
            }

            texmapGL->drawFiltered(*resultSurface.get(), spareSurface.get(), *filter, j);
            if (!j && filter->type() == FilterOperation::DROP_SHADOW) {
                spareSurface = resultSurface;
                resultSurface = nullptr;
            }
            std::swap(resultSurface, intermediateSurface);
        }
    }

    texmapGL->bindSurface(previousSurface.get());
    return resultSurface;
}

void BitmapTextureGL::initializeStencil()
{
    if (m_rbo)
        return;

    m_rbo = m_context3D->createRenderbuffer();
    m_context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_rbo);
#ifdef TEXMAP_OPENGL_ES_2
    m_context3D->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::STENCIL_INDEX8, m_textureSize.width(), m_textureSize.height());
#else
    m_context3D->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_STENCIL, m_textureSize.width(), m_textureSize.height());
#endif
    m_context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);
    m_context3D->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_rbo);
    m_context3D->clearStencil(0);
    m_context3D->clear(GraphicsContext3D::STENCIL_BUFFER_BIT);
}

void BitmapTextureGL::initializeDepthBuffer()
{
    if (m_depthBufferObject)
        return;

    m_depthBufferObject = m_context3D->createRenderbuffer();
    m_context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_depthBufferObject);
    m_context3D->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_COMPONENT16, m_textureSize.width(), m_textureSize.height());
    m_context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);
    m_context3D->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_depthBufferObject);
}

void BitmapTextureGL::clearIfNeeded()
{
    if (!m_shouldClear)
        return;

    m_clipStack.reset(IntRect(IntPoint::zero(), m_textureSize), TextureMapperGL::ClipStack::DefaultYAxis);
    m_clipStack.applyIfNeeded(m_context3D.get());
    m_context3D->clearColor(0, 0, 0, 0);
    m_context3D->clear(GraphicsContext3D::COLOR_BUFFER_BIT);
    m_shouldClear = false;
}

void BitmapTextureGL::createFboIfNeeded()
{
    if (m_fbo)
        return;

    m_fbo = m_context3D->createFramebuffer();
    m_context3D->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    m_context3D->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, id(), 0);
    m_shouldClear = true;
}

void BitmapTextureGL::bindAsSurface(GraphicsContext3D* context3D)
{
    context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, 0);
    createFboIfNeeded();
    context3D->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    context3D->viewport(0, 0, m_textureSize.width(), m_textureSize.height());
    clearIfNeeded();
    m_clipStack.apply(m_context3D.get());
}

BitmapTextureGL::~BitmapTextureGL()
{
    //MBG - this is owned by the ImageBuffer now
    //if (m_id)
    //    m_context3D->deleteTexture(m_id);

    if (m_fbo)
        m_context3D->deleteFramebuffer(m_fbo);

    if (m_rbo)
        m_context3D->deleteRenderbuffer(m_rbo);

    if (m_depthBufferObject)
        m_context3D->deleteRenderbuffer(m_depthBufferObject);
}

bool BitmapTextureGL::isValid() const
{
    return m_id;
}

IntSize BitmapTextureGL::size() const
{
    return m_textureSize;
}

}; // namespace WebCore
