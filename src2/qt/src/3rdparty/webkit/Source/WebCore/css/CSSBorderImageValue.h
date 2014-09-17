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

#ifndef CSSBorderImageValue_h
#define CSSBorderImageValue_h

#include "CSSValue.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Rect;

class CSSBorderImageValue : public CSSValue {
public:
    static PassRefPtr<CSSBorderImageValue> create(PassRefPtr<CSSValue> image, PassRefPtr<Rect> sliceRect, int horizontalRule, int verticalRule)
    {
        return adoptRef(new CSSBorderImageValue(image, sliceRect, horizontalRule, verticalRule));
    }
    virtual ~CSSBorderImageValue();

    virtual String cssText() const;

    CSSValue* imageValue() const { return m_image.get(); }

    virtual void addSubresourceStyleURLs(ListHashSet<KURL>&, const CSSStyleSheet*);

    // The border image.
    RefPtr<CSSValue> m_image;

    // These four values are used to make "cuts" in the image.  They can be numbers
    // or percentages.
    RefPtr<Rect> m_imageSliceRect;

    // Values for how to handle the scaling/stretching/tiling of the image slices.
    int m_horizontalSizeRule; // Rule for how to adjust the widths of the top/middle/bottom
    int m_verticalSizeRule; // Rule for how to adjust the heights of the left/middle/right

private:
    CSSBorderImageValue(PassRefPtr<CSSValue> image, PassRefPtr<Rect> sliceRect, int horizontalRule, int verticalRule);
    virtual bool isBorderImageValue() const { return true; }
};

} // namespace WebCore

#endif // CSSBorderImageValue_h
