/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "CSSImageValue.h"

#include "CSSValueKeywords.h"
#include "MemoryCache.h"
#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "StyleCachedImage.h"
#include "StylePendingImage.h"

namespace WebCore {

CSSImageValue::CSSImageValue(const String& url)
    : CSSPrimitiveValue(url, CSS_URI)
    , m_accessedImage(false)
{
}

CSSImageValue::CSSImageValue()
    : CSSPrimitiveValue(CSSValueNone)
    , m_accessedImage(true)
{
}

CSSImageValue::~CSSImageValue()
{
    if (m_image && m_image->isCachedImage())
        static_cast<StyleCachedImage*>(m_image.get())->cachedImage()->removeClient(this);
}

StyleImage* CSSImageValue::cachedOrPendingImage()
{
    if (getIdent() == CSSValueNone)
        return 0;

    if (!m_image)
        m_image = StylePendingImage::create(this);

    return m_image.get();
}

StyleCachedImage* CSSImageValue::cachedImage(CachedResourceLoader* loader)
{
    return cachedImage(loader, getStringValue());
}

StyleCachedImage* CSSImageValue::cachedImage(CachedResourceLoader* loader, const String& url)
{
    ASSERT(loader);

    if (!m_accessedImage) {
        m_accessedImage = true;

        if (CachedImage* cachedImage = loader->requestImage(url)) {
            cachedImage->addClient(this);
            m_image = StyleCachedImage::create(cachedImage);
        }
    }
    
    return (m_image && m_image->isCachedImage()) ? static_cast<StyleCachedImage*>(m_image.get()) : 0;
}

String CSSImageValue::cachedImageURL()
{
    if (!m_image || !m_image->isCachedImage())
        return String();
    return static_cast<StyleCachedImage*>(m_image.get())->cachedImage()->url();
}

void CSSImageValue::clearCachedImage()
{
    if (m_image && m_image->isCachedImage())
        static_cast<StyleCachedImage*>(m_image.get())->cachedImage()->removeClient(this);
    m_image = 0;
    m_accessedImage = false;
}

} // namespace WebCore
