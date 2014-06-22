/*
 * Copyright (C) 1999 Lars Knoll <knoll@kde.org>
 * Copyright (C) 1999 Antti Koivisto <koivisto@kde.org>
 * Copyright (C) 2000 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Allan Sandfeld Jensen <kde@carewolf.com>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
#include "RenderImageResource.h"

#include "CachedImage.h"
#include "Image.h"
#include "RenderImageResourceStyleImage.h"
#include "RenderObject.h"

namespace WebCore {

RenderImageResource::RenderImageResource()
    : m_renderer(0)
{
}

RenderImageResource::~RenderImageResource()
{
}

void RenderImageResource::initialize(RenderObject* renderer)
{
    ASSERT(!m_renderer);
    ASSERT(renderer);
    m_renderer = renderer;
}

void RenderImageResource::shutdown()
{
    ASSERT(m_renderer);

    if (m_cachedImage)
        m_cachedImage->removeClient(m_renderer);
}

void RenderImageResource::setCachedImage(CachedImage* newImage)
{
    ASSERT(m_renderer);

    if (m_cachedImage == newImage)
        return;

    if (m_cachedImage)
        m_cachedImage->removeClient(m_renderer);
    m_cachedImage = newImage;
    if (m_cachedImage) {
        m_cachedImage->addClient(m_renderer);
        if (m_cachedImage->errorOccurred())
            m_renderer->imageChanged(m_cachedImage.get());
    } else {
        m_renderer->imageChanged(m_cachedImage.get());
    }
}

void RenderImageResource::resetAnimation()
{
    ASSERT(m_renderer);

    if (!m_cachedImage)
        return;

    image()->resetAnimation();

    if (!m_renderer->needsLayout())
        m_renderer->repaint();
}

PassRefPtr<Image> RenderImageResource::image(int, int) const
{
    return m_cachedImage ? m_cachedImage->imageForRenderer(m_renderer) : nullImage();
}

bool RenderImageResource::errorOccurred() const
{
    return m_cachedImage && m_cachedImage->errorOccurred();
}

void RenderImageResource::setContainerSizeForRenderer(const IntSize& imageContainerSize)
{
    ASSERT(m_renderer);
    if (m_cachedImage)
        m_cachedImage->setContainerSizeForRenderer(m_renderer, imageContainerSize, m_renderer->style()->effectiveZoom());
}

Image* RenderImageResource::nullImage()
{
    return Image::nullImage();
}

bool RenderImageResource::usesImageContainerSize() const
{
    return m_cachedImage ? m_cachedImage->usesImageContainerSize() : false;
}

bool RenderImageResource::imageHasRelativeWidth() const
{
    return m_cachedImage ? m_cachedImage->imageHasRelativeWidth() : false;
}

bool RenderImageResource::imageHasRelativeHeight() const
{
    return m_cachedImage ? m_cachedImage->imageHasRelativeHeight() : false;
}

LayoutSize RenderImageResource::imageSize(float multiplier) const
{
    return m_cachedImage ? m_cachedImage->imageSizeForRenderer(m_renderer, multiplier) : LayoutSize();
}

} // namespace WebCore
