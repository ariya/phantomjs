/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

#ifndef CSSGradientValue_h
#define CSSGradientValue_h

#include "CSSImageGeneratorValue.h"
#include "CSSPrimitiveValue.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class FloatPoint;
class Gradient;

enum CSSGradientType { CSSLinearGradient, CSSRadialGradient };
enum CSSGradientRepeat { NonRepeating, Repeating };

struct CSSGradientColorStop {
    RefPtr<CSSPrimitiveValue> m_position; // percentage or length
    RefPtr<CSSPrimitiveValue> m_color;
};

class CSSGradientValue : public CSSImageGeneratorValue {
public:
    virtual PassRefPtr<Image> image(RenderObject*, const IntSize&);

    void setFirstX(PassRefPtr<CSSPrimitiveValue> val) { m_firstX = val; }
    void setFirstY(PassRefPtr<CSSPrimitiveValue> val) { m_firstY = val; }
    void setSecondX(PassRefPtr<CSSPrimitiveValue> val) { m_secondX = val; }
    void setSecondY(PassRefPtr<CSSPrimitiveValue> val) { m_secondY = val; }
    
    void addStop(const CSSGradientColorStop& stop) { m_stops.append(stop); }

    Vector<CSSGradientColorStop>& stops() { return m_stops; }

    void sortStopsIfNeeded();
    
    virtual bool isLinearGradient() const { return false; }
    virtual bool isRadialGradient() const { return false; }
    
    bool isRepeating() const { return m_repeating; }

    bool deprecatedType() const { return m_deprecatedType; } // came from -webkit-gradient

protected:
    CSSGradientValue(CSSGradientRepeat repeat, bool deprecatedType = false)
        : m_stopsSorted(false)
        , m_deprecatedType(deprecatedType)
        , m_repeating(repeat == Repeating)
    {
    }
    
    void addStops(Gradient*, RenderObject*, RenderStyle* rootStyle, float maxLengthForRepeat = 0);

    // Create the gradient for a given size.
    virtual PassRefPtr<Gradient> createGradient(RenderObject*, const IntSize&) = 0;

    // Resolve points/radii to front end values.
    FloatPoint computeEndPoint(CSSPrimitiveValue*, CSSPrimitiveValue*, RenderStyle*, RenderStyle* rootStyle, const IntSize&);
    
    bool isCacheable() const;

    // Points. Some of these may be null for linear gradients.
    RefPtr<CSSPrimitiveValue> m_firstX;
    RefPtr<CSSPrimitiveValue> m_firstY;
    
    RefPtr<CSSPrimitiveValue> m_secondX;
    RefPtr<CSSPrimitiveValue> m_secondY;
    
    // Stops
    Vector<CSSGradientColorStop> m_stops;
    bool m_stopsSorted;
    bool m_deprecatedType; // -webkit-gradient()
    bool m_repeating;
};


class CSSLinearGradientValue : public CSSGradientValue {
public:
    static PassRefPtr<CSSLinearGradientValue> create(CSSGradientRepeat repeat, bool deprecatedType = false)
    {
        return adoptRef(new CSSLinearGradientValue(repeat, deprecatedType));
    }

    void setAngle(PassRefPtr<CSSPrimitiveValue> val) { m_angle = val; }

    virtual String cssText() const;

private:
    CSSLinearGradientValue(CSSGradientRepeat repeat, bool deprecatedType = false)
        : CSSGradientValue(repeat, deprecatedType)
    {
    }

    virtual bool isLinearGradient() const { return true; }

    // Create the gradient for a given size.
    virtual PassRefPtr<Gradient> createGradient(RenderObject*, const IntSize&);

    RefPtr<CSSPrimitiveValue> m_angle; // may be null.
};

class CSSRadialGradientValue : public CSSGradientValue {
public:
    static PassRefPtr<CSSRadialGradientValue> create(CSSGradientRepeat repeat, bool deprecatedType = false)
    {
        return adoptRef(new CSSRadialGradientValue(repeat, deprecatedType));
    }

    virtual String cssText() const;

    void setFirstRadius(PassRefPtr<CSSPrimitiveValue> val) { m_firstRadius = val; }
    void setSecondRadius(PassRefPtr<CSSPrimitiveValue> val) { m_secondRadius = val; }

    void setShape(PassRefPtr<CSSPrimitiveValue> val) { m_shape = val; }
    void setSizingBehavior(PassRefPtr<CSSPrimitiveValue> val) { m_sizingBehavior = val; }

    void setEndHorizontalSize(PassRefPtr<CSSPrimitiveValue> val) { m_endHorizontalSize = val; }
    void setEndVerticalSize(PassRefPtr<CSSPrimitiveValue> val) { m_endVerticalSize = val; }

private:
    CSSRadialGradientValue(CSSGradientRepeat repeat, bool deprecatedType = false)
        : CSSGradientValue(repeat, deprecatedType)
    {
    }

    virtual bool isRadialGradient() const { return true; }

    // Create the gradient for a given size.
    virtual PassRefPtr<Gradient> createGradient(RenderObject*, const IntSize&);
    
    // Resolve points/radii to front end values.
    float resolveRadius(CSSPrimitiveValue*, RenderStyle*, RenderStyle* rootStyle, float* widthOrHeight = 0);
    
    // These may be null for non-deprecated gradients.
    RefPtr<CSSPrimitiveValue> m_firstRadius;
    RefPtr<CSSPrimitiveValue> m_secondRadius;

    // The below are only used for non-deprecated gradients.
    RefPtr<CSSPrimitiveValue> m_shape;
    RefPtr<CSSPrimitiveValue> m_sizingBehavior;

    RefPtr<CSSPrimitiveValue> m_endHorizontalSize;
    RefPtr<CSSPrimitiveValue> m_endVerticalSize;
};

} // namespace WebCore

#endif // CSSGradientValue_h
