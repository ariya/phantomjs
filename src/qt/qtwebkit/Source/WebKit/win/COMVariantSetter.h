/*
 * Copyright (C) 2007, 2008 Apple Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef COMVariantSetter_h
#define COMVariantSetter_h

#include <WebCore/BString.h>
#include <WebCore/COMPtr.h>
#include <wtf/Assertions.h>
#include <wtf/Forward.h>

template<typename T> struct COMVariantSetter {};

template<typename T> struct COMVariantSetterBase
{
    static inline VARENUM variantType(const T&)
    {
        return COMVariantSetter<T>::VariantType;
    }
};

template<> struct COMVariantSetter<WTF::String> : COMVariantSetterBase<WTF::String>
{
    static const VARENUM VariantType = VT_BSTR;

    static void setVariant(VARIANT* variant, const WTF::String& value)
    {
        ASSERT(V_VT(variant) == VT_EMPTY);

        V_VT(variant) = VariantType;
        V_BSTR(variant) = WebCore::BString(value).release();
    }
};

template<> struct COMVariantSetter<bool> : COMVariantSetterBase<bool>
{
    static const VARENUM VariantType = VT_BOOL;

    static void setVariant(VARIANT* variant, bool value)
    {
        ASSERT(V_VT(variant) == VT_EMPTY);

        V_VT(variant) = VariantType;
        V_BOOL(variant) = value;
    }
};

template<> struct COMVariantSetter<unsigned long long> : COMVariantSetterBase<unsigned long long>
{
    static const VARENUM VariantType = VT_UI8;

    static void setVariant(VARIANT* variant, unsigned long long value)
    {
        ASSERT(V_VT(variant) == VT_EMPTY);

        V_VT(variant) = VariantType;
        V_UI8(variant) = value;
    }
};

template<> struct COMVariantSetter<int> : COMVariantSetterBase<int>
{
    static const VARENUM VariantType = VT_I4;

    static void setVariant(VARIANT* variant, int value)
    {
        ASSERT(V_VT(variant) == VT_EMPTY);

        V_VT(variant) = VariantType;
        V_I4(variant) = value;
    }
};

template<> struct COMVariantSetter<float> : COMVariantSetterBase<float>
{
    static const VARENUM VariantType = VT_R4;

    static void setVariant(VARIANT* variant, float value)
    {
        ASSERT(V_VT(variant) == VT_EMPTY);

        V_VT(variant) = VariantType;
        V_R4(variant) = value;
    }
};

template<typename T> struct COMVariantSetter<COMPtr<T> > : COMVariantSetterBase<COMPtr<T> >
{
    static const VARENUM VariantType = VT_UNKNOWN;

    static void setVariant(VARIANT* variant, const COMPtr<T>& value)
    {
        ASSERT(V_VT(variant) == VT_EMPTY);

        V_VT(variant) = VariantType;
        V_UNKNOWN(variant) = value.get();
        value->AddRef();
    }
};

template<typename COMType, typename UnderlyingType>
struct COMIUnknownVariantSetter : COMVariantSetterBase<UnderlyingType>
{
    static const VARENUM VariantType = VT_UNKNOWN;

    static void setVariant(VARIANT* variant, const UnderlyingType& value)
    {
        ASSERT(V_VT(variant) == VT_EMPTY);

        V_VT(variant) = VariantType;
        V_UNKNOWN(variant) = COMType::createInstance(value);
    }
};

class COMVariant {
public:
    COMVariant()
    {
        ::VariantInit(&m_variant);
    }

    template<typename UnderlyingType>
    COMVariant(UnderlyingType value)
    {
        ::VariantInit(&m_variant);
        COMVariantSetter<UnderlyingType>::setVariant(&m_variant, value);
    }

    ~COMVariant()
    {
        ::VariantClear(&m_variant);
    }

    COMVariant(const COMVariant& other)
    {
        ::VariantInit(&m_variant);
        other.copyTo(&m_variant);
    }

    COMVariant& operator=(const COMVariant& other)
    {
        other.copyTo(&m_variant);
        return *this;
    }

    void copyTo(VARIANT* dest) const
    {
        ::VariantCopy(dest, const_cast<VARIANT*>(&m_variant));
    }

    VARENUM variantType() const { return static_cast<VARENUM>(V_VT(&m_variant)); }

private:
    VARIANT m_variant;
};

template<> struct COMVariantSetter<COMVariant>
{
    static inline VARENUM variantType(const COMVariant& value)
    {
        return value.variantType();
    }

    static void setVariant(VARIANT* variant, const COMVariant& value)
    {
        ASSERT(V_VT(variant) == VT_EMPTY);

        value.copyTo(variant);
    }
};

#endif // COMVariantSetter
