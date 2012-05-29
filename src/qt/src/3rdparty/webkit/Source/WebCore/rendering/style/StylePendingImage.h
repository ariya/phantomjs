/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef StylePendingImage_h
#define StylePendingImage_h

#include "Image.h"
#include "StyleImage.h"

namespace WebCore {

// StylePendingImage is a placeholder StyleImage that is entered into the RenderStyle during
// style resolution, in order to avoid loading images that are not referenced by the final style.
// They should never exist in a RenderStyle after it has been returned from the style selector.

class StylePendingImage : public StyleImage {
public:
    static PassRefPtr<StylePendingImage> create(CSSImageValue* value) { return adoptRef(new StylePendingImage(value)); }

    virtual WrappedImagePtr data() const { return m_value; }

    virtual bool isPendingImage() const { return true; }
    
    virtual PassRefPtr<CSSValue> cssValue() const { return m_value; }
    CSSImageValue* cssImageValue() const { return m_value; }
    
    virtual IntSize imageSize(const RenderObject*, float /*multiplier*/) const { return IntSize(); }
    virtual bool imageHasRelativeWidth() const { return false; }
    virtual bool imageHasRelativeHeight() const { return false; }
    virtual bool usesImageContainerSize() const { return false; }
    virtual void setImageContainerSize(const IntSize&) { }
    virtual void addClient(RenderObject*) { }
    virtual void removeClient(RenderObject*) { }
    virtual PassRefPtr<Image> image(RenderObject*, const IntSize&) const
    {
        ASSERT_NOT_REACHED();
        return 0;
    }
    
private:
    StylePendingImage(CSSImageValue* value)
        : m_value(value)
    {
    }

    CSSImageValue* m_value; // Not retained; it owns us.
};

}
#endif
