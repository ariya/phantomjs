/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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
#include "TextureMapperImageBuffer.h"

#include "GraphicsLayer.h"
#if PLATFORM(QT)
#include "NativeImageQt.h"
#endif
#include "NotImplemented.h"


#if USE(TEXTURE_MAPPER)
namespace WebCore {

static const int s_maximumAllowedImageBufferDimension = 4096;

void BitmapTextureImageBuffer::updateContents(const void* data, const IntRect& targetRect, const IntPoint& sourceOffset, int bytesPerLine, UpdateContentsFlag)
{
#if PLATFORM(QT)
    QImage image(reinterpret_cast<const uchar*>(data), targetRect.width(), targetRect.height(), bytesPerLine, NativeImageQt::defaultFormatForAlphaEnabledImages());

    QPainter* painter = m_image->context()->platformContext();
    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->drawImage(targetRect, image, IntRect(sourceOffset, targetRect.size()));
    painter->restore();
#elif PLATFORM(CAIRO)
    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(static_cast<unsigned char*>(data()),
                                                                                   CAIRO_FORMAT_ARGB32,
                                                                                   targetRect.width(), targetRect.height(),
                                                                                   bytesPerLine));
    m_image->context()->platformContext()->drawSurfaceToContext(surface.get(), targetRect,
                                                                IntRect(sourceOffset, targetRect.size()), m_image->context());
#else
    UNUSED_PARAM(data);
    UNUSED_PARAM(targetRect);
    UNUSED_PARAM(sourceOffset);
    UNUSED_PARAM(bytesPerLine);
#endif
}

void BitmapTextureImageBuffer::updateContents(TextureMapper*, GraphicsLayer* sourceLayer, const IntRect& targetRect, const IntPoint& sourceOffset, UpdateContentsFlag)
{
    GraphicsContext* context = m_image->context();

    context->clearRect(targetRect);

    IntRect sourceRect(targetRect);
    sourceRect.setLocation(sourceOffset);
    context->save();
    context->clip(targetRect);
    context->translate(targetRect.x() - sourceOffset.x(), targetRect.y() - sourceOffset.y());
    sourceLayer->paintGraphicsLayerContents(*context, sourceRect);
    context->restore();
}

void BitmapTextureImageBuffer::didReset()
{
    m_image = ImageBuffer::create(contentSize());
}

void BitmapTextureImageBuffer::updateContents(Image* image, const IntRect& targetRect, const IntPoint& offset, UpdateContentsFlag)
{
    m_image->context()->drawImage(image, ColorSpaceDeviceRGB, targetRect, IntRect(offset, targetRect.size()), CompositeCopy);
}

IntSize TextureMapperImageBuffer::maxTextureSize() const
{
    return IntSize(s_maximumAllowedImageBufferDimension, s_maximumAllowedImageBufferDimension);
}

void TextureMapperImageBuffer::beginClip(const TransformationMatrix& matrix, const FloatRect& rect)
{
    GraphicsContext* context = currentContext();
    if (!context)
        return;
#if ENABLE(3D_RENDERING)
    TransformationMatrix previousTransform = context->get3DTransform();
#else
    AffineTransform previousTransform = context->getCTM();
#endif
    context->save();

#if ENABLE(3D_RENDERING)
    context->concat3DTransform(matrix);
#else
    context->concatCTM(matrix.toAffineTransform());
#endif

    context->clip(rect);

#if ENABLE(3D_RENDERING)
    context->set3DTransform(previousTransform);
#else
    context->setCTM(previousTransform);
#endif
}

void TextureMapperImageBuffer::endClip()
{
    GraphicsContext* context = currentContext();
    if (!context)
        return;

    context->restore();
}

void TextureMapperImageBuffer::drawTexture(const BitmapTexture& texture, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity, unsigned /* exposedEdges */)
{
    GraphicsContext* context = currentContext();
    if (!context)
        return;

    const BitmapTextureImageBuffer& textureImageBuffer = static_cast<const BitmapTextureImageBuffer&>(texture);
    ImageBuffer* image = textureImageBuffer.m_image.get();
    context->save();
    context->setCompositeOperation(isInMaskMode() ? CompositeDestinationIn : CompositeSourceOver);
    context->setAlpha(opacity);
#if ENABLE(3D_RENDERING)
    context->concat3DTransform(matrix);
#else
    context->concatCTM(matrix.toAffineTransform());
#endif
    context->drawImageBuffer(image, ColorSpaceDeviceRGB, targetRect);
    context->restore();
}

void TextureMapperImageBuffer::drawSolidColor(const FloatRect& rect, const TransformationMatrix& matrix, const Color& color)
{
    GraphicsContext* context = currentContext();
    if (!context)
        return;

    context->save();
    context->setCompositeOperation(isInMaskMode() ? CompositeDestinationIn : CompositeSourceOver);
#if ENABLE(3D_RENDERING)
    context->concat3DTransform(matrix);
#else
    context->concatCTM(matrix.toAffineTransform());
#endif

    context->fillRect(rect, color, ColorSpaceDeviceRGB);
    context->restore();
}

void TextureMapperImageBuffer::drawBorder(const Color&, float /* borderWidth */, const FloatRect&, const TransformationMatrix&)
{
    notImplemented();
}

void TextureMapperImageBuffer::drawNumber(int /* number */, const Color&, const FloatPoint&, const TransformationMatrix&)
{
    notImplemented();
}

#if ENABLE(CSS_FILTERS)
PassRefPtr<BitmapTexture> BitmapTextureImageBuffer::applyFilters(TextureMapper*, const FilterOperations& filters)
{
    ASSERT_UNUSED(filters, filters.isEmpty());

    return this;
}
#endif

}
#endif
