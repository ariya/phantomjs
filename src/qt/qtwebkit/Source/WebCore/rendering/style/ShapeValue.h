/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ShapeValue_h
#define ShapeValue_h

#include "BasicShapes.h"
#include "StyleImage.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

class ShapeValue : public RefCounted<ShapeValue> {
public:
    enum ShapeValueType {
        // The Auto value is defined by a null ShapeValue*
        Shape,
        Outside,
        Image
    };

    static PassRefPtr<ShapeValue> createShapeValue(PassRefPtr<BasicShape> shape)
    {
        return adoptRef(new ShapeValue(shape));
    }

    static PassRefPtr<ShapeValue> createOutsideValue()
    {
        return adoptRef(new ShapeValue(Outside));
    }

    static PassRefPtr<ShapeValue> createImageValue(PassRefPtr<StyleImage> image)
    {
        return adoptRef(new ShapeValue(image));
    }

    ShapeValueType type() const { return m_type; }
    BasicShape* shape() const { return m_shape.get(); }
    StyleImage* image() const { return m_image.get(); }
    void setImage(PassRefPtr<StyleImage> image)
    {
        if (m_image != image)
            m_image = image;
    }
    bool operator==(const ShapeValue& other) const { return type() == other.type(); }

private:
    ShapeValue(PassRefPtr<BasicShape> shape)
        : m_type(Shape)
        , m_shape(shape)
    {
    }
    ShapeValue(ShapeValueType type)
        : m_type(type)
    {
    }
    ShapeValue(PassRefPtr<StyleImage> image)
        : m_type(Image)
        , m_image(image)
    {
    }
    ShapeValueType m_type;
    RefPtr<BasicShape> m_shape;
    RefPtr<StyleImage> m_image;
};

}

#endif
