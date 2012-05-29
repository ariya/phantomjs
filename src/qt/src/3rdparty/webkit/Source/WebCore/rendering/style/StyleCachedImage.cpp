/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "StyleCachedImage.h"

#include "CachedImage.h"
#include "RenderObject.h"

namespace WebCore {

PassRefPtr<CSSValue> StyleCachedImage::cssValue() const
{
    return CSSPrimitiveValue::create(m_image->url(), CSSPrimitiveValue::CSS_URI);
}

bool StyleCachedImage::canRender(float multiplier) const
{
    return m_image->canRender(multiplier);
}

bool StyleCachedImage::isLoaded() const
{
    return m_image->isLoaded();
}

bool StyleCachedImage::errorOccurred() const
{
    return m_image->errorOccurred();
}

IntSize StyleCachedImage::imageSize(const RenderObject* /*renderer*/, float multiplier) const
{
    return m_image->imageSize(multiplier);
}

bool StyleCachedImage::imageHasRelativeWidth() const
{
    return m_image->imageHasRelativeWidth();
}

bool StyleCachedImage::imageHasRelativeHeight() const
{
    return m_image->imageHasRelativeHeight();
}

bool StyleCachedImage::usesImageContainerSize() const
{
    return m_image->usesImageContainerSize();
}

void StyleCachedImage::setImageContainerSize(const IntSize& size)
{
    return m_image->setImageContainerSize(size);
}

void StyleCachedImage::addClient(RenderObject* renderer)
{
    return m_image->addClient(renderer);
}

void StyleCachedImage::removeClient(RenderObject* renderer)
{
    return m_image->removeClient(renderer);
}

PassRefPtr<Image> StyleCachedImage::image(RenderObject*, const IntSize&) const
{
    return m_image->image();
}

}
