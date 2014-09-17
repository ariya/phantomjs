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

#include "JSGlobalObjectFunctions.h"
#include "JSValue.h"
#include "UString.h"

namespace JSC {

    class LiteralParser {
    public:
        typedef enum { StrictJSON, NonStrictJSON } ParserMode;
        LiteralParser(ExecState* exec, const UString& s, ParserMode mode)
            : m_exec(exec)
            , m_lexer(s, mode)
            , m_mode(mode)
        {
        }
        
        JSValue tryLiteralParse()
        {
            m_lexer.next();
            JSValue result = parse(m_mode == StrictJSON ? StartParseExpression : StartParseStatement);
            if (m_lexer.currentToken().type != TokEnd)
                return JSValue();
            return result;
        }
    private:
        enum ParserState { StartParseObject, StartParseArray, StartParseExpression, 
                           StartParseStatement, StartParseStatementEndStatement, 
                           DoParseObjectStartExpression, DoParseObjectEndExpression,
                           DoParseArrayStartExpression, DoParseArrayEndExpression };
        enum TokenType { TokLBracket, TokRBracket, TokLBrace, TokRBrace, 
                         TokString, TokIdentifier, TokNumber, TokColon, 
                         TokLParen, TokRParen, TokComma, TokTrue, TokFalse,
                         TokNull, TokEnd, TokError };

        class Lexer {
        public:
            struct LiteralParserToken {
                TokenType type;
                const UChar* start;
                const UChar* end;
                UString stringToken;
                double numberToken;
            };
            Lexer(const UString& s, ParserMode mode)
                : m_string(s)
                , m_mode(mode)
                , m_ptr(s.characters())
                , m_end(s.characters() + s.length())
            {
            }
            
            TokenType next()
            {
                return lex(m_currentToken);
            }
            
            const LiteralParserToken& currentToken()
            {
                return m_currentToken;
            }
            
        private:
            TokenType lex(LiteralParserToken&);
            template <ParserMode mode> TokenType lexString(LiteralParserToken&);
            TokenType lexNumber(LiteralParserToken&);
            LiteralParserToken m_currentToken;
            UString m_string;
            ParserMode m_mode;
            const UChar* m_ptr;
            const UChar* m_end;
        };
        
        class StackGuard;
        JSValue parse(ParserState);

        ExecState* m_exec;
        LiteralParser::Lexer m_lexer;
        ParserMode m_mode;
    };
}

#endif
