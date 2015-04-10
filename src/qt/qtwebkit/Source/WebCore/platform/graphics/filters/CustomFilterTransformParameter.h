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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
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

#ifndef CustomFilterTransformParameter_h
#define CustomFilterTransformParameter_h

#if ENABLE(CSS_SHADERS)
#include "CustomFilterParameter.h"
#include "FloatSize.h"
#include "TransformOperations.h"

namespace WebCore {

class FloatRect;
class TransformationMatrix;

class CustomFilterTransformParameter : public CustomFilterParameter {
public:
    static PassRefPtr<CustomFilterTransformParameter> create(const String& name)
    {
        return adoptRef(new CustomFilterTransformParameter(name));
    }

    virtual PassRefPtr<CustomFilterParameter> blend(const CustomFilterParameter* fromParameter, double progress, const LayoutSize& size)
    {
        if (!fromParameter || !isSameType(*fromParameter))
            return this;

        const CustomFilterTransformParameter* fromTransformParameter = static_cast<const CustomFilterTransformParameter*>(fromParameter);
        const TransformOperations& from = fromTransformParameter->operations();
        const TransformOperations& to = operations();
        if (from == to)
            return this;
       
        RefPtr<CustomFilterTransformParameter> result = CustomFilterTransformParameter::create(name());
        if (from.size() && to.size())
            result->setOperations(to.blend(from, progress, size));
        else
            result->setOperations(progress > 0.5 ? to : from);
        return result;
    }

    virtual bool operator==(const CustomFilterParameter& o) const
    {
        if (!isSameType(o))
            return false;
        const CustomFilterTransformParameter* other = static_cast<const CustomFilterTransformParameter*>(&o);
        return m_operations == other->m_operations;
    }

    void applyTransform(TransformationMatrix& transform, const FloatSize& boxSize) const
    {
        for (unsigned i = 0, size = m_operations.size(); i < size; ++i)
            m_operations.at(i)->apply(transform, boxSize);
    }

    const TransformOperations& operations() const { return m_operations; }
    void setOperations(const TransformOperations& value) { m_operations = value; }
    
private:
    CustomFilterTransformParameter(const String& name)
        : CustomFilterParameter(TRANSFORM, name)
    {
    }
    virtual ~CustomFilterTransformParameter()
    {
    }
    
    TransformOperations m_operations;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif // CustomFilterTransformParameter_h
