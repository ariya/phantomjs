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
#include "CSSBorderImageValue.h"

#include "PlatformString.h"
#include "Rect.h"

namespace WebCore {

CSSBorderImageValue::CSSBorderImageValue(PassRefPtr<CSSValue> image, PassRefPtr<Rect> imageRect, int horizontalRule, int verticalRule)
    : m_image(image)
    , m_imageSliceRect(imageRect)
    , m_horizontalSizeRule(horizontalRule)
    , m_verticalSizeRule(verticalRule)
{
}

CSSBorderImageValue::~CSSBorderImageValue()
{
}

String CSSBorderImageValue::cssText() const
{
    // Image first.
    String text(m_image->cssText());
    text += " ";

    // Now the rect, but it isn't really a rect, so we dump manually
    text += m_imageSliceRect->top()->cssText();
    text += " ";
    text += m_imageSliceRect->right()->cssText();
    text += " ";
    text += m_imageSliceRect->bottom()->cssText();
    text += " ";
    text += m_imageSliceRect->left()->cssText();

    // Now the keywords.
    text += " ";
    text += CSSPrimitiveValue::createIdentifier(m_horizontalSizeRule)->cssText();
    text += " ";
    text += CSSPrimitiveValue::createIdentifier(m_verticalSizeRule)->cssText();

    return text;
}

void CSSBorderImageValue::addSubresourceStyleURLs(ListHashSet<KURL>& urls, const CSSStyleSheet* styleSheet)
{
    m_image->addSubresourceStyleURLs(urls, styleSheet);
}

} // namespace WebCore
