/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSVariableValue_h
#define CSSVariableValue_h

#if ENABLE(CSS_VARIABLES)

#include "CSSParserValues.h"
#include "CSSPropertyNames.h"
#include "CSSValue.h"

namespace WebCore {

class CSSVariableValue : public CSSValue {
public:
    static PassRefPtr<CSSVariableValue> create(const AtomicString& name, const String& value)
    {
        return adoptRef(new CSSVariableValue(name, value));
    }

    const AtomicString& name() const { return m_name; }
    const String& value() const { return m_value; }

    bool equals(const CSSVariableValue& other) const { return m_name == other.m_name && m_value == other.m_value; }

private:
    CSSVariableValue(const AtomicString& name, const String& value)
        : CSSValue(VariableClass)
        , m_name(name)
        , m_value(value)
    {
    }

    const AtomicString m_name;
    const String m_value;
};

}

#endif /* ENABLE(CSS_VARIABLES) */
#endif /* CSSVariableValue_h */
