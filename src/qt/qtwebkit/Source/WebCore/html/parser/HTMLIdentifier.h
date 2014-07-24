/*
 * Copyright (C) 2013 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLIdentifier_h
#define HTMLIdentifier_h

#if ENABLE(THREADED_HTML_PARSER)

#include <wtf/text/WTFString.h>

namespace WebCore {

class QualifiedName;

enum CharacterWidth {
    Likely8Bit,
    Force8Bit,
    Force16Bit
};

class HTMLIdentifier {
public:
    HTMLIdentifier() : m_index(0) { }

    template<size_t inlineCapacity>
    HTMLIdentifier(const Vector<UChar, inlineCapacity>& vector, CharacterWidth width)
        : m_index(findIndex(vector.data(), vector.size()))
    {
        if (m_index != invalidIndex)
            return;
        if (width == Likely8Bit)
            m_string = StringImpl::create8BitIfPossible(vector);
        else if (width == Force8Bit)
            m_string = String::make8BitFrom16BitSource(vector);
        else
            m_string = String(vector);
    }

    // asString should only be used on the main thread.
    const String& asString() const;
    // asStringImpl() is safe to call from any thread.
    const StringImpl* asStringImpl() const;

    static void init();

    bool isSafeToSendToAnotherThread() const { return m_string.isSafeToSendToAnotherThread(); }

#ifndef NDEBUG
    static bool hasIndex(const StringImpl*);
#endif

private:
    static const unsigned invalidIndex = -1;
    static unsigned maxNameLength;
    static unsigned findIndex(const UChar* characters, unsigned length);
    static void addNames(QualifiedName** names, unsigned namesCount, unsigned indexOffset);

    // FIXME: This could be a union.
    unsigned m_index;
    String m_string;
};

}

#endif // ENABLE(THREADED_HTML_PARSER)

#endif
