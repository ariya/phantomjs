/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "config.h"
#include "IDBKeyPath.h"

#if ENABLE(INDEXED_DATABASE)

#include <wtf/ASCIICType.h>
#include <wtf/dtoa.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

class IDBKeyPathLexer {
public:
    enum TokenType {
        TokenIdentifier,
        TokenDot,
        TokenEnd,
        TokenError
    };

    explicit IDBKeyPathLexer(const String& s)
        : m_string(s)
        , m_ptr(s.characters())
        , m_end(s.characters() + s.length())
        , m_currentTokenType(TokenError)
    {
    }

    TokenType currentTokenType() const { return m_currentTokenType; }

    TokenType nextTokenType()
    {
        m_currentTokenType = lex(m_currentElement);
        return m_currentTokenType;
    }

    const String& currentElement() { return m_currentElement; }

private:
    TokenType lex(String&);
    TokenType lexIdentifier(String&);
    String m_currentElement;
    String m_string;
    const UChar* m_ptr;
    const UChar* m_end;
    TokenType m_currentTokenType;
};

IDBKeyPathLexer::TokenType IDBKeyPathLexer::lex(String& element)
{
    if (m_ptr >= m_end)
        return TokenEnd;
    ASSERT(m_ptr < m_end);

    if (*m_ptr == '.') {
        ++m_ptr;
        return TokenDot;
    }
    return lexIdentifier(element);
}

namespace {

using namespace WTF::Unicode;

// The following correspond to grammar in ECMA-262.
const uint32_t unicodeLetter = Letter_Uppercase | Letter_Lowercase | Letter_Titlecase | Letter_Modifier | Letter_Other | Number_Letter;
const uint32_t unicodeCombiningMark = Mark_NonSpacing | Mark_SpacingCombining;
const uint32_t unicodeDigit = Number_DecimalDigit;
const uint32_t unicodeConnectorPunctuation = Punctuation_Connector;
const UChar ZWNJ = 0x200C;
const UChar ZWJ = 0x200D;

static inline bool isIdentifierStartCharacter(UChar c)
{
    return (category(c) & unicodeLetter) || (c == '$') || (c == '_');
}

static inline bool isIdentifierCharacter(UChar c)
{
    return (category(c) & (unicodeLetter | unicodeCombiningMark | unicodeDigit | unicodeConnectorPunctuation)) || (c == '$') || (c == '_') || (c == ZWNJ) || (c == ZWJ);
}

} // namespace

IDBKeyPathLexer::TokenType IDBKeyPathLexer::lexIdentifier(String& element)
{
    const UChar* start = m_ptr;
    if (m_ptr < m_end && isIdentifierStartCharacter(*m_ptr))
        ++m_ptr;
    else
        return TokenError;

    while (m_ptr < m_end && isIdentifierCharacter(*m_ptr))
        ++m_ptr;

    element = String(start, m_ptr - start);
    return TokenIdentifier;
}

bool IDBIsValidKeyPath(const String& keyPath)
{
    IDBKeyPathParseError error;
    Vector<String> keyPathElements;
    IDBParseKeyPath(keyPath, keyPathElements, error);
    return error == IDBKeyPathParseErrorNone;
}

void IDBParseKeyPath(const String& keyPath, Vector<String>& elements, IDBKeyPathParseError& error)
{
    // IDBKeyPath ::= EMPTY_STRING | identifier ('.' identifier)*
    // The basic state machine is:
    //   Start => {Identifier, End}
    //   Identifier => {Dot, End}
    //   Dot => {Identifier}
    // It bails out as soon as it finds an error, but doesn't discard the bits it managed to parse.
    enum ParserState { Identifier, Dot, End };

    IDBKeyPathLexer lexer(keyPath);
    IDBKeyPathLexer::TokenType tokenType = lexer.nextTokenType();
    ParserState state;
    if (tokenType == IDBKeyPathLexer::TokenIdentifier)
        state = Identifier;
    else if (tokenType == IDBKeyPathLexer::TokenEnd)
        state = End;
    else {
        error = IDBKeyPathParseErrorStart;
        return;
    }

    while (1) {
        switch (state) {
        case Identifier : {
            IDBKeyPathLexer::TokenType tokenType = lexer.currentTokenType();
            ASSERT(tokenType == IDBKeyPathLexer::TokenIdentifier);

            String element = lexer.currentElement();
            elements.append(element);

            tokenType = lexer.nextTokenType();
            if (tokenType == IDBKeyPathLexer::TokenDot)
                state = Dot;
            else if (tokenType == IDBKeyPathLexer::TokenEnd)
                state = End;
            else {
                error = IDBKeyPathParseErrorIdentifier;
                return;
            }
            break;
        }
        case Dot: {
            IDBKeyPathLexer::TokenType tokenType = lexer.currentTokenType();
            ASSERT(tokenType == IDBKeyPathLexer::TokenDot);

            tokenType = lexer.nextTokenType();
            if (tokenType == IDBKeyPathLexer::TokenIdentifier)
                state = Identifier;
            else {
                error = IDBKeyPathParseErrorDot;
                return;
            }
            break;
        }
        case End: {
            error = IDBKeyPathParseErrorNone;
            return;
        }
        }
    }
}

IDBKeyPath::IDBKeyPath(const String& string)
    : m_type(StringType)
    , m_string(string)
{
    ASSERT(!m_string.isNull());
}

IDBKeyPath::IDBKeyPath(const Vector<String>& array)
    : m_type(ArrayType)
    , m_array(array)
{
#ifndef NDEBUG
    for (size_t i = 0; i < m_array.size(); ++i)
        ASSERT(!m_array[i].isNull());
#endif
}

bool IDBKeyPath::isValid() const
{
    switch (m_type) {
    case NullType:
        return false;

    case StringType:
        return IDBIsValidKeyPath(m_string);

    case ArrayType:
        if (m_array.isEmpty())
            return false;
        for (size_t i = 0; i < m_array.size(); ++i) {
            if (!IDBIsValidKeyPath(m_array[i]))
                return false;
        }
        return true;
    }
    ASSERT_NOT_REACHED();
    return false;
}

bool IDBKeyPath::operator==(const IDBKeyPath& other) const
{
    if (m_type != other.m_type)
        return false;

    switch (m_type) {
    case NullType:
        return true;
    case StringType:
        return m_string == other.m_string;
    case ArrayType:
        return m_array == other.m_array;
    }
    ASSERT_NOT_REACHED();
    return false;
}


} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
