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

#ifndef StyleGeneratedImage_h
#define StyleGeneratedImage_h

#include "StyleImage.h"

namespace WebCore {

class CSSValue;
class CSSImageGeneratorValue;

class StyleGeneratedImage : public StyleImage {
public:
    static PassRefPtr<StyleGeneratedImage> create(CSSImageGeneratorValue* val, bool fixedSize)
    {
        return adoptRef(new StyleGeneratedImage(val, fixedSize));
    }

    virtual WrappedImagePtr data() const { return m_generator; }

    virtual bool isGeneratedImage() const { return true; }
    
    virtual PassRefPtr<CSSValue> cssValue() const;

    virtual IntSize imageSize(const RenderObject*, float multiplier) const;
    virtual bool imageHasRelativeWidth() const { return !m_fixedSize; }
    virtual bool imageHasRelativeHeight() const { return !m_fixedSize; }
    virtual bool usesImageContainerSize() const { return !m_fixedSize; }
    virtual void setImageContainerSize(const IntSize&);
    virtual void addClient(RenderObject*);
    virtual void removeClient(RenderObject*);
    virtual PassRefPtr<Image> image(RenderObject*, const IntSize&) const;
    
private:
    StyleGeneratedImage(CSSImageGeneratorValue* val, bool fixedSize)
        : m_generator(val)
        , m_fixedSize(fixedSize)
    {
    }
    
    CSSImageGeneratorValue* m_generator; // The generator holds a reference to us.
    IntSize m_containerSize;
    bool m_fixedSize;
};

}
#endif
