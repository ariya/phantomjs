/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Mathias Bynens (mathias@qiwi.be)
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

#include "config.h"
#include "LiteralParser.h"

#include "ButterflyInlines.h"
#include "CopiedSpaceInlines.h"
#include "JSArray.h"
#include "JSString.h"
#include "Lexer.h"
#include "ObjectConstructor.h"
#include "Operations.h"
#include "StrongInlines.h"
#include <wtf/ASCIICType.h>
#include <wtf/dtoa.h>
#include <wtf/text/StringBuilder.h>

namespace JSC {

template <typename CharType>
static inline bool isJSONWhiteSpace(const CharType& c)
{
    // The JSON RFC 4627 defines a list of allowed characters to be considered
    // insignificant white space: http://www.ietf.org/rfc/rfc4627.txt (2. JSON Grammar).
    return c == ' ' || c == 0x9 || c == 0xA || c == 0xD;
}

template <typename CharType>
bool LiteralParser<CharType>::tryJSONPParse(Vector<JSONPData>& results, bool needsFullSourceInfo)
{
    if (m_lexer.next() != TokIdentifier)
        return false;
    do {
        Vector<JSONPPathEntry> path;
        // Unguarded next to start off the lexer
        Identifier name = Identifier(&m_exec->vm(), m_lexer.currentToken().start, m_lexer.currentToken().end - m_lexer.currentToken().start);
        JSONPPathEntry entry;
        if (name == m_exec->vm().propertyNames->varKeyword) {
            if (m_lexer.next() != TokIdentifier)
                return false;
            entry.m_type = JSONPPathEntryTypeDeclare;
            entry.m_pathEntryName = Identifier(&m_exec->vm(), m_lexer.currentToken().start, m_lexer.currentToken().end - m_lexer.currentToken().start);
            path.append(entry);
        } else {
            entry.m_type = JSONPPathEntryTypeDot;
            entry.m_pathEntryName = Identifier(&m_exec->vm(), m_lexer.currentToken().start, m_lexer.currentToken().end - m_lexer.currentToken().start);
            path.append(entry);
        }
        if (m_exec->vm().keywords->isKeyword(entry.m_pathEntryName))
            return false;
        TokenType tokenType = m_lexer.next();
        if (entry.m_type == JSONPPathEntryTypeDeclare && tokenType != TokAssign)
            return false;
        while (tokenType != TokAssign) {
            switch (tokenType) {
            case TokLBracket: {
                entry.m_type = JSONPPathEntryTypeLookup;
                if (m_lexer.next() != TokNumber)
                    return false;
                double doubleIndex = m_lexer.currentToken().numberToken;
                int index = (int)doubleIndex;
                if (index != doubleIndex || index < 0)
                    return false;
                entry.m_pathIndex = index;
                if (m_lexer.next() != TokRBracket)
                    return false;
                break;
            }
            case TokDot: {
                entry.m_type = JSONPPathEntryTypeDot;
                if (m_lexer.next() != TokIdentifier)
                    return false;
                entry.m_pathEntryName = Identifier(&m_exec->vm(), m_lexer.currentToken().start, m_lexer.currentToken().end - m_lexer.currentToken().start);
                break;
            }
            case TokLParen: {
                if (path.last().m_type != JSONPPathEntryTypeDot || needsFullSourceInfo)
                    return false;
                path.last().m_type = JSONPPathEntryTypeCall;
                entry = path.last();
                goto startJSON;
            }
            default:
                return false;
            }
            path.append(entry);
            tokenType = m_lexer.next();
        }
    startJSON:
        m_lexer.next();
        results.append(JSONPData());
        results.last().m_value.set(m_exec->vm(), parse(StartParseExpression));
        if (!results.last().m_value)
            return false;
        results.last().m_path.swap(path);
        if (entry.m_type == JSONPPathEntryTypeCall) {
            if (m_lexer.currentToken().type != TokRParen)
                return false;
            m_lexer.next();
        }
        if (m_lexer.currentToken().type != TokSemi)
            break;
        m_lexer.next();
    } while (m_lexer.currentToken().type == TokIdentifier);
    return m_lexer.currentToken().type == TokEnd;
}
    
template <typename CharType>
ALWAYS_INLINE const Identifier LiteralParser<CharType>::makeIdentifier(const LChar* characters, size_t length)
{
    if (!length)
        return m_exec->vm().propertyNames->emptyIdentifier;
    if (characters[0] >= MaximumCachableCharacter)
        return Identifier(&m_exec->vm(), characters, length);

    if (length == 1) {
        if (!m_shortIdentifiers[characters[0]].isNull())
            return m_shortIdentifiers[characters[0]];
        m_shortIdentifiers[characters[0]] = Identifier(&m_exec->vm(), characters, length);
        return m_shortIdentifiers[characters[0]];
    }
    if (!m_recentIdentifiers[characters[0]].isNull() && Identifier::equal(m_recentIdentifiers[characters[0]].impl(), characters, length))
        return m_recentIdentifiers[characters[0]];
    m_recentIdentifiers[characters[0]] = Identifier(&m_exec->vm(), characters, length);
    return m_recentIdentifiers[characters[0]];
}

template <typename CharType>
ALWAYS_INLINE const Identifier LiteralParser<CharType>::makeIdentifier(const UChar* characters, size_t length)
{
    if (!length)
        return m_exec->vm().propertyNames->emptyIdentifier;
    if (characters[0] >= MaximumCachableCharacter)
        return Identifier(&m_exec->vm(), characters, length);

    if (length == 1) {
        if (!m_shortIdentifiers[characters[0]].isNull())
            return m_shortIdentifiers[characters[0]];
        m_shortIdentifiers[characters[0]] = Identifier(&m_exec->vm(), characters, length);
        return m_shortIdentifiers[characters[0]];
    }
    if (!m_recentIdentifiers[characters[0]].isNull() && Identifier::equal(m_recentIdentifiers[characters[0]].impl(), characters, length))
        return m_recentIdentifiers[characters[0]];
    m_recentIdentifiers[characters[0]] = Identifier(&m_exec->vm(), characters, length);
    return m_recentIdentifiers[characters[0]];
}

template <typename CharType>
template <ParserMode mode> TokenType LiteralParser<CharType>::Lexer::lex(LiteralParserToken<CharType>& token)
{
    while (m_ptr < m_end && isJSONWhiteSpace(*m_ptr))
        ++m_ptr;

    ASSERT(m_ptr <= m_end);
    if (m_ptr >= m_end) {
        token.type = TokEnd;
        token.start = token.end = m_ptr;
        return TokEnd;
    }
    token.type = TokError;
    token.start = m_ptr;
    switch (*m_ptr) {
        case '[':
            token.type = TokLBracket;
            token.end = ++m_ptr;
            return TokLBracket;
        case ']':
            token.type = TokRBracket;
            token.end = ++m_ptr;
            return TokRBracket;
        case '(':
            token.type = TokLParen;
            token.end = ++m_ptr;
            return TokLParen;
        case ')':
            token.type = TokRParen;
            token.end = ++m_ptr;
            return TokRParen;
        case '{':
            token.type = TokLBrace;
            token.end = ++m_ptr;
            return TokLBrace;
        case '}':
            token.type = TokRBrace;
            token.end = ++m_ptr;
            return TokRBrace;
        case ',':
            token.type = TokComma;
            token.end = ++m_ptr;
            return TokComma;
        case ':':
            token.type = TokColon;
            token.end = ++m_ptr;
            return TokColon;
        case '"':
            return lexString<mode, '"'>(token);
        case 't':
            if (m_end - m_ptr >= 4 && m_ptr[1] == 'r' && m_ptr[2] == 'u' && m_ptr[3] == 'e') {
                m_ptr += 4;
                token.type = TokTrue;
                token.end = m_ptr;
                return TokTrue;
            }
            break;
        case 'f':
            if (m_end - m_ptr >= 5 && m_ptr[1] == 'a' && m_ptr[2] == 'l' && m_ptr[3] == 's' && m_ptr[4] == 'e') {
                m_ptr += 5;
                token.type = TokFalse;
                token.end = m_ptr;
                return TokFalse;
            }
            break;
        case 'n':
            if (m_end - m_ptr >= 4 && m_ptr[1] == 'u' && m_ptr[2] == 'l' && m_ptr[3] == 'l') {
                m_ptr += 4;
                token.type = TokNull;
                token.end = m_ptr;
                return TokNull;
            }
            break;
        case '-':
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
            return lexNumber(token);
    }
    if (m_ptr < m_end) {
        if (*m_ptr == '.') {
            token.type = TokDot;
            token.end = ++m_ptr;
            return TokDot;
        }
        if (*m_ptr == '=') {
            token.type = TokAssign;
            token.end = ++m_ptr;
            return TokAssign;
        }
        if (*m_ptr == ';') {
            token.type = TokSemi;
            token.end = ++m_ptr;
            return TokAssign;
        }
        if (isASCIIAlpha(*m_ptr) || *m_ptr == '_' || *m_ptr == '$')
            return lexIdentifier(token);
        if (*m_ptr == '\'') {
            if (mode == StrictJSON) {
                m_lexErrorMessage = ASCIILiteral("Single quotes (\') are not allowed in JSON");
                return TokError;
            }
            return lexString<mode, '\''>(token);
        }
    }
    m_lexErrorMessage = String::format("Unrecognized token '%c'", *m_ptr).impl();
    return TokError;
}

template <>
ALWAYS_INLINE TokenType LiteralParser<LChar>::Lexer::lexIdentifier(LiteralParserToken<LChar>& token)
{
    while (m_ptr < m_end && (isASCIIAlphanumeric(*m_ptr) || *m_ptr == '_' || *m_ptr == '$'))
        m_ptr++;
    token.stringIs8Bit = 1;
    token.stringToken8 = token.start;
    token.stringLength = m_ptr - token.start;
    token.type = TokIdentifier;
    token.end = m_ptr;
    return TokIdentifier;
}

template <>
ALWAYS_INLINE TokenType LiteralParser<UChar>::Lexer::lexIdentifier(LiteralParserToken<UChar>& token)
{
    while (m_ptr < m_end && (isASCIIAlphanumeric(*m_ptr) || *m_ptr == '_' || *m_ptr == '$' || *m_ptr == 0x200C || *m_ptr == 0x200D))
        m_ptr++;
    token.stringIs8Bit = 0;
    token.stringToken16 = token.start;
    token.stringLength = m_ptr - token.start;
    token.type = TokIdentifier;
    token.end = m_ptr;
    return TokIdentifier;
}

template <typename CharType>
TokenType LiteralParser<CharType>::Lexer::next()
{
    if (m_mode == NonStrictJSON)
        return lex<NonStrictJSON>(m_currentToken);
    if (m_mode == JSONP)
        return lex<JSONP>(m_currentToken);
    return lex<StrictJSON>(m_currentToken);
}

template <>
ALWAYS_INLINE void setParserTokenString<LChar>(LiteralParserToken<LChar>& token, const LChar* string)
{
    token.stringIs8Bit = 1;
    token.stringToken8 = string;
}

template <>
ALWAYS_INLINE void setParserTokenString<UChar>(LiteralParserToken<UChar>& token, const UChar* string)
{
    token.stringIs8Bit = 0;
    token.stringToken16 = string;
}

template <ParserMode mode, typename CharType, LChar terminator> static inline bool isSafeStringCharacter(LChar c)
{
    return (c >= ' ' && c != '\\' && c != terminator) || (c == '\t' && mode != StrictJSON);
}

template <ParserMode mode, typename CharType, UChar terminator> static inline bool isSafeStringCharacter(UChar c)
{
    return (c >= ' ' && (mode == StrictJSON || c <= 0xff) && c != '\\' && c != terminator) || (c == '\t' && mode != StrictJSON);
}

template <typename CharType>
template <ParserMode mode, char terminator> ALWAYS_INLINE TokenType LiteralParser<CharType>::Lexer::lexString(LiteralParserToken<CharType>& token)
{
    ++m_ptr;
    const CharType* runStart = m_ptr;
    StringBuilder builder;
    do {
        runStart = m_ptr;
        while (m_ptr < m_end && isSafeStringCharacter<mode, CharType, terminator>(*m_ptr))
            ++m_ptr;
        if (builder.length())
            builder.append(runStart, m_ptr - runStart);
        if ((mode != NonStrictJSON) && m_ptr < m_end && *m_ptr == '\\') {
            if (builder.isEmpty() && runStart < m_ptr)
                builder.append(runStart, m_ptr - runStart);
            ++m_ptr;
            if (m_ptr >= m_end) {
                m_lexErrorMessage = ASCIILiteral("Unterminated string");
                return TokError;
            }
            switch (*m_ptr) {
                case '"':
                    builder.append('"');
                    m_ptr++;
                    break;
                case '\\':
                    builder.append('\\');
                    m_ptr++;
                    break;
                case '/':
                    builder.append('/');
                    m_ptr++;
                    break;
                case 'b':
                    builder.append('\b');
                    m_ptr++;
                    break;
                case 'f':
                    builder.append('\f');
                    m_ptr++;
                    break;
                case 'n':
                    builder.append('\n');
                    m_ptr++;
                    break;
                case 'r':
                    builder.append('\r');
                    m_ptr++;
                    break;
                case 't':
                    builder.append('\t');
                    m_ptr++;
                    break;

                case 'u':
                    if ((m_end - m_ptr) < 5) { 
                        m_lexErrorMessage = ASCIILiteral("\\u must be followed by 4 hex digits");
                        return TokError;
                    } // uNNNN == 5 characters
                    for (int i = 1; i < 5; i++) {
                        if (!isASCIIHexDigit(m_ptr[i])) {
                            m_lexErrorMessage = String::format("\"\\%s\" is not a valid unicode escape", String(m_ptr, 5).ascii().data()).impl();
                            return TokError;
                        }
                    }
                    builder.append(JSC::Lexer<CharType>::convertUnicode(m_ptr[1], m_ptr[2], m_ptr[3], m_ptr[4]));
                    m_ptr += 5;
                    break;

                default:
                    if (*m_ptr == '\'' && mode != StrictJSON) {
                        builder.append('\'');
                        m_ptr++;
                        break;
                    }
                    m_lexErrorMessage = String::format("Invalid escape character %c", *m_ptr).impl();
                    return TokError;
            }
        }
    } while ((mode != NonStrictJSON) && m_ptr != runStart && (m_ptr < m_end) && *m_ptr != terminator);

    if (m_ptr >= m_end || *m_ptr != terminator) {
        m_lexErrorMessage = ASCIILiteral("Unterminated string");
        return TokError;
    }

    if (builder.isEmpty()) {
        token.stringBuffer = String();
        setParserTokenString<CharType>(token, runStart);
        token.stringLength = m_ptr - runStart;
    } else {
        token.stringBuffer = builder.toString();
        if (token.stringBuffer.is8Bit()) {
            token.stringIs8Bit = 1;
            token.stringToken8 = token.stringBuffer.characters8();
        } else {
            token.stringIs8Bit = 0;
            token.stringToken16 = token.stringBuffer.characters16();
        }
        token.stringLength = token.stringBuffer.length();
    }
    token.type = TokString;
    token.end = ++m_ptr;
    return TokString;
}

template <typename CharType>
TokenType LiteralParser<CharType>::Lexer::lexNumber(LiteralParserToken<CharType>& token)
{
    // ES5 and json.org define numbers as
    // number
    //     int
    //     int frac? exp?
    //
    // int
    //     -? 0
    //     -? digit1-9 digits?
    //
    // digits
    //     digit digits?
    //
    // -?(0 | [1-9][0-9]*) ('.' [0-9]+)? ([eE][+-]? [0-9]+)?

    if (m_ptr < m_end && *m_ptr == '-') // -?
        ++m_ptr;
    
    // (0 | [1-9][0-9]*)
    if (m_ptr < m_end && *m_ptr == '0') // 0
        ++m_ptr;
    else if (m_ptr < m_end && *m_ptr >= '1' && *m_ptr <= '9') { // [1-9]
        ++m_ptr;
        // [0-9]*
        while (m_ptr < m_end && isASCIIDigit(*m_ptr))
            ++m_ptr;
    } else {
        m_lexErrorMessage = ASCIILiteral("Invalid number");
        return TokError;
    }

    // ('.' [0-9]+)?
    if (m_ptr < m_end && *m_ptr == '.') {
        ++m_ptr;
        // [0-9]+
        if (m_ptr >= m_end || !isASCIIDigit(*m_ptr)) {
            m_lexErrorMessage = ASCIILiteral("Invalid digits after decimal point");
            return TokError;
        }

        ++m_ptr;
        while (m_ptr < m_end && isASCIIDigit(*m_ptr))
            ++m_ptr;
    } else if (m_ptr < m_end && (*m_ptr != 'e' && *m_ptr != 'E') && (m_ptr - token.start) < 10) {
        int result = 0;
        token.type = TokNumber;
        token.end = m_ptr;
        const CharType* digit = token.start;
        int negative = 1;
        if (*digit == '-') {
            negative = -1;
            digit++;
        }
        
        while (digit < m_ptr)
            result = result * 10 + (*digit++) - '0';
        result *= negative;
        token.numberToken = result;
        return TokNumber;
    }

    //  ([eE][+-]? [0-9]+)?
    if (m_ptr < m_end && (*m_ptr == 'e' || *m_ptr == 'E')) { // [eE]
        ++m_ptr;

        // [-+]?
        if (m_ptr < m_end && (*m_ptr == '-' || *m_ptr == '+'))
            ++m_ptr;

        // [0-9]+
        if (m_ptr >= m_end || !isASCIIDigit(*m_ptr)) {
            m_lexErrorMessage = ASCIILiteral("Exponent symbols should be followed by an optional '+' or '-' and then by at least one number");
            return TokError;
        }
        
        ++m_ptr;
        while (m_ptr < m_end && isASCIIDigit(*m_ptr))
            ++m_ptr;
    }
    
    token.type = TokNumber;
    token.end = m_ptr;
    size_t parsedLength;
    token.numberToken = parseDouble(token.start, token.end - token.start, parsedLength);
    return TokNumber;
}

template <typename CharType>
JSValue LiteralParser<CharType>::parse(ParserState initialState)
{
    ParserState state = initialState;
    MarkedArgumentBuffer objectStack;
    JSValue lastValue;
    Vector<ParserState, 16, UnsafeVectorOverflow> stateStack;
    Vector<Identifier, 16, UnsafeVectorOverflow> identifierStack;
    while (1) {
        switch(state) {
            startParseArray:
            case StartParseArray: {
                JSArray* array = constructEmptyArray(m_exec, 0);
                objectStack.append(array);
                // fallthrough
            }
            doParseArrayStartExpression:
            case DoParseArrayStartExpression: {
                TokenType lastToken = m_lexer.currentToken().type;
                if (m_lexer.next() == TokRBracket) {
                    if (lastToken == TokComma) {
                        m_parseErrorMessage = ASCIILiteral("Unexpected comma at the end of array expression");
                        return JSValue();
                    }
                    m_lexer.next();
                    lastValue = objectStack.last();
                    objectStack.removeLast();
                    break;
                }

                stateStack.append(DoParseArrayEndExpression);
                goto startParseExpression;
            }
            case DoParseArrayEndExpression: {
                JSArray* array = asArray(objectStack.last());
                array->putDirectIndex(m_exec, array->length(), lastValue);
                
                if (m_lexer.currentToken().type == TokComma)
                    goto doParseArrayStartExpression;

                if (m_lexer.currentToken().type != TokRBracket) {
                    m_parseErrorMessage = ASCIILiteral("Expected ']'");
                    return JSValue();
                }
                
                m_lexer.next();
                lastValue = objectStack.last();
                objectStack.removeLast();
                break;
            }
            startParseObject:
            case StartParseObject: {
                JSObject* object = constructEmptyObject(m_exec);
                objectStack.append(object);

                TokenType type = m_lexer.next();
                if (type == TokString || (m_mode != StrictJSON && type == TokIdentifier)) {
                    LiteralParserToken<CharType> identifierToken = m_lexer.currentToken();

                    // Check for colon
                    if (m_lexer.next() != TokColon) {
                        m_parseErrorMessage = ASCIILiteral("Expected ':' before value in object property definition");
                        return JSValue();
                    }
                    
                    m_lexer.next();
                    if (identifierToken.stringIs8Bit)
                        identifierStack.append(makeIdentifier(identifierToken.stringToken8, identifierToken.stringLength));
                    else
                        identifierStack.append(makeIdentifier(identifierToken.stringToken16, identifierToken.stringLength));
                    stateStack.append(DoParseObjectEndExpression);
                    goto startParseExpression;
                }
                if (type != TokRBrace)  {
                    m_parseErrorMessage = ASCIILiteral("Expected '}'");
                    return JSValue();
                }
                m_lexer.next();
                lastValue = objectStack.last();
                objectStack.removeLast();
                break;
            }
            doParseObjectStartExpression:
            case DoParseObjectStartExpression: {
                TokenType type = m_lexer.next();
                if (type != TokString && (m_mode == StrictJSON || type != TokIdentifier)) {
                    m_parseErrorMessage = ASCIILiteral("Property name must be a string literal");
                    return JSValue();
                }
                LiteralParserToken<CharType> identifierToken = m_lexer.currentToken();

                // Check for colon
                if (m_lexer.next() != TokColon) {
                    m_parseErrorMessage = ASCIILiteral("Expected ':'");
                    return JSValue();
                }

                m_lexer.next();
                if (identifierToken.stringIs8Bit)
                    identifierStack.append(makeIdentifier(identifierToken.stringToken8, identifierToken.stringLength));
                else
                    identifierStack.append(makeIdentifier(identifierToken.stringToken16, identifierToken.stringLength));
                stateStack.append(DoParseObjectEndExpression);
                goto startParseExpression;
            }
            case DoParseObjectEndExpression:
            {
                JSObject* object = asObject(objectStack.last());
                PropertyName ident = identifierStack.last();
                unsigned i = ident.asIndex();
                if (i != PropertyName::NotAnIndex)
                    object->putDirectIndex(m_exec, i, lastValue);
                else
                    object->putDirect(m_exec->vm(), ident, lastValue);
                identifierStack.removeLast();
                if (m_lexer.currentToken().type == TokComma)
                    goto doParseObjectStartExpression;
                if (m_lexer.currentToken().type != TokRBrace) {
                    m_parseErrorMessage = ASCIILiteral("Expected '}'");
                    return JSValue();
                }
                m_lexer.next();
                lastValue = objectStack.last();
                objectStack.removeLast();
                break;
            }
            startParseExpression:
            case StartParseExpression: {
                switch (m_lexer.currentToken().type) {
                    case TokLBracket:
                        goto startParseArray;
                    case TokLBrace:
                        goto startParseObject;
                    case TokString: {
                        LiteralParserToken<CharType> stringToken = m_lexer.currentToken();
                        m_lexer.next();
                        if (stringToken.stringIs8Bit)
                            lastValue = jsString(m_exec, makeIdentifier(stringToken.stringToken8, stringToken.stringLength).string());
                        else
                            lastValue = jsString(m_exec, makeIdentifier(stringToken.stringToken16, stringToken.stringLength).string());
                        break;
                    }
                    case TokNumber: {
                        LiteralParserToken<CharType> numberToken = m_lexer.currentToken();
                        m_lexer.next();
                        lastValue = jsNumber(numberToken.numberToken);
                        break;
                    }
                    case TokNull:
                        m_lexer.next();
                        lastValue = jsNull();
                        break;

                    case TokTrue:
                        m_lexer.next();
                        lastValue = jsBoolean(true);
                        break;

                    case TokFalse:
                        m_lexer.next();
                        lastValue = jsBoolean(false);
                        break;
                    case TokRBracket:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ']'");
                        return JSValue();
                    case TokRBrace:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token '}'");
                        return JSValue();
                    case TokIdentifier: {
                        const LiteralParserToken<CharType>& token = m_lexer.currentToken();
                        if (token.stringIs8Bit)
                            m_parseErrorMessage = String::format("Unexpected identifier \"%s\"", String(m_lexer.currentToken().stringToken8, m_lexer.currentToken().stringLength).ascii().data()).impl();
                        else
                            m_parseErrorMessage = String::format("Unexpected identifier \"%s\"", String(m_lexer.currentToken().stringToken16, m_lexer.currentToken().stringLength).ascii().data()).impl();
                        return JSValue();
                    }
                    case TokColon:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ':'");
                        return JSValue();
                    case TokLParen:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token '('");
                        return JSValue();
                    case TokRParen:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ')'");
                        return JSValue();
                    case TokComma:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ','");
                        return JSValue();
                    case TokDot:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token '.'");
                        return JSValue();
                    case TokAssign:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token '='");
                        return JSValue();
                    case TokSemi:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ';'");
                        return JSValue();
                    case TokEnd:
                        m_parseErrorMessage = ASCIILiteral("Unexpected EOF");
                        return JSValue();
                    case TokError:
                    default:
                        // Error
                        m_parseErrorMessage = ASCIILiteral("Could not parse value expression");
                        return JSValue();
                }
                break;
            }
            case StartParseStatement: {
                switch (m_lexer.currentToken().type) {
                    case TokLBracket:
                    case TokNumber:
                    case TokString:
                        goto startParseExpression;

                    case TokLParen: {
                        m_lexer.next();
                        stateStack.append(StartParseStatementEndStatement);
                        goto startParseExpression;
                    }
                    case TokRBracket:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ']'");
                        return JSValue();
                    case TokLBrace:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token '{'");
                        return JSValue();
                    case TokRBrace:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token '}'");
                        return JSValue();
                    case TokIdentifier:
                        m_parseErrorMessage = ASCIILiteral("Unexpected identifier");
                        return JSValue();
                    case TokColon:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ':'");
                        return JSValue();
                    case TokRParen:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ')'");
                        return JSValue();
                    case TokComma:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ','");
                        return JSValue();
                    case TokTrue:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token 'true'");
                        return JSValue();
                    case TokFalse:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token 'false'");
                        return JSValue();
                    case TokNull:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token 'null'");
                        return JSValue();
                    case TokEnd:
                        m_parseErrorMessage = ASCIILiteral("Unexpected EOF");
                        return JSValue();
                    case TokDot:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token '.'");
                        return JSValue();
                    case TokAssign:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token '='");
                        return JSValue();
                    case TokSemi:
                        m_parseErrorMessage = ASCIILiteral("Unexpected token ';'");
                        return JSValue();
                    case TokError:
                    default:
                        m_parseErrorMessage = ASCIILiteral("Could not parse statement");
                        return JSValue();
                }
            }
            case StartParseStatementEndStatement: {
                ASSERT(stateStack.isEmpty());
                if (m_lexer.currentToken().type != TokRParen)
                    return JSValue();
                if (m_lexer.next() == TokEnd)
                    return lastValue;
                m_parseErrorMessage = ASCIILiteral("Unexpected content at end of JSON literal");
                return JSValue();
            }
            default:
                RELEASE_ASSERT_NOT_REACHED();
        }
        if (stateStack.isEmpty())
            return lastValue;
        state = stateStack.last();
        stateStack.removeLast();
        continue;
    }
}

// Instantiate the two flavors of LiteralParser we need instead of putting most of this file in LiteralParser.h
template class LiteralParser<LChar>;
template class LiteralParser<UChar>;

}
