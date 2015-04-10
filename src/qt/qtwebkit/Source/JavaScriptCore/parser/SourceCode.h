/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef SourceCode_h
#define SourceCode_h

#include "SourceProvider.h"
#include <wtf/RefPtr.h>

namespace JSC {

    class SourceCode {
    public:
        SourceCode()
            : m_provider(0)
            , m_startChar(0)
            , m_endChar(0)
            , m_firstLine(0)
            , m_startColumn(0)
        {
        }

        SourceCode(WTF::HashTableDeletedValueType)
            : m_provider(WTF::HashTableDeletedValue)
        {
        }

        SourceCode(PassRefPtr<SourceProvider> provider)
            : m_provider(provider)
            , m_startChar(0)
            , m_endChar(m_provider->source().length())
            , m_firstLine(1)
            , m_startColumn(1)
        {
        }

        SourceCode(PassRefPtr<SourceProvider> provider, int firstLine, int startColumn)
            : m_provider(provider)
            , m_startChar(0)
            , m_endChar(m_provider->source().length())
            , m_firstLine(std::max(firstLine, 1))
            , m_startColumn(std::max(startColumn, 1))
        {
        }

        SourceCode(PassRefPtr<SourceProvider> provider, int start, int end, int firstLine, int startColumn)
            : m_provider(provider)
            , m_startChar(start)
            , m_endChar(end)
            , m_firstLine(std::max(firstLine, 1))
            , m_startColumn(std::max(startColumn, 1))
        {
        }

        bool isHashTableDeletedValue() const { return m_provider.isHashTableDeletedValue(); }

        String toString() const
        {
            if (!m_provider)
                return String();
            return m_provider->getRange(m_startChar, m_endChar);
        }
        
        intptr_t providerID() const
        {
            if (!m_provider)
                return SourceProvider::nullID;
            return m_provider->asID();
        }
        
        bool isNull() const { return !m_provider; }
        SourceProvider* provider() const { return m_provider.get(); }
        int firstLine() const { return m_firstLine; }
        int startColumn() const { return m_startColumn; }
        int startOffset() const { return m_startChar; }
        int endOffset() const { return m_endChar; }
        int length() const { return m_endChar - m_startChar; }
        
        SourceCode subExpression(unsigned openBrace, unsigned closeBrace, int firstLine, int startColumn);

    private:
        RefPtr<SourceProvider> m_provider;
        int m_startChar;
        int m_endChar;
        int m_firstLine;
        int m_startColumn;
    };

    inline SourceCode makeSource(const String& source, const String& url = String(), const TextPosition& startPosition = TextPosition::minimumPosition())
    {
        return SourceCode(StringSourceProvider::create(source, url, startPosition), startPosition.m_line.oneBasedInt(), startPosition.m_column.oneBasedInt());
    }

    inline SourceCode SourceCode::subExpression(unsigned openBrace, unsigned closeBrace, int firstLine, int startColumn)
    {
        ASSERT(provider()->source()[openBrace] == '{');
        ASSERT(provider()->source()[closeBrace] == '}');
        startColumn += 1; // Convert to base 1.
        return SourceCode(provider(), openBrace, closeBrace + 1, firstLine, startColumn);
    }

} // namespace JSC

#endif // SourceCode_h
