/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorStyleTextEditor_h
#define InspectorStyleTextEditor_h

#include "CSSPropertySourceData.h"

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

#if ENABLE(INSPECTOR)

struct InspectorStyleProperty;
struct SourceRange;

typedef std::pair<String, String> NewLineAndWhitespace;

class InspectorStyleTextEditor {
public:
    InspectorStyleTextEditor(Vector<InspectorStyleProperty>* allProperties, Vector<InspectorStyleProperty>* disabledProperties, const String& styleText, const NewLineAndWhitespace& format);
    void insertProperty(unsigned index, const String& propertyText, unsigned styleBodyLength);
    void replaceProperty(unsigned index, const String& newText);
    void removeProperty(unsigned index);
    void enableProperty(unsigned index);
    void disableProperty(unsigned index);
    const String& styleText() const { return m_styleText; }

private:
    unsigned disabledIndexByOrdinal(unsigned ordinal, bool canUseSubsequent);
    void shiftDisabledProperties(unsigned fromIndex, long delta);
    void internalReplaceProperty(const InspectorStyleProperty&, const String& newText, SourceRange* removedRange, unsigned* insertedLength);

    Vector<InspectorStyleProperty>* m_allProperties;
    Vector<InspectorStyleProperty>* m_disabledProperties;
    String m_styleText;
    const std::pair<String, String> m_format;
};

#endif

} // namespace WebCore

#endif // !defined(InspectorStyleTextEditor_h)
