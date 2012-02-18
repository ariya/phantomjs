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

#if ENABLE(XPATH)

#include "XPathFunctions.h"
#include "XPathNSResolver.h"
#include "XPathParser.h"
#include "XPathPath.h"
#include "XPathPredicate.h"
#include "XPathVariableReference.h"
#include <wtf/FastMalloc.h>

#define YYMALLOC fastMalloc
#define YYFREE fastFree

#define YYENABLE_NLS 0
#define YYLTYPE_IS_TRIVIAL 1
#define YYDEBUG 0
#define YYMAXDEPTH 10000
#define YYPARSE_PARAM parserParameter
#define PARSER static_cast<Parser*>(parserParameter)

using namespace WebCore;
using namespace XPath;

%}

%pure_parser

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
static void xpathyyerror(const char*) { }
    
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
        PARSER->m_topExpr = $1;
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
        PARSER->registerParseNode($$);
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
        PARSER->unregisterParseNode($1);
    }
    ;

RelativeLocationPath:
    Step
    {
        $$ = new LocationPath;
        $$->appendStep($1);
        PARSER->unregisterParseNode($1);
        PARSER->registerParseNode($$);
    }
    |
    RelativeLocationPath '/' Step
    {
        $$->appendStep($3);
        PARSER->unregisterParseNode($3);
    }
    |
    RelativeLocationPath DescendantOrSelf Step
    {
        $$->appendStep($2);
        $$->appendStep($3);
        PARSER->unregisterParseNode($2);
        PARSER->unregisterParseNode($3);
    }
    ;

Step:
    NodeTest OptionalPredicateList
    {
        if ($2) {
            $$ = new Step(Step::ChildAxis, *$1, *$2);
            PARSER->deletePredicateVector($2);
        } else
            $$ = new Step(Step::ChildAxis, *$1);
        PARSER->deleteNodeTest($1);
        PARSER->registerParseNode($$);
    }
    |
    NAMETEST OptionalPredicateList
    {
        String localName;
        String namespaceURI;
        if (!PARSER->expandQName(*$1, localName, namespaceURI)) {
            PARSER->m_gotNamespaceError = true;
            YYABORT;
        }
        
        if ($2) {
            $$ = new Step(Step::ChildAxis, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI), *$2);
            PARSER->deletePredicateVector($2);
        } else
            $$ = new Step(Step::ChildAxis, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI));
        PARSER->deleteString($1);
        PARSER->registerParseNode($$);
    }
    |
    AxisSpecifier NodeTest OptionalPredicateList
    {
        if ($3) {
            $$ = new Step($1, *$2, *$3);
            PARSER->deletePredicateVector($3);
        } else
            $$ = new Step($1, *$2);
        PARSER->deleteNodeTest($2);
        PARSER->registerParseNode($$);
    }
    |
    AxisSpecifier NAMETEST OptionalPredicateList
    {
        String localName;
        String namespaceURI;
        if (!PARSER->expandQName(*$2, localName, namespaceURI)) {
            PARSER->m_gotNamespaceError = true;
            YYABORT;
        }

        if ($3) {
            $$ = new Step($1, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI), *$3);
            PARSER->deletePredicateVector($3);
        } else
            $$ = new Step($1, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI));
        PARSER->deleteString($2);
        PARSER->registerParseNode($$);
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

        PARSER->deleteString($1);
        PARSER->registerNodeTest($$);
    }
    |
    PI '(' ')'
    {
        $$ = new Step::NodeTest(Step::NodeTest::ProcessingInstructionNodeTest);
        PARSER->deleteString($1);        
        PARSER->registerNodeTest($$);
    }
    |
    PI '(' LITERAL ')'
    {
        $$ = new Step::NodeTest(Step::NodeTest::ProcessingInstructionNodeTest, $3->stripWhiteSpace());
        PARSER->deleteString($1);        
        PARSER->deleteString($3);
        PARSER->registerNodeTest($$);
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
        PARSER->unregisterParseNode($1);
        PARSER->registerPredicateVector($$);
    }
    |
    PredicateList Predicate
    {
        $$->append(new Predicate($2));
        PARSER->unregisterParseNode($2);
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
        PARSER->registerParseNode($$);
    }
    ;

AbbreviatedStep:
    '.'
    {
        $$ = new Step(Step::SelfAxis, Step::NodeTest(Step::NodeTest::AnyNodeTest));
        PARSER->registerParseNode($$);
    }
    |
    DOTDOT
    {
        $$ = new Step(Step::ParentAxis, Step::NodeTest(Step::NodeTest::AnyNodeTest));
        PARSER->registerParseNode($$);
    }
    ;

PrimaryExpr:
    VARIABLEREFERENCE
    {
        $$ = new VariableReference(*$1);
        PARSER->deleteString($1);
        PARSER->registerParseNode($$);
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
        PARSER->deleteString($1);
        PARSER->registerParseNode($$);
    }
    |
    NUMBER
    {
        $$ = new Number($1->toDouble());
        PARSER->deleteString($1);
        PARSER->registerParseNode($$);
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
        PARSER->deleteString($1);
        PARSER->registerParseNode($$);
    }
    |
    FUNCTIONNAME '(' ArgumentList ')'
    {
        $$ = createFunction(*$1, *$3);
        if (!$$)
            YYABORT;
        PARSER->deleteString($1);
        PARSER->deleteExpressionVector($3);
        PARSER->registerParseNode($$);
    }
    ;

ArgumentList:
    Argument
    {
        $$ = new Vector<Expression*>;
        $$->append($1);
        PARSER->unregisterParseNode($1);
        PARSER->registerExpressionVector($$);
    }
    |
    ArgumentList ',' Argument
    {
        $$->append($3);
        PARSER->unregisterParseNode($3);
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
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
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
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    |
    FilterExpr DescendantOrSelf RelativeLocationPath
    {
        $3->insertFirstStep($2);
        $3->setAbsolute(true);
        $$ = new Path(static_cast<Filter*>($1), $3);
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($2);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    ;

FilterExpr:
    PrimaryExpr
    |
    PrimaryExpr PredicateList
    {
        $$ = new Filter($1, *$2);
        PARSER->unregisterParseNode($1);
        PARSER->deletePredicateVector($2);
        PARSER->registerParseNode($$);
    }
    ;

OrExpr:
    AndExpr
    |
    OrExpr OR AndExpr
    {
        $$ = new LogicalOp(LogicalOp::OP_Or, $1, $3);
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    ;

AndExpr:
    EqualityExpr
    |
    AndExpr AND EqualityExpr
    {
        $$ = new LogicalOp(LogicalOp::OP_And, $1, $3);
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    ;

EqualityExpr:
    RelationalExpr
    |
    EqualityExpr EQOP RelationalExpr
    {
        $$ = new EqTestOp($2, $1, $3);
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    ;

RelationalExpr:
    AdditiveExpr
    |
    RelationalExpr RELOP AdditiveExpr
    {
        $$ = new EqTestOp($2, $1, $3);
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    ;

AdditiveExpr:
    MultiplicativeExpr
    |
    AdditiveExpr PLUS MultiplicativeExpr
    {
        $$ = new NumericOp(NumericOp::OP_Add, $1, $3);
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    |
    AdditiveExpr MINUS MultiplicativeExpr
    {
        $$ = new NumericOp(NumericOp::OP_Sub, $1, $3);
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    ;

MultiplicativeExpr:
    UnaryExpr
    |
    MultiplicativeExpr MULOP UnaryExpr
    {
        $$ = new NumericOp($2, $1, $3);
        PARSER->unregisterParseNode($1);
        PARSER->unregisterParseNode($3);
        PARSER->registerParseNode($$);
    }
    ;

UnaryExpr:
    UnionExpr
    |
    MINUS UnaryExpr
    {
        $$ = new Negative;
        $$->addSubExpression($2);
        PARSER->unregisterParseNode($2);
        PARSER->registerParseNode($$);
    }
    ;

%%

#endif
