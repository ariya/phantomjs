/*
    Copyright (C) 1999 Lars Knoll (knoll@kde.org)
    Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
    Copyright (C) 2011 Rik Cabanier (cabanier@adobe.com)
    Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef Length_h
#define Length_h

#include "AnimationUtilities.h"
#include <string.h>
#include <wtf/Assertions.h>
#include <wtf/FastAllocBase.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/MathExtras.h>
#include <wtf/PassOwnArrayPtr.h>

namespace WebCore {

enum LengthType {
    Auto, Relative, Percent, Fixed,
    Intrinsic, MinIntrinsic,
    MinContent, MaxContent, FillAvailable, FitContent,
    Calculated,
    ViewportPercentageWidth, ViewportPercentageHeight, ViewportPercentageMin, ViewportPercentageMax,
    Undefined
};

class CalculationValue;    
    
struct Length {
    WTF_MAKE_FAST_ALLOCATED;
public:
    Length()
        :  m_intValue(0), m_quirk(false), m_type(Auto), m_isFloat(false)
    {
    }

    Length(LengthType t)
        : m_intValue(0), m_quirk(false), m_type(t), m_isFloat(false)
    {
        ASSERT(t != Calculated);
    }

    Length(int v, LengthType t, bool q = false)
        : m_intValue(v), m_quirk(q), m_type(t), m_isFloat(false)
    {
        ASSERT(t != Calculated);
    }
    
    Length(LayoutUnit v, LengthType t, bool q = false)
        : m_floatValue(v.toFloat()), m_quirk(q), m_type(t), m_isFloat(true)
    {
        ASSERT(t != Calculated);
    }
    
    Length(float v, LengthType t, bool q = false)
        : m_floatValue(v), m_quirk(q), m_type(t), m_isFloat(true)
    {
        ASSERT(t != Calculated);
    }

    Length(double v, LengthType t, bool q = false)
        : m_quirk(q), m_type(t), m_isFloat(true)
    {
        m_floatValue = static_cast<float>(v);    
    }

    explicit Length(PassRefPtr<CalculationValue>);

    Length(const Length& length)
    {
        initFromLength(length);
    }
    
    Length& operator=(const Length& length)
    {
        initFromLength(length);
        return *this;
    }
    
    ~Length()
    {
        if (isCalculated())
            decrementCalculatedRef();
    }  
    
    bool operator==(const Length& o) const { return (m_type == o.m_type) && (m_quirk == o.m_quirk) && (isUndefined() || (getFloatValue() == o.getFloatValue()) || isCalculatedEqual(o)); }
    bool operator!=(const Length& o) const { return !(*this == o); }

    const Length& operator*=(float v)
    {       
        if (isCalculated()) {
            ASSERT_NOT_REACHED();
            return *this;
        }
        
        if (m_isFloat)
            m_floatValue = static_cast<float>(m_floatValue * v);
        else        
            m_intValue = static_cast<int>(m_intValue * v);
        
        return *this;
    }
    
    inline float value() const
    {
        return getFloatValue();
    }

     int intValue() const
     {
        if (isCalculated()) {
            ASSERT_NOT_REACHED();
            return 0;
        }
        return getIntValue();
    }

    float percent() const
    {
        ASSERT(type() == Percent);
        return getFloatValue();
    }

    PassRefPtr<CalculationValue> calculationValue() const;

    LengthType type() const { return static_cast<LengthType>(m_type); }
    bool quirk() const { return m_quirk; }

    void setQuirk(bool quirk)
    {
        m_quirk = quirk;
    }

    void setValue(LengthType t, int value)
    {
        m_type = t;
        m_intValue = value;
        m_isFloat = false;
    }

    void setValue(int value)
    {
        if (isCalculated()) {
            ASSERT_NOT_REACHED();
            return;
        }
        setValue(Fixed, value);
    }

    void setValue(LengthType t, float value)
    {
        m_type = t;
        m_floatValue = value;
        m_isFloat = true;    
    }

    void setValue(LengthType t, LayoutUnit value)
    {
        m_type = t;
        m_floatValue = value;
        m_isFloat = true;    
    }

    void setValue(float value)
    {
        *this = Length(value, Fixed);
    }

    bool isUndefined() const { return type() == Undefined; }

    // FIXME calc: https://bugs.webkit.org/show_bug.cgi?id=80357. A calculated Length 
    // always contains a percentage, and without a maxValue passed to these functions
    // it's impossible to determine the sign or zero-ness. We assume all calc values
    // are positive and non-zero for now.    
    bool isZero() const 
    {
        ASSERT(!isUndefined());
        if (isCalculated())
            return false;
            
        return m_isFloat ? !m_floatValue : !m_intValue;
    }
    bool isPositive() const
    {
        if (isUndefined())
            return false;
        if (isCalculated())
            return true;
                
        return getFloatValue() > 0;
    }
    bool isNegative() const
    {
        if (isUndefined() || isCalculated())
            return false;
            
        return getFloatValue() < 0;
    }
    
    bool isAuto() const { return type() == Auto; }
    bool isRelative() const { return type() == Relative; }
    bool isPercent() const { return type() == Percent || type() == Calculated; }
    bool isFixed() const { return type() == Fixed; }
    bool isIntrinsicOrAuto() const { return type() == Auto || isLegacyIntrinsic() || isIntrinsic(); }
    bool isLegacyIntrinsic() const { return type() == Intrinsic || type() == MinIntrinsic; }
    bool isIntrinsic() const { return type() == MinContent || type() == MaxContent || type() == FillAvailable || type() == FitContent; }
    bool isSpecified() const { return type() == Fixed || type() == Percent || type() == Calculated || isViewportPercentage(); }
    bool isSpecifiedOrIntrinsic() const { return isSpecified() || isIntrinsic(); }
    bool isCalculated() const { return type() == Calculated; }
    bool isCalculatedEqual(const Length&) const;
    bool isMinContent() const { return type() == MinContent; }
    bool isMaxContent() const { return type() == MaxContent; }

    Length blend(const Length& from, double progress) const
    {
        // Blend two lengths to produce a new length that is in between them.  Used for animation.
        if (from.type() == Calculated || type() == Calculated)
            return blendMixedTypes(from, progress);
        
        if (!from.isZero() && !isZero() && from.type() != type())
            return blendMixedTypes(from, progress);

        if (from.isZero() && isZero())
            return *this;
        
        LengthType resultType = type();
        if (isZero())
            resultType = from.type();
        
        if (resultType == Percent) {
            float fromPercent = from.isZero() ? 0 : from.percent();
            float toPercent = isZero() ? 0 : percent();
            return Length(WebCore::blend(fromPercent, toPercent, progress), Percent);
        } 

        float fromValue = from.isZero() ? 0 : from.value();
        float toValue = isZero() ? 0 : value();
        return Length(WebCore::blend(fromValue, toValue, progress), resultType);
    }

    float getFloatValue() const
    {
        ASSERT(!isUndefined());
        return m_isFloat ? m_floatValue : m_intValue;
    }
    float nonNanCalculatedValue(int maxValue) const;

    bool isViewportPercentage() const
    {
        LengthType lengthType = type();
        return lengthType >= ViewportPercentageWidth && lengthType <= ViewportPercentageMax;
    }
    float viewportPercentageLength() const
    {
        ASSERT(isViewportPercentage());
        return getFloatValue();
    }
private:
    int getIntValue() const
    {
        ASSERT(!isUndefined());
        return m_isFloat ? static_cast<int>(m_floatValue) : m_intValue;
    }
    void initFromLength(const Length& length)
    {
        memcpy(this, &length, sizeof(Length));
        if (isCalculated())
            incrementCalculatedRef();
    }

    Length blendMixedTypes(const Length& from, double progress) const;

    int calculationHandle() const
    {
        ASSERT(isCalculated());
        return getIntValue();
    }
    void incrementCalculatedRef() const;
    void decrementCalculatedRef() const;    
    
    union {
        int m_intValue;
        float m_floatValue;
    };
    bool m_quirk;
    unsigned char m_type;
    bool m_isFloat;
};

PassOwnArrayPtr<Length> newCoordsArray(const String&, int& len);
PassOwnArrayPtr<Length> newLengthArray(const String&, int& len);

} // namespace WebCore

#endif // Length_h
