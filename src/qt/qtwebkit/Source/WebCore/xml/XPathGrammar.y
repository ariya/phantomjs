/*
 * Copyright 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
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

%{

#include "config.h"

#include "XPathFunctions.h"
#include "XPathNSResolver.h"
#include "XPathParser.h"
#include "XPathPath.h"
#include "XPathPredicate.h"
#include "XPathStep.h"
#include "XPathVariableReference.h"
#include <wtf/FastMalloc.h>

#define YYMALLOC fastMalloc
#define YYFREE fastFree

#define YYENABLE_NLS 0
#define YYLTYPE_IS_TRIVIAL 1
#define YYDEBUG 0
#define YYMAXDEPTH 10000

using namespace WebCore;
using namespace XPath;

%}

%pure_parser
%parse-param { WebCore::XPath::Parser* parser }

%union
{
    Step::Axis axis;
    Step::NodeTest* nodeTest;
    NumericOp::Opcode numop;
    EqTestOp::Opcode eqop;
    String* str;
    Expression* expr;
    Vector<Predicate*>* predList;
    Vector<Expression*>* argList;
    Step* step;
    LocationPath* locationPath;
}

%{

static int xpathyylex(YYSTYPE* yylval) { return Parser::current()->lex(yylval); }
static void xpathyyerror(void*, const char*) { }
    
%}

%left <numop> MULOP
%left <eqop> EQOP RELOP
%left PLUS MINUS
%left OR AND
%token <axis> AXISNAME
%token <str> NODETYPE PI FUNCTIONNAME LITERAL
%token <str> VARIABLEREFERENCE NUMBER
%token DOTDOT SLASHSLASH
%token <str> NAMETEST
%token XPATH_ERROR

%type <locationPath> LocationPath
%type <locationPath> AbsoluteLocationPath
%type <locationPath> RelativeLocationPath
%type <step> Step
%type <axis> AxisSpecifier
%type <step> DescendantOrSelf
%type <nodeTest> NodeTest
%type <expr> Predicate
%type <predList> OptionalPredicateList
%type <predList> PredicateList
%type <step> AbbreviatedStep
%type <expr> Expr
%type <expr> PrimaryExpr
%type <expr> FunctionCall
%type <argList> ArgumentList
%type <expr> Argument
%type <expr> UnionExpr
%type <expr> PathExpr
%type <expr> FilterExpr
%type <expr> OrExpr
%type <expr> AndExpr
%type <expr> EqualityExpr
%type <expr> RelationalExpr
%type <expr> AdditiveExpr
%type <expr> MultiplicativeExpr
%type <expr> UnaryExpr

%%

Expr:
    OrExpr
    {
        parser->m_topExpr = $1;
    }
    ;

LocationPath:
    RelativeLocationPath
    {
        $$->setAbsolute(false);
    }
    |
    AbsoluteLocationPath
    {
        $$->setAbsolute(true);
    }
    ;

AbsoluteLocationPath:
    '/'
    {
        $$ = new LocationPath;
        parser->registerParseNode($$);
    }
    |
    '/' RelativeLocationPath
    {
        $$ = $2;
    }
    |
    DescendantOrSelf RelativeLocationPath
    {
        $$ = $2;
        $$->insertFirstStep($1);
        parser->unregisterParseNode($1);
    }
    ;

RelativeLocationPath:
    Step
    {
        $$ = new LocationPath;
        $$->appendStep($1);
        parser->unregisterParseNode($1);
        parser->registerParseNode($$);
    }
    |
    RelativeLocationPath '/' Step
    {
        $$->appendStep($3);
        parser->unregisterParseNode($3);
    }
    |
    RelativeLocationPath DescendantOrSelf Step
    {
        $$->appendStep($2);
        $$->appendStep($3);
        parser->unregisterParseNode($2);
        parser->unregisterParseNode($3);
    }
    ;

Step:
    NodeTest OptionalPredicateList
    {
        if ($2) {
            $$ = new Step(Step::ChildAxis, *$1, *$2);
            parser->deletePredicateVector($2);
        } else
            $$ = new Step(Step::ChildAxis, *$1);
        parser->deleteNodeTest($1);
        parser->registerParseNode($$);
    }
    |
    NAMETEST OptionalPredicateList
    {
        String localName;
        String namespaceURI;
        if (!parser->expandQName(*$1, localName, namespaceURI)) {
            parser->m_gotNamespaceError = true;
            YYABORT;
        }
        
        if ($2) {
            $$ = new Step(Step::ChildAxis, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI), *$2);
            parser->deletePredicateVector($2);
        } else
            $$ = new Step(Step::ChildAxis, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI));
        parser->deleteString($1);
        parser->registerParseNode($$);
    }
    |
    AxisSpecifier NodeTest OptionalPredicateList
    {
        if ($3) {
            $$ = new Step($1, *$2, *$3);
            parser->deletePredicateVector($3);
        } else
            $$ = new Step($1, *$2);
        parser->deleteNodeTest($2);
        parser->registerParseNode($$);
    }
    |
    AxisSpecifier NAMETEST OptionalPredicateList
    {
        String localName;
        String namespaceURI;
        if (!parser->expandQName(*$2, localName, namespaceURI)) {
            parser->m_gotNamespaceError = true;
            YYABORT;
        }

        if ($3) {
            $$ = new Step($1, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI), *$3);
            parser->deletePredicateVector($3);
        } else
            $$ = new Step($1, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI));
        parser->deleteString($2);
        parser->registerParseNode($$);
    }
    |
    AbbreviatedStep
    ;

AxisSpecifier:
    AXISNAME
    |
    '@'
    {
        $$ = Step::AttributeAxis;
    }
    ;

NodeTest:
    NODETYPE '(' ')'
    {
        if (*$1 == "node")
            $$ = new Step::NodeTest(Step::NodeTest::AnyNodeTest);
        else if (*$1 == "text")
            $$ = new Step::NodeTest(Step::NodeTest::TextNodeTest);
        else if (*$1 == "comment")
            $$ = new Step::NodeTest(Step::NodeTest::CommentNodeTest);

        parser->deleteString($1);
        parser->registerNodeTest($$);
    }
    |
    PI '(' ')'
    {
        $$ = new Step::NodeTest(Step::NodeTest::ProcessingInstructionNodeTest);
        parser->deleteString($1);
        parser->registerNodeTest($$);
    }
    |
    PI '(' LITERAL ')'
    {
        $$ = new Step::NodeTest(Step::NodeTest::ProcessingInstructionNodeTest, $3->stripWhiteSpace());
        parser->deleteString($1);
        parser->deleteString($3);
        parser->registerNodeTest($$);
    }
    ;

OptionalPredicateList:
    /* empty */
    {
        $$ = 0;
    }
    |
    PredicateList
    ;

PredicateList:
    Predicate
    {
        $$ = new Vector<Predicate*>;
        $$->append(new Predicate($1));
        parser->unregisterParseNode($1);
        parser->registerPredicateVector($$);
    }
    |
    PredicateList Predicate
    {
        $$->append(new Predicate($2));
        parser->unregisterParseNode($2);
    }
    ;

Predicate:
    '[' Expr ']'
    {
        $$ = $2;
    }
    ;

DescendantOrSelf:
    SLASHSLASH
    {
        $$ = new Step(Step::DescendantOrSelfAxis, Step::NodeTest(Step::NodeTest::AnyNodeTest));
        parser->registerParseNode($$);
    }
    ;

AbbreviatedStep:
    '.'
    {
        $$ = new Step(Step::SelfAxis, Step::NodeTest(Step::NodeTest::AnyNodeTest));
        parser->registerParseNode($$);
    }
    |
    DOTDOT
    {
        $$ = new Step(Step::ParentAxis, Step::NodeTest(Step::NodeTest::AnyNodeTest));
        parser->registerParseNode($$);
    }
    ;

PrimaryExpr:
    VARIABLEREFERENCE
    {
        $$ = new VariableReference(*$1);
        parser->deleteString($1);
        parser->registerParseNode($$);
    }
    |
    '(' Expr ')'
    {
        $$ = $2;
    }
    |
    LITERAL
    {
        $$ = new StringExpression(*$1);
        parser->deleteString($1);
        parser->registerParseNode($$);
    }
    |
    NUMBER
    {
        $$ = new Number($1->toDouble());
        parser->deleteString($1);
        parser->registerParseNode($$);
    }
    |
    FunctionCall
    ;

FunctionCall:
    FUNCTIONNAME '(' ')'
    {
        $$ = createFunction(*$1);
        if (!$$)
            YYABORT;
        parser->deleteString($1);
        parser->registerParseNode($$);
    }
    |
    FUNCTIONNAME '(' ArgumentList ')'
    {
        $$ = createFunction(*$1, *$3);
        if (!$$)
            YYABORT;
        parser->deleteString($1);
        parser->deleteExpressionVector($3);
        parser->registerParseNode($$);
    }
    ;

ArgumentList:
    Argument
    {
        $$ = new Vector<Expression*>;
        $$->append($1);
        parser->unregisterParseNode($1);
        parser->registerExpressionVector($$);
    }
    |
    ArgumentList ',' Argument
    {
        $$->append($3);
        parser->unregisterParseNode($3);
    }
    ;

Argument:
    Expr
    ;

UnionExpr:
    PathExpr
    |
    UnionExpr '|' PathExpr
    {
        $$ = new Union;
        $$->addSubExpression($1);
        $$->addSubExpression($3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    ;

PathExpr:
    LocationPath
    {
        $$ = $1;
    }
    |
    FilterExpr
    |
    FilterExpr '/' RelativeLocationPath
    {
        $3->setAbsolute(true);
        $$ = new Path(static_cast<Filter*>($1), $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    |
    FilterExpr DescendantOrSelf RelativeLocationPath
    {
        $3->insertFirstStep($2);
        $3->setAbsolute(true);
        $$ = new Path(static_cast<Filter*>($1), $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($2);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    ;

FilterExpr:
    PrimaryExpr
    |
    PrimaryExpr PredicateList
    {
        $$ = new Filter($1, *$2);
        parser->unregisterParseNode($1);
        parser->deletePredicateVector($2);
        parser->registerParseNode($$);
    }
    ;

OrExpr:
    AndExpr
    |
    OrExpr OR AndExpr
    {
        $$ = new LogicalOp(LogicalOp::OP_Or, $1, $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    ;

AndExpr:
    EqualityExpr
    |
    AndExpr AND EqualityExpr
    {
        $$ = new LogicalOp(LogicalOp::OP_And, $1, $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    ;

EqualityExpr:
    RelationalExpr
    |
    EqualityExpr EQOP RelationalExpr
    {
        $$ = new EqTestOp($2, $1, $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    ;

RelationalExpr:
    AdditiveExpr
    |
    RelationalExpr RELOP AdditiveExpr
    {
        $$ = new EqTestOp($2, $1, $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    ;

AdditiveExpr:
    MultiplicativeExpr
    |
    AdditiveExpr PLUS MultiplicativeExpr
    {
        $$ = new NumericOp(NumericOp::OP_Add, $1, $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    |
    AdditiveExpr MINUS MultiplicativeExpr
    {
        $$ = new NumericOp(NumericOp::OP_Sub, $1, $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    ;

MultiplicativeExpr:
    UnaryExpr
    |
    MultiplicativeExpr MULOP UnaryExpr
    {
        $$ = new NumericOp($2, $1, $3);
        parser->unregisterParseNode($1);
        parser->unregisterParseNode($3);
        parser->registerParseNode($$);
    }
    ;

UnaryExpr:
    UnionExpr
    |
    MINUS UnaryExpr
    {
        $$ = new Negative;
        $$->addSubExpression($2);
        parser->unregisterParseNode($2);
        parser->registerParseNode($$);
    }
    ;

%%
