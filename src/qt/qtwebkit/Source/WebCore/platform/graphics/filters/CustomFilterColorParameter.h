/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
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

#ifndef CustomFilterColorParameter_h
#define CustomFilterColorParameter_h

#if ENABLE(CSS_SHADERS)
#include "Color.h"
#include "CustomFilterParameter.h"

namespace WebCore {

static inline Color blendFunc(const Color& from, const Color& to, double progress)
{
    return blend(from, to, progress);
}

class CustomFilterColorParameter : public CustomFilterParameter {
public:
    static PassRefPtr<CustomFilterColorParameter> create(const String& name)
    {
        return adoptRef(new CustomFilterColorParameter(name));
    }

    virtual PassRefPtr<CustomFilterParameter> blend(const CustomFilterParameter* fromParameter, double progress, const LayoutSize&)
    {
        if (!fromParameter || !isSameType(*fromParameter))
            return this;
        const CustomFilterColorParameter* fromColor = static_cast<const CustomFilterColorParameter*>(fromParameter);

        RefPtr<CustomFilterColorParameter> result = CustomFilterColorParameter::create(name());
        result->setColor(blendFunc(fromColor->color(), color(), progress));
        return result;
    }

    const Color& color() const { return m_color; }
    void setColor(const Color& color) { m_color = color; }

    virtual bool operator==(const CustomFilterParameter& o) const
    {
        if (!isSameType(o))
            return false;
        const CustomFilterColorParameter* other = static_cast<const CustomFilterColorParameter*>(&o);
        return m_color == other->m_color;
    }
    
private:
    CustomFilterColorParameter(const String& name)
        : CustomFilterParameter(COLOR, name)
    {
    }
    virtual ~CustomFilterColorParameter()
    {
    }
    
    Color m_color;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif // CustomFilterColorParameter_h
