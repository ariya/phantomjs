/*
 * Copyright 2005 Maksim Orlovich <maksim@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XPathParser_h
#define XPathParser_h

#include "XPathStep.h"
#include "XPathPredicate.h"

namespace WebCore {

    typedef int ExceptionCode;

    class XPathNSResolver;

    namespace XPath {

        class Expression;
        class ParseNode;
        class Predicate;

        struct Token {
            int type;
            String str;
            Step::Axis axis;
            NumericOp::Opcode numop;
            EqTestOp::Opcode eqop;
            
            Token(int t) : type(t) {}
            Token(int t, const String& v): type(t), str(v) {}
            Token(int t, Step::Axis v): type(t), axis(v) {}
            Token(int t, NumericOp::Opcode v): type(t), numop(v) {}
            Token(int t, EqTestOp::Opcode v): type(t), eqop(v) {}
        };

        class Parser {
            WTF_MAKE_NONCOPYABLE(Parser);
        public:
            Parser();
            ~Parser();

            XPathNSResolver* resolver() const { return m_resolver.get(); }
            bool expandQName(const String& qName, String& localName, String& namespaceURI);

            Expression* parseStatement(const String& statement, PassRefPtr<XPathNSResolver>, ExceptionCode&);

            static Parser* current() { return currentParser; }

            int lex(void* yylval);

            Expression* m_topExpr;
            bool m_gotNamespaceError;

            void registerParseNode(ParseNode*);
            void unregisterParseNode(ParseNode*);

            void registerPredicateVector(Vector<Predicate*>*);
            void deletePredicateVector(Vector<Predicate*>*);

            void registerExpressionVector(Vector<Expression*>*);
            void deleteExpressionVector(Vector<Expression*>*);

            void registerString(String*);
            void deleteString(String*);

            void registerNodeTest(Step::NodeTest*);
            void deleteNodeTest(Step::NodeTest*);

        private:
            bool isBinaryOperatorContext() const;

            void skipWS();
            Token makeTokenAndAdvance(int type, int advance = 1);
            Token makeTokenAndAdvance(int type, NumericOp::Opcode, int advance = 1);
            Token makeTokenAndAdvance(int type, EqTestOp::Opcode, int advance = 1);
            char peekAheadHelper();
            char peekCurHelper();

            Token lexString();
            Token lexNumber();
            bool lexNCName(String&);
            bool lexQName(String&);

            Token nextToken();
            Token nextTokenInternal();

            void reset(const String& data);

            static Parser* currentParser;

            unsigned m_nextPos;
            String m_data;
            int m_lastTokenType;
            RefPtr<XPathNSResolver> m_resolver;

            HashSet<ParseNode*> m_parseNodes;
            HashSet<Vector<Predicate*>*> m_predicateVectors;
            HashSet<Vector<Expression*>*> m_expressionVectors;
            HashSet<String*> m_strings;
            HashSet<Step::NodeTest*> m_nodeTests;
        };

    }
}

#endif
