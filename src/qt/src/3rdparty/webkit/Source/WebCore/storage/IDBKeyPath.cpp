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

namespace WebCore {

class IDBKeyPathLexer {
public:
    enum TokenType {
        TokenLeftBracket,
        TokenRightBracket,
        TokenIdentifier,
        TokenNumber,
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

    const IDBKeyPathElement& currentElement() { return m_currentElement; }

private:
    TokenType lex(IDBKeyPathElement&);
    TokenType lexIdentifier(IDBKeyPathElement&);
    TokenType lexNumber(IDBKeyPathElement&);
    IDBKeyPathElement m_currentElement;
    String m_string;
    const UChar* m_ptr;
    const UChar* m_end;
    TokenType m_currentTokenType;
};

IDBKeyPathLexer::TokenType IDBKeyPathLexer::lex(IDBKeyPathElement& element)
{
    while (m_ptr < m_end && isASCIISpace(*m_ptr))
        ++m_ptr;

    if (m_ptr >= m_end)
        return TokenEnd;

    ASSERT(m_ptr < m_end);
    switch (*m_ptr) {
    case '[':
        ++m_ptr;
        return TokenLeftBracket;
    case ']':
        ++m_ptr;
        return TokenRightBracket;
    case '.':
        ++m_ptr;
        return TokenDot;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return lexNumber(element);
    default:
        return lexIdentifier(element);
    }
    return TokenError;
}

static inline bool isSafeIdentifierStartCharacter(UChar c)
{
    return isASCIIAlpha(c) || (c == '_') || (c == '$');
}

static inline bool isSafeIdentifierCharacter(UChar c)
{
    return isASCIIAlphanumeric(c) || (c == '_') || (c == '$');
}

IDBKeyPathLexer::TokenType IDBKeyPathLexer::lexIdentifier(IDBKeyPathElement& element)
{
    const UChar* start = m_ptr;
    if (m_ptr < m_end && isSafeIdentifierStartCharacter(*m_ptr))
        ++m_ptr;
    else
        return TokenError;

    while (m_ptr < m_end && isSafeIdentifierCharacter(*m_ptr))
        ++m_ptr;

    element.type = IDBKeyPathElement::IsNamed;
    element.identifier = String(start, m_ptr - start);
    return TokenIdentifier;
}

IDBKeyPathLexer::TokenType IDBKeyPathLexer::lexNumber(IDBKeyPathElement& element)
{
    if (m_ptr >= m_end)
        return TokenError;

    const UChar* start = m_ptr;
    // [0-9]*
    while (m_ptr < m_end && isASCIIDigit(*m_ptr))
        ++m_ptr;

    String numberAsString;
    numberAsString = String(start, m_ptr - start);
    bool ok = false;
    unsigned number = numberAsString.toUIntStrict(&ok);
    if (!ok)
        return TokenError;

    element.type = IDBKeyPathElement::IsIndexed;
    element.index = number;
    return TokenNumber;
}

void IDBParseKeyPath(const String& keyPath, Vector<IDBKeyPathElement>& elements, IDBKeyPathParseError& error)
{
    // This is a simplified parser loosely based on LiteralParser.
    // An IDBKeyPath is defined as a sequence of:
    // identifierA{.identifierB{[numeric_value]}
    // where "{}" represents an optional part
    // The basic state machine is:
    // Start => {Identifier, Array}
    // Identifier => {Dot, Array, End}
    // Array => {Start, Dot, End}
    // Dot => {Identifier}
    // It bails out as soon as it finds an error, but doesn't discard the bits it managed to parse.
    enum ParserState { Identifier, Array, Dot, End };

    IDBKeyPathLexer lexer(keyPath);
    IDBKeyPathLexer::TokenType tokenType = lexer.nextTokenType();
    ParserState state;
    if (tokenType == IDBKeyPathLexer::TokenIdentifier)
        state = Identifier;
    else if (tokenType == IDBKeyPathLexer::TokenLeftBracket)
        state = Array;
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

            IDBKeyPathElement element = lexer.currentElement();
            ASSERT(element.type == IDBKeyPathElement::IsNamed);
            elements.append(element);

            tokenType = lexer.nextTokenType();
            if (tokenType == IDBKeyPathLexer::TokenDot)
                state = Dot;
            else if (tokenType == IDBKeyPathLexer::TokenLeftBracket)
                state = Array;
            else if (tokenType == IDBKeyPathLexer::TokenEnd)
                state = End;
            else {
                error = IDBKeyPathParseErrorIdentifier;
                return;
            }
            break;
        }
        case Array : {
            IDBKeyPathLexer::TokenType tokenType = lexer.currentTokenType();
            ASSERT(tokenType == IDBKeyPathLexer::TokenLeftBracket);

            tokenType = lexer.nextTokenType();
            if (tokenType != IDBKeyPathLexer::TokenNumber) {
                error = IDBKeyPathParseErrorArrayIndex;
                return;
            }

            ASSERT(tokenType == IDBKeyPathLexer::TokenNumber);
            IDBKeyPathElement element = lexer.currentElement();
            ASSERT(element.type == IDBKeyPathElement::IsIndexed);
            elements.append(element);

            tokenType = lexer.nextTokenType();
            if (tokenType != IDBKeyPathLexer::TokenRightBracket) {
                error = IDBKeyPathParseErrorArrayIndex;
                return;
            }

            tokenType = lexer.nextTokenType();
            if (tokenType == IDBKeyPathLexer::TokenDot)
                state = Dot;
            else if (tokenType == IDBKeyPathLexer::TokenLeftBracket)
                state = Array;
            else if (tokenType == IDBKeyPathLexer::TokenEnd)
                state = End;
            else {
                error = IDBKeyPathParseErrorAfterArray;
                return;
            }
            break;
        }
        case Dot: {
            IDBKeyPathLexer::TokenType tokenType = lexer.currentTokenType();
            ASSERT(tokenType == IDBKeyPathLexer::TokenDot);

            tokenType = lexer.nextTokenType();
            if (tokenType != IDBKeyPathLexer::TokenIdentifier) {
                error = IDBKeyPathParseErrorDot;
                return;
            }

            state = Identifier;
            break;
        }
        case End: {
            error = IDBKeyPathParseErrorNone;
            return;
        }
        }
    }
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
