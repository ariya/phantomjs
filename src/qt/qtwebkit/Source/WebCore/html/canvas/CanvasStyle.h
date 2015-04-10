/*
 * Copyright (C) 2006, 2008, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef CanvasStyle_h
#define CanvasStyle_h

#include "Color.h"
#include <wtf/Assertions.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

    class CanvasGradient;
    class CanvasPattern;
    class Document;
    class GraphicsContext;
    class HTMLCanvasElement;

    class CanvasStyle {
    public:
        CanvasStyle();
        explicit CanvasStyle(RGBA32);
        CanvasStyle(float grayLevel, float alpha);
        CanvasStyle(float r, float g, float b, float alpha);
        CanvasStyle(float c, float m, float y, float k, float alpha);
        explicit CanvasStyle(PassRefPtr<CanvasGradient>);
        explicit CanvasStyle(PassRefPtr<CanvasPattern>);
        ~CanvasStyle();

        static CanvasStyle createFromString(const String& color, Document* = 0);
        static CanvasStyle createFromStringWithOverrideAlpha(const String& color, float alpha);

        bool isValid() const { return m_type != Invalid; }
        bool isCurrentColor() const { return m_type == CurrentColor || m_type == CurrentColorWithOverrideAlpha; }
        bool hasOverrideAlpha() const { return m_type == CurrentColorWithOverrideAlpha; }
        float overrideAlpha() const { ASSERT(m_type == CurrentColorWithOverrideAlpha); return m_overrideAlpha; }

        String color() const;
        CanvasGradient* canvasGradient() const;
        CanvasPattern* canvasPattern() const;

        void applyFillColor(GraphicsContext*) const;
        void applyStrokeColor(GraphicsContext*) const;

        bool isEquivalentColor(const CanvasStyle&) const;
        bool isEquivalentRGBA(float r, float g, float b, float a) const;
        bool isEquivalentCMYKA(float c, float m, float y, float k, float a) const;

        CanvasStyle(const CanvasStyle&);
        CanvasStyle& operator=(const CanvasStyle&);
#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
        CanvasStyle(CanvasStyle&&);
        CanvasStyle& operator=(CanvasStyle&&);
#endif

    private:
        enum Type { RGBA, CMYKA, Gradient, ImagePattern, CurrentColor, CurrentColorWithOverrideAlpha, Invalid };
        struct CMYKAValues {
            WTF_MAKE_FAST_ALLOCATED;
            WTF_MAKE_NONCOPYABLE(CMYKAValues);
        public:
            CMYKAValues() : rgba(0), c(0), m(0), y(0), k(0), a(0) { }
            CMYKAValues(RGBA32 rgba, float cyan, float magenta, float yellow, float black, float alpha) : rgba(rgba), c(cyan), m(magenta), y(yellow), k(black), a(alpha) { }
            RGBA32 rgba;
            float c;
            float m;
            float y;
            float k;
            float a;
        };

        enum ConstructCurrentColorTag { ConstructCurrentColor };
        CanvasStyle(ConstructCurrentColorTag) : m_type(CurrentColor) { }
        CanvasStyle(Type type, float overrideAlpha)
            : m_overrideAlpha(overrideAlpha)
            , m_type(type)
        {
        }

        union {
            RGBA32 m_rgba;
            float m_overrideAlpha;
            CanvasGradient* m_gradient;
            CanvasPattern* m_pattern;
            CMYKAValues* m_cmyka;
        };
        Type m_type;
    };

    RGBA32 currentColor(HTMLCanvasElement*);
    bool parseColorOrCurrentColor(RGBA32& parsedColor, const String& colorString, HTMLCanvasElement*);

    inline CanvasStyle::CanvasStyle()
        : m_type(Invalid)
    {
    }

    inline CanvasGradient* CanvasStyle::canvasGradient() const
    {
        if (m_type == Gradient)
            return m_gradient;
        return 0;
    }

    inline CanvasPattern* CanvasStyle::canvasPattern() const
    {
        if (m_type == ImagePattern)
            return m_pattern;
        return 0;
    }

    inline String CanvasStyle::color() const
    {
        ASSERT(m_type == RGBA || m_type == CMYKA);
        if (m_type == RGBA)
            return Color(m_rgba).serialized();
        return Color(m_cmyka->rgba).serialized();
    }

#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
    inline CanvasStyle::CanvasStyle(CanvasStyle&& other)
    {
        memcpy(this, &other, sizeof(CanvasStyle));
        other.m_type = Invalid;
    }

    inline CanvasStyle& CanvasStyle::operator=(CanvasStyle&& other)
    {
        if (this != &other) {
            memcpy(this, &other, sizeof(CanvasStyle));
            other.m_type = Invalid;
        }
        return *this;
    }
#endif

} // namespace WebCore

#endif
