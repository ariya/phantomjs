/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "config.h"
#include "ImageBuffer.h"

#include "Base64.h"
#include "BitmapImage.h"
#include "HostWindow.h"
#include "ImageData.h"
#include "JPEGImageEncoder.h"
#include "LayerMessage.h"
#include "PNGImageEncoder.h"
#include "TiledImage.h"

#include <BlackBerryPlatformGLES2ContextState.h>
#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformGraphicsContext.h>
#include <BlackBerryPlatformSettings.h>
#include <BlackBerryPlatformWindow.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

using namespace std;

namespace WebCore {

static bool makeBufferCurrent(HostWindow* window)
{
    ASSERT(isCompositingThread());

    if (window && window->platformPageClient()) {
        if (BlackBerry::Platform::Graphics::Window* platformWindow = window->platformPageClient()->platformWindow()) {
            if (BlackBerry::Platform::Graphics::makeBufferCurrent(platformWindow->buffer(), BlackBerry::Platform::Graphics::GLES2))
                return true;
        }
    }

    // If we don't have a window but the application state saver is enabled,
    // we're using a BlackBerry::WebKit::WebPageCompositor for rendering,
    // that makes sure a context is always current on the compositing thread.
    if (BlackBerry::Platform::Settings::isGLES2AppStateSaverEnabled())
        return true;

    // Otherwise, we'll have to try and fall back to our last hope.
    return BlackBerry::Platform::Graphics::makeSharedResourceContextCurrent(BlackBerry::Platform::Graphics::GLES2);
}

static bool getImageDataInternal(GraphicsContext* context, const IntRect& rect, const IntRect& size, unsigned char* result, bool unmultiply, HostWindow* window)
{
    if (!makeBufferCurrent(window)) {
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelWarn,
            "ImageBufferBlackBerry getImageDataInternal() error: cannot make buffer current, returning zeroed-out pixels.");
        memset(result, 0, rect.width() * rect.height() * 4);
        return false;
    }

    // readPixels can call updateBacking which draws the display list. This is out-of-band rendering and we need
    // to protect the embedder from state mutations.
    BlackBerry::Platform::Graphics::GLES2ContextState::AppStateSaver appStateSaver;

    // Any pixels outside the canvas are returned as transparent black in the resulting ImageData object.
    if (rect.x() < 0
        || rect.y() < 0
        || rect.maxX() > size.width()
        || rect.maxY() > size.height())
        memset(result, 0, rect.width() * rect.height() * 4);

    IntRect subrect = rect;
    subrect.intersect(size);
    // nothing to do if rect is completely outside of the image area
    if (subrect.isEmpty())
        return true;
    // adjust for negative x and y index
    if (rect.x() < 0)
        result += (-rect.x() * 4);
    if (rect.y() < 0)
        result += (-rect.y() * rect.width() * 4);

    // FIXME: We need to implement a tiled buffer.
    const int maxTileSize = BlackBerry::Platform::Graphics::TiledImage::defaultTileSize();
    IntRect r = subrect;
    IntRect tileRect(0, 0, maxTileSize, maxTileSize);
    r.intersect(tileRect);
    if (!r.isEmpty())
        context->platformContext()->readPixels(r, result, unmultiply);
    return true;
}

void ImageBufferData::getImageData(GraphicsContext* context, const IntRect& rect, const IntRect& size, unsigned char* result, bool unmultiply) const
{
    if (!isCompositingThread()) {
        // Use createFunctionCallMessage instead of createMethodCallMessage to avoid deadlock
        // with Guarded pointers.
        dispatchSyncCompositingMessage(BlackBerry::Platform::createFunctionCallMessage(
            &getImageDataInternal, context, rect, size, result, unmultiply, m_window));
        return;
    }

    getImageDataInternal(context, rect, size, result, unmultiply, m_window);
}

static bool flushAndDraw(const ImageBufferData* object, GraphicsContext* context, ColorSpace, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, bool)
{
    if (!makeBufferCurrent(object->m_window)) {
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelWarn,
            "ImageBufferBlackBerry flushAndDraw() error: cannot make buffer current, ignoring call.");
        return false;
    }

    using namespace BlackBerry::Platform::Graphics;

    // flushAndDrawBuffer calls updateBacking which draws the display list. This is out-of-band rendering and we need
    // to protect the embedder from state mutations.
    BlackBerry::Platform::Graphics::GLES2ContextState::AppStateSaver appStateSaver;

    CompositeOperator oldOperator = context->compositeOperation();
    context->setCompositeOperation(op);
    context->platformContext()->flushAndDrawBuffer(object->m_buffer, destRect, srcRect);
    context->setCompositeOperation(oldOperator);
    return true;
}

void ImageBufferData::draw(GraphicsContext* thisContext, GraphicsContext* otherContext, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, bool useLowQualityScale) const
{
    const FloatRect normDestRect = destRect.normalized();
    const FloatRect normSrcRect = srcRect.normalized();

    if (normSrcRect.isEmpty() || normDestRect.isEmpty())
        return; // Nothing to draw.

    if (thisContext->platformContext()->isEmpty() && platformBufferHandle(m_buffer)) {
        CompositeOperator oldOperator = otherContext->compositeOperation();
        otherContext->setCompositeOperation(op);
        otherContext->platformContext()->drawBuffer(m_buffer, normDestRect, normSrcRect);
        otherContext->setCompositeOperation(oldOperator);
    } else {
        dispatchSyncCompositingMessage(BlackBerry::Platform::createFunctionCallMessage(
            &flushAndDraw, this, otherContext, styleColorSpace, normDestRect, normSrcRect, op, useLowQualityScale));
    }
}

ImageBuffer::ImageBuffer(const IntSize& size, float, ColorSpace, RenderingMode renderingMode, HostWindow* window, bool& success)
    : m_size(size)
    , m_logicalSize(size)
    , m_resolutionScale(1)
{
    success = false;

    // There is no explicitly defined default ctor for ImageBufferData::m_data,
    // on purpose - precaution. Hence upon creation m_data will contain garbage
    // leading to crash up dtor. To avoid the case, initilize it explicitly.
    m_data.m_buffer = 0;
    m_data.m_platformLayer = nullptr;
    m_data.m_window = 0;

    if (m_size.isEmpty())
        return;

    // anything bigger then defaultTileSize is just a problem for the HW.
    const int maxTileSize = BlackBerry::Platform::Graphics::TiledImage::defaultTileSize();
    if (maxTileSize <= size.width() || maxTileSize <= size.height()) {
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelWarn, "Requested ImageBuffer size [%d %d] is too big. HW allows up to [%d %d].",
            size.width(), size.height(), maxTileSize, maxTileSize);
    }

    m_data.m_buffer = BlackBerry::Platform::Graphics::createBuffer(m_size, BlackBerry::Platform::Graphics::AlwaysBacked);
    m_data.m_platformLayer = CanvasLayerWebKitThread::create(m_data.m_buffer, m_size);
    m_data.m_window = window;
    m_context = adoptPtr(new GraphicsContext(lockBufferDrawable(m_data.m_buffer)));
    m_context->scale(FloatSize(m_resolutionScale, m_resolutionScale));
    m_context->setIsAcceleratedContext(renderingMode == Accelerated);
    success = true;
}

ImageBuffer::~ImageBuffer()
{
    m_context.clear();

    // Ensure that we clear the buffer in the compositing thread before the buffer is destroyed.
    if (m_data.m_platformLayer) {
        dispatchSyncCompositingMessage(BlackBerry::Platform::createFunctionCallMessage(
            &CanvasLayerWebKitThread::clearBuffer, m_data.m_platformLayer.get()));
    }

    BlackBerry::Platform::Graphics::destroyBuffer(m_data.m_buffer);
    m_data.m_buffer = 0;
}

GraphicsContext* ImageBuffer::context() const
{
    return m_context.get();
}

PlatformLayer* ImageBuffer::platformLayer() const
{
    return m_data.m_platformLayer.get();
}

PassRefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy, ScaleBehavior) const
{
    // FIXME respect copyBehaviour enum.
    Vector<unsigned> pixels;
    pixels.reserveCapacity(m_size.area());
    m_data.getImageData(m_context.get(), IntRect(IntPoint(0, 0), m_size), IntRect(IntPoint(0, 0), m_size), reinterpret_cast<unsigned char*>(pixels.data()), false /* unmultiply */);
    return BitmapImage::create(new BlackBerry::Platform::Graphics::TiledImage(m_size, pixels.data(), false /* dataIsBGRA */));
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& rect) const
{
    WTF_ALIGNED(unsigned char*, imageData, 4) = new unsigned char[m_size.width() * m_size.height() * 4];
    m_data.getImageData(m_context.get(), IntRect(IntPoint(0, 0), m_size), IntRect(IntPoint(0, 0), m_size), imageData, false /* unmultiply */);
    BlackBerry::Platform::Graphics::TiledImage* nativeImage = new BlackBerry::Platform::Graphics::TiledImage(m_size, reinterpret_cast_ptr<unsigned*>(imageData), false /* dataIsBGRA */);
    context->platformContext()->addMaskLayer(rect, nativeImage);
}

void ImageBuffer::draw(GraphicsContext* context, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, BlendMode, bool useLowQualityScale)
{
    m_data.draw(m_context.get(), context, styleColorSpace, destRect, srcRect, op, useLowQualityScale);
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    RefPtr<Image> image = copyImage(DontCopyBackingStore);
    image->drawPattern(context, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>&)
{
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    m_data.getImageData(m_context.get(), rect, IntRect(IntPoint(0, 0), m_size), result->data(), true);
    return result;
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    m_data.getImageData(m_context.get(), rect, IntRect(IntPoint(0, 0), m_size), result->data(), false);
    return result;
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem)
{
    BlackBerry::Platform::Graphics::TiledImage image(sourceSize, reinterpret_cast_ptr<unsigned*>(source->data()), false /* this data is RGBA, not BGRA */, multiplied == Premultiplied);
    m_context->platformContext()->save();
    const double matrix[] = { 1., 0., 0., 1., 0., 0. };
    m_context->platformContext()->setTransform(matrix);
    m_context->platformContext()->setAlpha(1.0);
    m_context->platformContext()->resetClip();
    m_context->platformContext()->setCompositeOperation(BlackBerry::Platform::Graphics::CompositeCopy);
    m_context->platformContext()->addImage(FloatRect(destPoint + sourceRect.location(), sourceRect.size()), FloatRect(sourceRect), &image);
    m_context->platformContext()->restore();
}

String ImageBuffer::toDataURL(const String& mimeType, const double* quality, CoordinateSystem) const
{
    if (m_size.isEmpty())
        return "data:,";

    enum {
        EncodeJPEG,
        EncodePNG,
    } encodeType = mimeType.lower() == "image/png" ? EncodePNG : EncodeJPEG;

    // According to http://www.w3.org/TR/html5/the-canvas-element.html,
    // "For image types that do not support an alpha channel, the image must be"
    // "composited onto a solid black background using the source-over operator,"
    // "and the resulting image must be the one used to create the data: URL."
    // JPEG doesn't have alpha channel, so we need premultiplied data.
    RefPtr<Uint8ClampedArray> imageData = encodeType == EncodePNG
        ? getUnmultipliedImageData(IntRect(IntPoint(0, 0), m_size))
        : getPremultipliedImageData(IntRect(IntPoint(0, 0), m_size));

    Vector<char> output;
    const char* header;
    if (encodeType == EncodePNG) {
        if (!compressRGBABigEndianToPNG(imageData->data(), m_size, output))
            return "data:,";
        header = "data:image/png;base64,";
    } else {
        if (!compressRGBABigEndianToJPEG(imageData->data(), m_size, output, quality))
            return "data:,";
        header = "data:image/jpeg;base64,";
    }

    Vector<char> base64;
    base64Encode(output, base64);

    output.clear();

    Vector<char> url;
    url.append(header, strlen(header));
    url.append(base64);

    return String(url.data(), url.size());
}

} // namespace WebCore
