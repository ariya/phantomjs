/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef LiteralParser_h
#define LiteralParser_h

#include "Identifier.h"
#include "JSCJSValue.h"
#include "JSGlobalObjectFunctions.h"
#include <wtf/text/WTFString.h>

namespace JSC {

typedef enum { StrictJSON, NonStrictJSON, JSONP } ParserMode;

enum JSONPPathEntryType {
    JSONPPathEntryTypeDeclare, // var pathEntryName = JSON
    JSONPPathEntryTypeDot, // <prior entries>.pathEntryName = JSON
    JSONPPathEntryTypeLookup, // <prior entries>[pathIndex] = JSON
    JSONPPathEntryTypeCall // <prior entries>(JSON)
};

enum ParserState { StartParseObject, StartParseArray, StartParseExpression, 
                   StartParseStatement, StartParseStatementEndStatement, 
                   DoParseObjectStartExpression, DoParseObjectEndExpression,
                   DoParseArrayStartExpression, DoParseArrayEndExpression };
enum TokenType { TokLBracket, TokRBracket, TokLBrace, TokRBrace, 
                 TokString, TokIdentifier, TokNumber, TokColon, 
                 TokLParen, TokRParen, TokComma, TokTrue, TokFalse,
                 TokNull, TokEnd, TokDot, TokAssign, TokSemi, TokError };
    
struct JSONPPathEntry {
    JSONPPathEntryType m_type;
    Identifier m_pathEntryName;
    int m_pathIndex;
};

struct JSONPData {
    Vector<JSONPPathEntry> m_path;
    Strong<Unknown> m_value;
};

template <typename CharType>
struct LiteralParserToken {
    TokenType type;
    const CharType* start;
    const CharType* end;
    String stringBuffer;
    union {
        double numberToken;
        struct {
            union {
                const LChar* stringToken8;
                const UChar* stringToken16;
            };
            unsigned stringIs8Bit : 1;
            unsigned stringLength : 31;
        };
    };
};

template <typename CharType>
ALWAYS_INLINE void setParserTokenString(LiteralParserToken<CharType>&, const CharType* string);

template <typename CharType>
class LiteralParser {
public:
    LiteralParser(ExecState* exec, const CharType* characters, unsigned length, ParserMode mode)
        : m_exec(exec)
        , m_lexer(characters, length, mode)
        , m_mode(mode)
    {
    }
    
    String getErrorMessage()
    { 
        if (!m_lexer.getErrorMessage().isEmpty())
            return String::format("JSON Parse error: %s", m_lexer.getErrorMessage().ascii().data()).impl();
        if (!m_parseErrorMessage.isEmpty())
            return String::format("JSON Parse error: %s", m_parseErrorMessage.ascii().data()).impl();
        return ASCIILiteral("JSON Parse error: Unable to parse JSON string");
    }
    
    JSValue tryLiteralParse()
    {
        m_lexer.next();
        JSValue result = parse(m_mode == StrictJSON ? StartParseExpression : StartParseStatement);
        if (m_lexer.currentToken().type == TokSemi)
            m_lexer.next();
        if (m_lexer.currentToken().type != TokEnd)
            return JSValue();
        return result;
    }
    
    bool tryJSONPParse(Vector<JSONPData>&, bool needsFullSourceInfo);

private:
    class Lexer {
    public:
        Lexer(const CharType* characters, unsigned length, ParserMode mode)
            : m_mode(mode)
            , m_ptr(characters)
            , m_end(characters + length)
        {
        }
        
        TokenType next();
        
        const LiteralParserToken<CharType>& currentToken()
        {
            return m_currentToken;
        }
        
        String getErrorMessage() { return m_lexErrorMessage; }
        
    private:
        String m_lexErrorMessage;
        template <ParserMode mode> TokenType lex(LiteralParserToken<CharType>&);
        ALWAYS_INLINE TokenType lexIdentifier(LiteralParserToken<CharType>&);
        template <ParserMode mode, char terminator> ALWAYS_INLINE TokenType lexString(LiteralParserToken<CharType>&);
        ALWAYS_INLINE TokenType lexNumber(LiteralParserToken<CharType>&);
        LiteralParserToken<CharType> m_currentToken;
        ParserMode m_mode;
        const CharType* m_ptr;
        const CharType* m_end;
    };
    
    class StackGuard;
    JSValue parse(ParserState);

    ExecState* m_exec;
    typename LiteralParser<CharType>::Lexer m_lexer;
    ParserMode m_mode;
    String m_parseErrorMessage;
    static unsigned const MaximumCachableCharacter = 128;
    FixedArray<Identifier, MaximumCachableCharacter> m_shortIdentifiers;
    FixedArray<Identifier, MaximumCachableCharacter> m_recentIdentifiers;
    ALWAYS_INLINE const Identifier makeIdentifier(const LChar* characters, size_t length);
    ALWAYS_INLINE const Identifier makeIdentifier(const UChar* characters, size_t length);
    };

}

#endif
