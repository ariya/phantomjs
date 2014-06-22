/*
 * Copyright (C) 2012 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef StyleVariableData_h
#define StyleVariableData_h
#if ENABLE(CSS_VARIABLES)

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class CursorList;
class QuotesData;
class ShadowData;

class StyleVariableData : public RefCounted<StyleVariableData> {
public:
    static PassRefPtr<StyleVariableData> create() { return adoptRef(new StyleVariableData()); }
    PassRefPtr<StyleVariableData> copy() const { return adoptRef(new StyleVariableData(*this)); }

    bool operator==(const StyleVariableData& other) const { return other.m_data == m_data; }
    bool operator!=(const StyleVariableData& other) const { return !(*this == other); }

    void setVariable(const AtomicString& name, const String& value) { m_data.set(name, value); }

    HashMap<AtomicString, String> m_data;
private:
    explicit StyleVariableData() : RefCounted<StyleVariableData>() { }
    StyleVariableData(const StyleVariableData& other) : RefCounted<StyleVariableData>(), m_data(HashMap<AtomicString, String>(other.m_data)) { }
};

} // namespace WebCore

#endif /* ENABLE(CSS_VARIABLES) */
#endif /* StyleVariableData_h */
