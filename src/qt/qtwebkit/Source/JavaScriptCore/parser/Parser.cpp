/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009, 2010, 2013 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Parser.h"

#include "ASTBuilder.h"
#include "CodeBlock.h"
#include "Debugger.h"
#include "JSCJSValueInlines.h"
#include "Lexer.h"
#include "NodeInfo.h"
#include "SourceProvider.h"
#include "VM.h"
#include <utility>
#include <wtf/HashFunctions.h>
#include <wtf/OwnPtr.h>
#include <wtf/WTFThreadData.h>

#define fail() do { if (!hasError()) updateErrorMessage(); return 0; } while (0)
#define failWithToken(tok) do { if (!hasError()) updateErrorMessage(tok); return 0; } while (0)
#define failWithMessage(msg) do { if (!hasError()) updateErrorMessage(msg); return 0; } while (0)
#define failWithNameAndMessage(before, name, after) do { if (!hasError()) updateErrorWithNameAndMessage(before, name, after); return 0; } while (0)
#define failWithStackOverflow() do { updateErrorMessage("Stack exhausted"); m_hasStackOverflow = true; return 0; } while (0)
#define failIfFalse(cond) do { if (!(cond)) fail(); } while (0)
#define failIfFalseWithMessage(cond, msg) do { if (!(cond)) failWithMessage(msg); } while (0)
#define failIfFalseWithNameAndMessage(cond, before, name, msg) do { if (!(cond)) failWithNameAndMessage(before, name, msg); } while (0)
#define failIfTrue(cond) do { if ((cond)) fail(); } while (0)
#define failIfTrueWithMessage(cond, msg) do { if ((cond)) failWithMessage(msg); } while (0)
#define failIfTrueWithNameAndMessage(cond, before, name, msg) do { if ((cond)) failWithNameAndMessage(before, name, msg); } while (0)
#define failIfTrueIfStrict(cond) do { if ((cond) && strictMode()) fail(); } while (0)
#define failIfTrueIfStrictWithMessage(cond, msg) do { if ((cond) && strictMode()) failWithMessage(msg); } while (0)
#define failIfTrueIfStrictWithNameAndMessage(cond, before, name, after) do { if ((cond) && strictMode()) failWithNameAndMessage(before, name, after); } while (0)
#define failIfFalseIfStrict(cond) do { if ((!(cond)) && strictMode()) fail(); } while (0)
#define failIfFalseIfStrictWithMessage(cond, msg) do { if ((!(cond)) && strictMode()) failWithMessage(msg); } while (0)
#define failIfFalseIfStrictWithNameAndMessage(cond, before, name, after) do { if ((!(cond)) && strictMode()) failWithNameAndMessage(before, name, after); } while (0)
#define consumeOrFail(tokenType) do { if (!consume(tokenType)) failWithToken(tokenType); } while (0)
#define consumeOrFailWithFlags(tokenType, flags) do { if (!consume(tokenType, flags)) failWithToken(tokenType); } while (0)
#define matchOrFail(tokenType) do { if (!match(tokenType)) failWithToken(tokenType); } while (0)
#define failIfStackOverflow() do { if (!canRecurse()) failWithStackOverflow(); } while (0)

using namespace std;

namespace JSC {

template <typename LexerType>
Parser<LexerType>::Parser(VM* vm, const SourceCode& source, FunctionParameters* parameters, const Identifier& name, JSParserStrictness strictness, JSParserMode parserMode)
    : m_vm(vm)
    , m_source(&source)
    , m_stack(*vm, wtfThreadData().stack())
    , m_hasStackOverflow(false)
    , m_allowsIn(true)
    , m_assignmentCount(0)
    , m_nonLHSCount(0)
    , m_syntaxAlreadyValidated(source.provider()->isValid())
    , m_statementDepth(0)
    , m_nonTrivialExpressionCount(0)
    , m_lastIdentifier(0)
    , m_sourceElements(0)
{
    m_lexer = adoptPtr(new LexerType(vm));
    m_arena = m_vm->parserArena.get();
    m_lexer->setCode(source, m_arena);
    m_token.m_location.endOffset = source.startOffset();
    m_token.m_location.lineStartOffset = source.startOffset();

    m_functionCache = vm->addSourceProviderCache(source.provider());
    ScopeRef scope = pushScope();
    if (parserMode == JSParseFunctionCode)
        scope->setIsFunction();
    if (strictness == JSParseStrict)
        scope->setStrictMode();
    if (parameters) {
        for (unsigned i = 0; i < parameters->size(); i++)
            scope->declareParameter(&parameters->at(i));
    }
    if (!name.isNull())
        scope->declareCallee(&name);
    next();
}

template <typename LexerType>
Parser<LexerType>::~Parser()
{
}

template <typename LexerType>
String Parser<LexerType>::parseInner()
{
    String parseError = String();
    
    ASTBuilder context(const_cast<VM*>(m_vm), const_cast<SourceCode*>(m_source));
    if (m_lexer->isReparsing())
        m_statementDepth--;
    ScopeRef scope = currentScope();
    SourceElements* sourceElements = parseSourceElements<CheckForStrictMode>(context);
    if (!sourceElements || !consume(EOFTOK)) {
        if (hasError())
            parseError = m_errorMessage;
        else
            parseError = ASCIILiteral("Parser error");
    }

    IdentifierSet capturedVariables;
    scope->getCapturedVariables(capturedVariables);
    CodeFeatures features = context.features();
    if (scope->strictMode())
        features |= StrictModeFeature;
    if (scope->shadowsArguments())
        features |= ShadowsArgumentsFeature;

    didFinishParsing(sourceElements, context.varDeclarations(), context.funcDeclarations(), features,
        context.numConstants(), capturedVariables);

    return parseError;
}

template <typename LexerType>
void Parser<LexerType>::didFinishParsing(SourceElements* sourceElements, ParserArenaData<DeclarationStacks::VarStack>* varStack, 
    ParserArenaData<DeclarationStacks::FunctionStack>* funcStack, CodeFeatures features, int numConstants, IdentifierSet& capturedVars)
{
    m_sourceElements = sourceElements;
    m_varDeclarations = varStack;
    m_funcDeclarations = funcStack;
    m_capturedVariables.swap(capturedVars);
    m_features = features;
    m_numConstants = numConstants;
}

template <typename LexerType>
bool Parser<LexerType>::allowAutomaticSemicolon()
{
    return match(CLOSEBRACE) || match(EOFTOK) || m_lexer->prevTerminator();
}

template <typename LexerType>
template <SourceElementsMode mode, class TreeBuilder> TreeSourceElements Parser<LexerType>::parseSourceElements(TreeBuilder& context)
{
    const unsigned lengthOfUseStrictLiteral = 12; // "use strict".length
    TreeSourceElements sourceElements = context.createSourceElements();
    bool seenNonDirective = false;
    const Identifier* directive = 0;
    unsigned directiveLiteralLength = 0;
    unsigned startOffset = m_token.m_location.startOffset;
    unsigned startLineStartOffset = m_token.m_location.lineStartOffset;
    unsigned oldLastLineNumber = m_lexer->lastLineNumber();
    unsigned oldLineNumber = m_lexer->lineNumber();
    bool hasSetStrict = false;
    while (TreeStatement statement = parseStatement(context, directive, &directiveLiteralLength)) {
        if (mode == CheckForStrictMode && !seenNonDirective) {
            if (directive) {
                // "use strict" must be the exact literal without escape sequences or line continuation.
                if (!hasSetStrict && directiveLiteralLength == lengthOfUseStrictLiteral && m_vm->propertyNames->useStrictIdentifier == *directive) {
                    setStrictMode();
                    hasSetStrict = true;
                    failIfFalse(isValidStrictMode());
                    m_lexer->setOffset(startOffset, startLineStartOffset);
                    next();
                    m_lexer->setLastLineNumber(oldLastLineNumber);
                    m_lexer->setLineNumber(oldLineNumber);
                    failIfTrue(hasError());
                    continue;
                }
            } else
                seenNonDirective = true;
        }
        context.appendStatement(sourceElements, statement);
    }

    failIfTrue(hasError());
    return sourceElements;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseVarDeclaration(TreeBuilder& context)
{
    ASSERT(match(VAR));
    JSTokenLocation location(tokenLocation());
    int start = tokenLine();
    int end = 0;
    int scratch;
    const Identifier* scratch1 = 0;
    TreeExpression scratch2 = 0;
    JSTextPosition scratch3;
    TreeExpression varDecls = parseVarDeclarationList(context, scratch, scratch1, scratch2, scratch3, scratch3, scratch3);
    failIfTrue(hasError());
    failIfFalse(autoSemiColon());
    
    return context.createVarStatement(location, varDecls, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseConstDeclaration(TreeBuilder& context)
{
    ASSERT(match(CONSTTOKEN));
    JSTokenLocation location(tokenLocation());
    int start = tokenLine();
    int end = 0;
    TreeConstDeclList constDecls = parseConstDeclarationList(context);
    failIfTrue(hasError());
    failIfFalse(autoSemiColon());
    
    return context.createConstStatement(location, constDecls, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseDoWhileStatement(TreeBuilder& context)
{
    ASSERT(match(DO));
    int startLine = tokenLine();
    next();
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement);
    int endLine = tokenLine();
    JSTokenLocation location(tokenLocation());
    consumeOrFail(WHILE);
    consumeOrFail(OPENPAREN);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    consumeOrFail(CLOSEPAREN);
    if (match(SEMICOLON))
        next(); // Always performs automatic semicolon insertion.
    return context.createDoWhileStatement(location, statement, expr, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseWhileStatement(TreeBuilder& context)
{
    ASSERT(match(WHILE));
    JSTokenLocation location(tokenLocation());
    int startLine = tokenLine();
    next();
    consumeOrFail(OPENPAREN);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    int endLine = tokenLine();
    consumeOrFail(CLOSEPAREN);
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement);
    return context.createWhileStatement(location, expr, statement, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseVarDeclarationList(TreeBuilder& context, int& declarations, const Identifier*& lastIdent, TreeExpression& lastInitializer, JSTextPosition& identStart, JSTextPosition& initStart, JSTextPosition& initEnd)
{
    TreeExpression varDecls = 0;
    do {
        declarations++;
        JSTokenLocation location(tokenLocation());
        next();
        matchOrFail(IDENT);
        
        JSTextPosition varStart = tokenStartPosition();
        identStart = varStart;
        const Identifier* name = m_token.m_data.ident;
        lastIdent = name;
        next();
        bool hasInitializer = match(EQUAL);
        failIfFalseIfStrictWithNameAndMessage(declareVariable(name), "Cannot declare a variable named", name->impl(), "in strict mode.");
        context.addVar(name, (hasInitializer || (!m_allowsIn && match(INTOKEN))) ? DeclarationStacks::HasInitializer : 0);
        if (hasInitializer) {
            JSTextPosition varDivot = tokenStartPosition() + 1;
            initStart = tokenStartPosition();
            next(TreeBuilder::DontBuildStrings); // consume '='
            TreeExpression initializer = parseAssignmentExpression(context);
            initEnd = lastTokenEndPosition();
            lastInitializer = initializer;
            failIfFalse(initializer);
            
            TreeExpression node = context.createAssignResolve(location, *name, initializer, varStart, varDivot, lastTokenEndPosition());
            if (!varDecls)
                varDecls = node;
            else
                varDecls = context.combineCommaNodes(location, varDecls, node);
        }
    } while (match(COMMA));
    return varDecls;
}

template <typename LexerType>
template <class TreeBuilder> TreeConstDeclList Parser<LexerType>::parseConstDeclarationList(TreeBuilder& context)
{
    failIfTrue(strictMode());
    TreeConstDeclList constDecls = 0;
    TreeConstDeclList tail = 0;
    do {
        JSTokenLocation location(tokenLocation());
        next();
        matchOrFail(IDENT);
        const Identifier* name = m_token.m_data.ident;
        next();
        bool hasInitializer = match(EQUAL);
        declareVariable(name);
        context.addVar(name, DeclarationStacks::IsConstant | (hasInitializer ? DeclarationStacks::HasInitializer : 0));

        TreeExpression initializer = 0;
        if (hasInitializer) {
            next(TreeBuilder::DontBuildStrings); // consume '='
            initializer = parseAssignmentExpression(context);
        }
        tail = context.appendConstDecl(location, tail, name, initializer);
        if (!constDecls)
            constDecls = tail;
    } while (match(COMMA));
    return constDecls;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseForStatement(TreeBuilder& context)
{
    ASSERT(match(FOR));
    JSTokenLocation location(tokenLocation());
    int startLine = tokenLine();
    next();
    consumeOrFail(OPENPAREN);
    int nonLHSCount = m_nonLHSCount;
    int declarations = 0;
    JSTextPosition declsStart;
    JSTextPosition declsEnd;
    TreeExpression decls = 0;
    if (match(VAR)) {
        /*
         for (var IDENT in expression) statement
         for (var IDENT = expression in expression) statement
         for (var varDeclarationList; expressionOpt; expressionOpt)
         */
        const Identifier* forInTarget = 0;
        TreeExpression forInInitializer = 0;
        m_allowsIn = false;
        JSTextPosition initStart;
        JSTextPosition initEnd;
        decls = parseVarDeclarationList(context, declarations, forInTarget, forInInitializer, declsStart, initStart, initEnd);
        m_allowsIn = true;
        failIfTrue(hasError());

        // Remainder of a standard for loop is handled identically
        if (match(SEMICOLON))
            goto standardForLoop;
        
        failIfFalse(declarations == 1);
        
        // Handle for-in with var declaration
        JSTextPosition inLocation = tokenStartPosition();
        consumeOrFail(INTOKEN);
        
        TreeExpression expr = parseExpression(context);
        failIfFalse(expr);
        JSTextPosition exprEnd = lastTokenEndPosition();
        
        int endLine = tokenLine();
        consumeOrFail(CLOSEPAREN);
        
        const Identifier* unused = 0;
        startLoop();
        TreeStatement statement = parseStatement(context, unused);
        endLoop();
        failIfFalse(statement);
        
        return context.createForInLoop(location, forInTarget, forInInitializer, expr, statement, declsStart, inLocation, exprEnd, initStart, initEnd, startLine, endLine);
    }
    
    if (!match(SEMICOLON)) {
        m_allowsIn = false;
        declsStart = tokenStartPosition();
        decls = parseExpression(context);
        declsEnd = lastTokenEndPosition();
        m_allowsIn = true;
        failIfFalse(decls);
    }
    
    if (match(SEMICOLON)) {
    standardForLoop:
        // Standard for loop
        next();
        TreeExpression condition = 0;
        
        if (!match(SEMICOLON)) {
            condition = parseExpression(context);
            failIfFalse(condition);
        }
        consumeOrFail(SEMICOLON);
        
        TreeExpression increment = 0;
        if (!match(CLOSEPAREN)) {
            increment = parseExpression(context);
            failIfFalse(increment);
        }
        int endLine = tokenLine();
        consumeOrFail(CLOSEPAREN);
        const Identifier* unused = 0;
        startLoop();
        TreeStatement statement = parseStatement(context, unused);
        endLoop();
        failIfFalse(statement);
        return context.createForLoop(location, decls, condition, increment, statement, startLine, endLine);
    }
    
    // For-in loop
    failIfFalse(nonLHSCount == m_nonLHSCount);
    consumeOrFail(INTOKEN);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    JSTextPosition exprEnd = lastTokenEndPosition();
    int endLine = tokenLine();
    consumeOrFail(CLOSEPAREN);
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement);
    
    return context.createForInLoop(location, decls, expr, statement, declsStart, declsEnd, exprEnd, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseBreakStatement(TreeBuilder& context)
{
    ASSERT(match(BREAK));
    JSTokenLocation location(tokenLocation());
    JSTextPosition start = tokenStartPosition();
    JSTextPosition end = tokenEndPosition();
    next();
    
    if (autoSemiColon()) {
        failIfFalseWithMessage(breakIsValid(), "'break' is only valid inside a switch or loop statement");
        return context.createBreakStatement(location, start, end);
    }
    matchOrFail(IDENT);
    const Identifier* ident = m_token.m_data.ident;
    failIfFalseWithNameAndMessage(getLabel(ident), "Label", ident->impl(), "is not defined");
    end = tokenEndPosition();
    next();
    failIfFalse(autoSemiColon());
    return context.createBreakStatement(location, ident, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseContinueStatement(TreeBuilder& context)
{
    ASSERT(match(CONTINUE));
    JSTokenLocation location(tokenLocation());
    JSTextPosition start = tokenStartPosition();
    JSTextPosition end = tokenEndPosition();
    next();
    
    if (autoSemiColon()) {
        failIfFalseWithMessage(continueIsValid(), "'continue' is only valid inside a loop statement");
        return context.createContinueStatement(location, start, end);
    }
    matchOrFail(IDENT);
    const Identifier* ident = m_token.m_data.ident;
    ScopeLabelInfo* label = getLabel(ident);
    failIfFalseWithNameAndMessage(label, "Label", ident->impl(), "is not defined");
    failIfFalseWithMessage(label->m_isLoop, "'continue' is only valid inside a loop statement");
    end = tokenEndPosition();
    next();
    failIfFalse(autoSemiColon());
    return context.createContinueStatement(location, ident, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseReturnStatement(TreeBuilder& context)
{
    ASSERT(match(RETURN));
    JSTokenLocation location(tokenLocation());
    failIfFalse(currentScope()->isFunction());
    JSTextPosition start = tokenStartPosition();
    JSTextPosition end = tokenEndPosition();
    next();
    // We do the auto semicolon check before attempting to parse an expression
    // as we need to ensure the a line break after the return correctly terminates
    // the statement
    if (match(SEMICOLON))
        end = tokenEndPosition();

    if (autoSemiColon())
        return context.createReturnStatement(location, 0, start, end);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    end = lastTokenEndPosition();
    if (match(SEMICOLON))
        end  = tokenEndPosition();
    failIfFalse(autoSemiColon());
    return context.createReturnStatement(location, expr, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseThrowStatement(TreeBuilder& context)
{
    ASSERT(match(THROW));
    JSTokenLocation location(tokenLocation());
    JSTextPosition start = tokenStartPosition();
    next();
    
    failIfTrue(autoSemiColon());
    
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    JSTextPosition end = lastTokenEndPosition();
    failIfFalse(autoSemiColon());
    
    return context.createThrowStatement(location, expr, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseWithStatement(TreeBuilder& context)
{
    ASSERT(match(WITH));
    JSTokenLocation location(tokenLocation());
    failIfTrueWithMessage(strictMode(), "'with' statements are not valid in strict mode");
    currentScope()->setNeedsFullActivation();
    int startLine = tokenLine();
    next();
    consumeOrFail(OPENPAREN);
    int start = tokenStart();
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    JSTextPosition end = lastTokenEndPosition();
    int endLine = tokenLine();
    consumeOrFail(CLOSEPAREN);
    const Identifier* unused = 0;
    TreeStatement statement = parseStatement(context, unused);
    failIfFalse(statement);
    
    return context.createWithStatement(location, expr, statement, start, end, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseSwitchStatement(TreeBuilder& context)
{
    ASSERT(match(SWITCH));
    JSTokenLocation location(tokenLocation());
    int startLine = tokenLine();
    next();
    consumeOrFail(OPENPAREN);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr);
    int endLine = tokenLine();
    consumeOrFail(CLOSEPAREN);
    consumeOrFail(OPENBRACE);
    startSwitch();
    TreeClauseList firstClauses = parseSwitchClauses(context);
    failIfTrue(hasError());
    
    TreeClause defaultClause = parseSwitchDefaultClause(context);
    failIfTrue(hasError());
    
    TreeClauseList secondClauses = parseSwitchClauses(context);
    failIfTrue(hasError());
    endSwitch();
    consumeOrFail(CLOSEBRACE);
    
    return context.createSwitchStatement(location, expr, firstClauses, defaultClause, secondClauses, startLine, endLine);
    
}

template <typename LexerType>
template <class TreeBuilder> TreeClauseList Parser<LexerType>::parseSwitchClauses(TreeBuilder& context)
{
    if (!match(CASE))
        return 0;
    next();
    TreeExpression condition = parseExpression(context);
    failIfFalse(condition);
    consumeOrFail(COLON);
    TreeSourceElements statements = parseSourceElements<DontCheckForStrictMode>(context);
    failIfFalse(statements);
    TreeClause clause = context.createClause(condition, statements);
    TreeClauseList clauseList = context.createClauseList(clause);
    TreeClauseList tail = clauseList;
    
    while (match(CASE)) {
        next();
        TreeExpression condition = parseExpression(context);
        failIfFalse(condition);
        consumeOrFail(COLON);
        TreeSourceElements statements = parseSourceElements<DontCheckForStrictMode>(context);
        failIfFalse(statements);
        clause = context.createClause(condition, statements);
        tail = context.createClauseList(tail, clause);
    }
    return clauseList;
}

template <typename LexerType>
template <class TreeBuilder> TreeClause Parser<LexerType>::parseSwitchDefaultClause(TreeBuilder& context)
{
    if (!match(DEFAULT))
        return 0;
    next();
    consumeOrFail(COLON);
    TreeSourceElements statements = parseSourceElements<DontCheckForStrictMode>(context);
    failIfFalse(statements);
    return context.createClause(0, statements);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseTryStatement(TreeBuilder& context)
{
    ASSERT(match(TRY));
    JSTokenLocation location(tokenLocation());
    TreeStatement tryBlock = 0;
    const Identifier* ident = &m_vm->propertyNames->nullIdentifier;
    TreeStatement catchBlock = 0;
    TreeStatement finallyBlock = 0;
    int firstLine = tokenLine();
    next();
    matchOrFail(OPENBRACE);
    
    tryBlock = parseBlockStatement(context);
    failIfFalse(tryBlock);
    int lastLine = m_lastTokenEndPosition.line;
    
    if (match(CATCH)) {
        currentScope()->setNeedsFullActivation();
        next();
        consumeOrFail(OPENPAREN);
        matchOrFail(IDENT);
        ident = m_token.m_data.ident;
        next();
        AutoPopScopeRef catchScope(this, pushScope());
        failIfFalseIfStrictWithNameAndMessage(declareVariable(ident), "Cannot declare a variable named", ident->impl(), "in strict mode");
        catchScope->preventNewDecls();
        consumeOrFail(CLOSEPAREN);
        matchOrFail(OPENBRACE);
        catchBlock = parseBlockStatement(context);
        failIfFalseWithMessage(catchBlock, "'try' must have a catch or finally block");
        failIfFalse(popScope(catchScope, TreeBuilder::NeedsFreeVariableInfo));
    }
    
    if (match(FINALLY)) {
        next();
        matchOrFail(OPENBRACE);
        finallyBlock = parseBlockStatement(context);
        failIfFalse(finallyBlock);
    }
    failIfFalse(catchBlock || finallyBlock);
    return context.createTryStatement(location, tryBlock, ident, catchBlock, finallyBlock, firstLine, lastLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseDebuggerStatement(TreeBuilder& context)
{
    ASSERT(match(DEBUGGER));
    JSTokenLocation location(tokenLocation());
    int startLine = tokenLine();
    int endLine = startLine;
    next();
    if (match(SEMICOLON))
        startLine = tokenLine();
    failIfFalse(autoSemiColon());
    return context.createDebugger(location, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseBlockStatement(TreeBuilder& context)
{
    ASSERT(match(OPENBRACE));
    JSTokenLocation location(tokenLocation());
    int start = tokenLine();
    next();
    if (match(CLOSEBRACE)) {
        next();
        return context.createBlockStatement(location, 0, start, m_lastTokenEndPosition.line);
    }
    TreeSourceElements subtree = parseSourceElements<DontCheckForStrictMode>(context);
    failIfFalse(subtree);
    matchOrFail(CLOSEBRACE);
    next();
    return context.createBlockStatement(location, subtree, start, m_lastTokenEndPosition.line);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseStatement(TreeBuilder& context, const Identifier*& directive, unsigned* directiveLiteralLength)
{
    DepthManager statementDepth(&m_statementDepth);
    m_statementDepth++;
    directive = 0;
    int nonTrivialExpressionCount = 0;
    failIfStackOverflow();
    switch (m_token.m_type) {
    case OPENBRACE:
        return parseBlockStatement(context);
    case VAR:
        return parseVarDeclaration(context);
    case CONSTTOKEN:
        return parseConstDeclaration(context);
    case FUNCTION:
        failIfFalseIfStrictWithMessage(m_statementDepth == 1, "Functions cannot be declared in a nested block in strict mode");
        return parseFunctionDeclaration(context);
    case SEMICOLON: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.createEmptyStatement(location);
    }
    case IF:
        return parseIfStatement(context);
    case DO:
        return parseDoWhileStatement(context);
    case WHILE:
        return parseWhileStatement(context);
    case FOR:
        return parseForStatement(context);
    case CONTINUE:
        return parseContinueStatement(context);
    case BREAK:
        return parseBreakStatement(context);
    case RETURN:
        return parseReturnStatement(context);
    case WITH:
        return parseWithStatement(context);
    case SWITCH:
        return parseSwitchStatement(context);
    case THROW:
        return parseThrowStatement(context);
    case TRY:
        return parseTryStatement(context);
    case DEBUGGER:
        return parseDebuggerStatement(context);
    case EOFTOK:
    case CASE:
    case CLOSEBRACE:
    case DEFAULT:
        // These tokens imply the end of a set of source elements
        return 0;
    case IDENT:
        return parseExpressionOrLabelStatement(context);
    case STRING:
        directive = m_token.m_data.ident;
        if (directiveLiteralLength)
            *directiveLiteralLength = m_token.m_location.endOffset - m_token.m_location.startOffset;
        nonTrivialExpressionCount = m_nonTrivialExpressionCount;
    default:
        TreeStatement exprStatement = parseExpressionStatement(context);
        if (directive && nonTrivialExpressionCount != m_nonTrivialExpressionCount)
            directive = 0;
        return exprStatement;
    }
}

template <typename LexerType>
template <class TreeBuilder> TreeFormalParameterList Parser<LexerType>::parseFormalParameters(TreeBuilder& context)
{
    matchOrFail(IDENT);
    failIfFalseIfStrictWithNameAndMessage(declareParameter(m_token.m_data.ident), "Cannot declare a parameter named", m_token.m_data.ident->impl(), " in strict mode");
    TreeFormalParameterList list = context.createFormalParameterList(*m_token.m_data.ident);
    TreeFormalParameterList tail = list;
    next();
    while (match(COMMA)) {
        next();
        matchOrFail(IDENT);
        const Identifier* ident = m_token.m_data.ident;
        failIfFalseIfStrictWithNameAndMessage(declareParameter(ident), "Cannot declare a parameter named", ident->impl(), "in strict mode");
        next();
        tail = context.createFormalParameterList(tail, *ident);
    }
    return list;
}

template <typename LexerType>
template <class TreeBuilder> TreeFunctionBody Parser<LexerType>::parseFunctionBody(TreeBuilder& context)
{
    JSTokenLocation startLocation(tokenLocation());
    unsigned startColumn = tokenColumn();
    next();

    if (match(CLOSEBRACE))
        return context.createFunctionBody(startLocation, tokenLocation(), startColumn, strictMode());
    DepthManager statementDepth(&m_statementDepth);
    m_statementDepth = 0;
    typename TreeBuilder::FunctionBodyBuilder bodyBuilder(const_cast<VM*>(m_vm), m_lexer.get());
    failIfFalse(parseSourceElements<CheckForStrictMode>(bodyBuilder));
    return context.createFunctionBody(startLocation, tokenLocation(), startColumn, strictMode());
}

template <typename LexerType>
template <FunctionRequirements requirements, bool nameIsInContainingScope, class TreeBuilder> bool Parser<LexerType>::parseFunctionInfo(TreeBuilder& context, const Identifier*& name, TreeFormalParameterList& parameters, TreeFunctionBody& body, unsigned& openBraceOffset, unsigned& closeBraceOffset, int& bodyStartLine, unsigned& bodyStartColumn)
{
    AutoPopScopeRef functionScope(this, pushScope());
    functionScope->setIsFunction();
    int functionStart = m_token.m_location.startOffset;
    if (match(IDENT)) {
        name = m_token.m_data.ident;
        next();
        if (!nameIsInContainingScope)
            failIfFalseIfStrict(functionScope->declareVariable(name));
    } else if (requirements == FunctionNeedsName)
        return false;
    consumeOrFail(OPENPAREN);
    if (!match(CLOSEPAREN)) {
        parameters = parseFormalParameters(context);
        failIfFalse(parameters);
    }
    consumeOrFail(CLOSEPAREN);
    matchOrFail(OPENBRACE);
    
    openBraceOffset = m_token.m_data.offset;
    bodyStartLine = tokenLine();
    bodyStartColumn = m_token.m_data.offset - m_token.m_data.lineStartOffset;
    JSTokenLocation startLocation(tokenLocation());
    
    // If we know about this function already, we can use the cached info and skip the parser to the end of the function.
    if (const SourceProviderCacheItem* cachedInfo = TreeBuilder::CanUseFunctionCache ? findCachedFunctionInfo(openBraceOffset) : 0) {
        // If we're in a strict context, the cached function info must say it was strict too.
        ASSERT(!strictMode() || cachedInfo->strictMode);
        JSTokenLocation endLocation;

        endLocation.line = cachedInfo->closeBraceLine;
        endLocation.startOffset = cachedInfo->closeBraceOffset;
        endLocation.lineStartOffset = cachedInfo->closeBraceLineStartOffset;
        ASSERT(endLocation.startOffset >= endLocation.lineStartOffset);

        body = context.createFunctionBody(startLocation, endLocation, bodyStartColumn, cachedInfo->strictMode);
        
        functionScope->restoreFromSourceProviderCache(cachedInfo);
        failIfFalse(popScope(functionScope, TreeBuilder::NeedsFreeVariableInfo));
        
        closeBraceOffset = cachedInfo->closeBraceOffset;

        context.setFunctionStart(body, functionStart);
        m_token = cachedInfo->closeBraceToken();

        m_lexer->setOffset(m_token.m_location.endOffset, m_token.m_location.lineStartOffset);
        m_lexer->setLineNumber(m_token.m_location.line);
        
        next();
        return true;
    }
    
    body = parseFunctionBody(context);
    failIfFalse(body);
    if (functionScope->strictMode() && name) {
        failIfTrueWithNameAndMessage(m_vm->propertyNames->arguments == *name, "Function name", name->impl(), "is not valid in strict mode");
        failIfTrueWithNameAndMessage(m_vm->propertyNames->eval == *name, "Function name", name->impl(), "is not valid in strict mode");
    }
    closeBraceOffset = m_token.m_data.offset;
    unsigned closeBraceLine = m_token.m_data.line;
    unsigned closeBraceLineStartOffset = m_token.m_data.lineStartOffset;
    
    // Cache the tokenizer state and the function scope the first time the function is parsed.
    // Any future reparsing can then skip the function.
    static const int minimumFunctionLengthToCache = 16;
    OwnPtr<SourceProviderCacheItem> newInfo;
    int functionLength = closeBraceOffset - openBraceOffset;
    if (TreeBuilder::CanUseFunctionCache && m_functionCache && functionLength > minimumFunctionLengthToCache) {
        SourceProviderCacheItemCreationParameters parameters;
        parameters.functionStart = functionStart;
        parameters.closeBraceLine = closeBraceLine;
        parameters.closeBraceOffset = closeBraceOffset;
        parameters.closeBraceLineStartOffset = closeBraceLineStartOffset;
        functionScope->fillParametersForSourceProviderCache(parameters);
        newInfo = SourceProviderCacheItem::create(parameters);

    }
    context.setFunctionStart(body, functionStart);
    
    failIfFalse(popScope(functionScope, TreeBuilder::NeedsFreeVariableInfo));
    matchOrFail(CLOSEBRACE);
    
    if (newInfo)
        m_functionCache->add(openBraceOffset, newInfo.release());
    
    next();
    return true;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseFunctionDeclaration(TreeBuilder& context)
{
    ASSERT(match(FUNCTION));
    JSTokenLocation location(tokenLocation());
    next();
    const Identifier* name = 0;
    TreeFormalParameterList parameters = 0;
    TreeFunctionBody body = 0;
    unsigned openBraceOffset = 0;
    unsigned closeBraceOffset = 0;
    int bodyStartLine = 0;
    unsigned bodyStartColumn = 0;
    failIfFalse((parseFunctionInfo<FunctionNeedsName, true>(context, name, parameters, body, openBraceOffset, closeBraceOffset, bodyStartLine, bodyStartColumn)));
    failIfFalse(name);
    failIfFalseIfStrict(declareVariable(name));
    return context.createFuncDeclStatement(location, name, body, parameters, openBraceOffset, closeBraceOffset, bodyStartLine, m_lastTokenEndPosition.line, bodyStartColumn);
}

struct LabelInfo {
    LabelInfo(const Identifier* ident, const JSTextPosition& start, const JSTextPosition& end)
    : m_ident(ident)
    , m_start(start)
    , m_end(end)
    {
    }
    
    const Identifier* m_ident;
    JSTextPosition m_start;
    JSTextPosition m_end;
};

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseExpressionOrLabelStatement(TreeBuilder& context)
{
    
    /* Expression and Label statements are ambiguous at LL(1), so we have a
     * special case that looks for a colon as the next character in the input.
     */
    Vector<LabelInfo> labels;
    JSTokenLocation location;
    do {
        JSTextPosition start = tokenStartPosition();
        location = tokenLocation();
        if (!nextTokenIsColon()) {
            // If we hit this path we're making a expression statement, which
            // by definition can't make use of continue/break so we can just
            // ignore any labels we might have accumulated.
            TreeExpression expression = parseExpression(context);
            failIfFalse(expression);
            failIfFalse(autoSemiColon());
            return context.createExprStatement(location, expression, start, m_lastTokenEndPosition.line);
        }
        const Identifier* ident = m_token.m_data.ident;
        JSTextPosition end = tokenEndPosition();
        next();
        consumeOrFail(COLON);
        if (!m_syntaxAlreadyValidated) {
            // This is O(N^2) over the current list of consecutive labels, but I
            // have never seen more than one label in a row in the real world.
            for (size_t i = 0; i < labels.size(); i++)
                failIfTrue(ident->impl() == labels[i].m_ident->impl());
            failIfTrue(getLabel(ident));
            labels.append(LabelInfo(ident, start, end));
        }
    } while (match(IDENT));
    bool isLoop = false;
    switch (m_token.m_type) {
    case FOR:
    case WHILE:
    case DO:
        isLoop = true;
        break;
        
    default:
        break;
    }
    const Identifier* unused = 0;
    if (!m_syntaxAlreadyValidated) {
        for (size_t i = 0; i < labels.size(); i++)
            pushLabel(labels[i].m_ident, isLoop);
    }
    TreeStatement statement = parseStatement(context, unused);
    if (!m_syntaxAlreadyValidated) {
        for (size_t i = 0; i < labels.size(); i++)
            popLabel();
    }
    failIfFalse(statement);
    for (size_t i = 0; i < labels.size(); i++) {
        const LabelInfo& info = labels[labels.size() - i - 1];
        statement = context.createLabelStatement(location, info.m_ident, statement, info.m_start, info.m_end);
    }
    return statement;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseExpressionStatement(TreeBuilder& context)
{
    JSTextPosition start = tokenStartPosition();
    JSTokenLocation location(tokenLocation());
    TreeExpression expression = parseExpression(context);
    failIfFalse(expression);
    failIfFalse(autoSemiColon());
    return context.createExprStatement(location, expression, start, m_lastTokenEndPosition.line);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseIfStatement(TreeBuilder& context)
{
    ASSERT(match(IF));
    JSTokenLocation ifLocation(tokenLocation());
    int start = tokenLine();
    next();

    consumeOrFail(OPENPAREN);

    TreeExpression condition = parseExpression(context);
    failIfFalse(condition);
    int end = tokenLine();
    consumeOrFail(CLOSEPAREN);

    const Identifier* unused = 0;
    TreeStatement trueBlock = parseStatement(context, unused);
    failIfFalse(trueBlock);

    if (!match(ELSE))
        return context.createIfStatement(ifLocation, condition, trueBlock, 0, start, end);

    Vector<TreeExpression> exprStack;
    Vector<pair<int, int> > posStack;
    Vector<JSTokenLocation> tokenLocationStack;
    Vector<TreeStatement> statementStack;
    bool trailingElse = false;
    do {
        JSTokenLocation tempLocation = tokenLocation();
        next();
        if (!match(IF)) {
            const Identifier* unused = 0;
            TreeStatement block = parseStatement(context, unused);
            failIfFalse(block);
            statementStack.append(block);
            trailingElse = true;
            break;
        }
        int innerStart = tokenLine();
        next();

        consumeOrFail(OPENPAREN);

        TreeExpression innerCondition = parseExpression(context);
        failIfFalse(innerCondition);
        int innerEnd = tokenLine();
        consumeOrFail(CLOSEPAREN);
        const Identifier* unused = 0;
        TreeStatement innerTrueBlock = parseStatement(context, unused);
        failIfFalse(innerTrueBlock);
        tokenLocationStack.append(tempLocation);
        exprStack.append(innerCondition);
        posStack.append(make_pair(innerStart, innerEnd));
        statementStack.append(innerTrueBlock);
    } while (match(ELSE));

    if (!trailingElse) {
        TreeExpression condition = exprStack.last();
        exprStack.removeLast();
        TreeStatement trueBlock = statementStack.last();
        statementStack.removeLast();
        pair<int, int> pos = posStack.last();
        posStack.removeLast();
        JSTokenLocation elseLocation = tokenLocationStack.last();
        tokenLocationStack.removeLast();
        statementStack.append(context.createIfStatement(elseLocation, condition, trueBlock, 0, pos.first, pos.second));
    }

    while (!exprStack.isEmpty()) {
        TreeExpression condition = exprStack.last();
        exprStack.removeLast();
        TreeStatement falseBlock = statementStack.last();
        statementStack.removeLast();
        TreeStatement trueBlock = statementStack.last();
        statementStack.removeLast();
        pair<int, int> pos = posStack.last();
        posStack.removeLast();
        JSTokenLocation elseLocation = tokenLocationStack.last();
        tokenLocationStack.removeLast();
        statementStack.append(context.createIfStatement(elseLocation, condition, trueBlock, falseBlock, pos.first, pos.second));
    }

    return context.createIfStatement(ifLocation, condition, trueBlock, statementStack.last(), start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseExpression(TreeBuilder& context)
{
    failIfStackOverflow();
    JSTokenLocation location(tokenLocation());
    TreeExpression node = parseAssignmentExpression(context);
    failIfFalse(node);
    if (!match(COMMA))
        return node;
    next();
    m_nonTrivialExpressionCount++;
    m_nonLHSCount++;
    TreeExpression right = parseAssignmentExpression(context);
    failIfFalse(right);
    typename TreeBuilder::Comma commaNode = context.createCommaExpr(location, node, right);
    while (match(COMMA)) {
        next(TreeBuilder::DontBuildStrings);
        right = parseAssignmentExpression(context);
        failIfFalse(right);
        context.appendToComma(commaNode, right);
    }
    return commaNode;
}

template <typename LexerType>
template <typename TreeBuilder> TreeExpression Parser<LexerType>::parseAssignmentExpression(TreeBuilder& context)
{
    failIfStackOverflow();
    JSTextPosition start = tokenStartPosition();
    JSTokenLocation location(tokenLocation());
    int initialAssignmentCount = m_assignmentCount;
    int initialNonLHSCount = m_nonLHSCount;
    TreeExpression lhs = parseConditionalExpression(context);
    failIfFalse(lhs);
    if (initialNonLHSCount != m_nonLHSCount)
        return lhs;
    
    int assignmentStack = 0;
    Operator op;
    bool hadAssignment = false;
    while (true) {
        switch (m_token.m_type) {
        case EQUAL: op = OpEqual; break;
        case PLUSEQUAL: op = OpPlusEq; break;
        case MINUSEQUAL: op = OpMinusEq; break;
        case MULTEQUAL: op = OpMultEq; break;
        case DIVEQUAL: op = OpDivEq; break;
        case LSHIFTEQUAL: op = OpLShift; break;
        case RSHIFTEQUAL: op = OpRShift; break;
        case URSHIFTEQUAL: op = OpURShift; break;
        case ANDEQUAL: op = OpAndEq; break;
        case XOREQUAL: op = OpXOrEq; break;
        case OREQUAL: op = OpOrEq; break;
        case MODEQUAL: op = OpModEq; break;
        default:
            goto end;
        }
        m_nonTrivialExpressionCount++;
        hadAssignment = true;
        context.assignmentStackAppend(assignmentStack, lhs, start, tokenStartPosition(), m_assignmentCount, op);
        start = tokenStartPosition();
        m_assignmentCount++;
        next(TreeBuilder::DontBuildStrings);
        if (strictMode() && m_lastIdentifier && context.isResolve(lhs)) {
            failIfTrueIfStrictWithMessage(m_vm->propertyNames->eval == *m_lastIdentifier, "'eval' cannot be modified in strict mode");
            failIfTrueIfStrictWithMessage(m_vm->propertyNames->arguments == *m_lastIdentifier, "'arguments' cannot be modified in strict mode");
            declareWrite(m_lastIdentifier);
            m_lastIdentifier = 0;
        }
        lhs = parseConditionalExpression(context);
        failIfFalse(lhs);
        if (initialNonLHSCount != m_nonLHSCount)
            break;
    }
end:
    if (hadAssignment)
        m_nonLHSCount++;
    
    if (!TreeBuilder::CreatesAST)
        return lhs;
    
    while (assignmentStack)
        lhs = context.createAssignment(location, assignmentStack, lhs, initialAssignmentCount, m_assignmentCount, lastTokenEndPosition());
    
    return lhs;
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseConditionalExpression(TreeBuilder& context)
{
    JSTokenLocation location(tokenLocation());
    TreeExpression cond = parseBinaryExpression(context);
    failIfFalse(cond);
    if (!match(QUESTION))
        return cond;
    m_nonTrivialExpressionCount++;
    m_nonLHSCount++;
    next(TreeBuilder::DontBuildStrings);
    TreeExpression lhs = parseAssignmentExpression(context);
    consumeOrFailWithFlags(COLON, TreeBuilder::DontBuildStrings);
    
    TreeExpression rhs = parseAssignmentExpression(context);
    failIfFalse(rhs);
    return context.createConditionalExpr(location, cond, lhs, rhs);
}

ALWAYS_INLINE static bool isUnaryOp(JSTokenType token)
{
    return token & UnaryOpTokenFlag;
}

template <typename LexerType>
int Parser<LexerType>::isBinaryOperator(JSTokenType token)
{
    if (m_allowsIn)
        return token & (BinaryOpTokenPrecedenceMask << BinaryOpTokenAllowsInPrecedenceAdditionalShift);
    return token & BinaryOpTokenPrecedenceMask;
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseBinaryExpression(TreeBuilder& context)
{
    
    int operandStackDepth = 0;
    int operatorStackDepth = 0;
    typename TreeBuilder::BinaryExprContext binaryExprContext(context);
    JSTokenLocation location(tokenLocation());
    while (true) {
        JSTextPosition exprStart = tokenStartPosition();
        int initialAssignments = m_assignmentCount;
        TreeExpression current = parseUnaryExpression(context);
        failIfFalse(current);
        
        context.appendBinaryExpressionInfo(operandStackDepth, current, exprStart, lastTokenEndPosition(), lastTokenEndPosition(), initialAssignments != m_assignmentCount);
        int precedence = isBinaryOperator(m_token.m_type);
        if (!precedence)
            break;
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        int operatorToken = m_token.m_type;
        next(TreeBuilder::DontBuildStrings);
        
        while (operatorStackDepth &&  context.operatorStackHasHigherPrecedence(operatorStackDepth, precedence)) {
            ASSERT(operandStackDepth > 1);
            
            typename TreeBuilder::BinaryOperand rhs = context.getFromOperandStack(-1);
            typename TreeBuilder::BinaryOperand lhs = context.getFromOperandStack(-2);
            context.shrinkOperandStackBy(operandStackDepth, 2);
            context.appendBinaryOperation(location, operandStackDepth, operatorStackDepth, lhs, rhs);
            context.operatorStackPop(operatorStackDepth);
        }
        context.operatorStackAppend(operatorStackDepth, operatorToken, precedence);
    }
    while (operatorStackDepth) {
        ASSERT(operandStackDepth > 1);
        
        typename TreeBuilder::BinaryOperand rhs = context.getFromOperandStack(-1);
        typename TreeBuilder::BinaryOperand lhs = context.getFromOperandStack(-2);
        context.shrinkOperandStackBy(operandStackDepth, 2);
        context.appendBinaryOperation(location, operandStackDepth, operatorStackDepth, lhs, rhs);
        context.operatorStackPop(operatorStackDepth);
    }
    return context.popOperandStack(operandStackDepth);
}

template <typename LexerType>
template <bool complete, class TreeBuilder> TreeProperty Parser<LexerType>::parseProperty(TreeBuilder& context)
{
    bool wasIdent = false;
    switch (m_token.m_type) {
    namedProperty:
    case IDENT:
        wasIdent = true;
    case STRING: {
        const Identifier* ident = m_token.m_data.ident;
        if (complete || (wasIdent && (*ident == m_vm->propertyNames->get || *ident == m_vm->propertyNames->set)))
            nextExpectIdentifier(LexerFlagsIgnoreReservedWords);
        else
            nextExpectIdentifier(LexerFlagsIgnoreReservedWords | TreeBuilder::DontBuildKeywords);
        
        if (match(COLON)) {
            next();
            TreeExpression node = parseAssignmentExpression(context);
            failIfFalse(node);
            return context.template createProperty<complete>(ident, node, PropertyNode::Constant);
        }
        failIfFalse(wasIdent);
        const Identifier* accessorName = 0;
        TreeFormalParameterList parameters = 0;
        TreeFunctionBody body = 0;
        unsigned openBraceOffset = 0;
        unsigned closeBraceOffset = 0;
        int bodyStartLine = 0;
        unsigned bodyStartColumn = 0;
        PropertyNode::Type type;
        if (*ident == m_vm->propertyNames->get)
            type = PropertyNode::Getter;
        else if (*ident == m_vm->propertyNames->set)
            type = PropertyNode::Setter;
        else
            fail();
        const Identifier* stringPropertyName = 0;
        double numericPropertyName = 0;
        if (m_token.m_type == IDENT || m_token.m_type == STRING)
            stringPropertyName = m_token.m_data.ident;
        else if (m_token.m_type == NUMBER)
            numericPropertyName = m_token.m_data.doubleValue;
        else
            fail();
        JSTokenLocation location(tokenLocation());
        next();
        failIfFalse((parseFunctionInfo<FunctionNoRequirements, false>(context, accessorName, parameters, body, openBraceOffset, closeBraceOffset, bodyStartLine, bodyStartColumn)));
        if (stringPropertyName)
            return context.template createGetterOrSetterProperty<complete>(location, type, stringPropertyName, parameters, body, openBraceOffset, closeBraceOffset, bodyStartLine, m_lastTokenEndPosition.line, bodyStartColumn);
        return context.template createGetterOrSetterProperty<complete>(const_cast<VM*>(m_vm), location, type, numericPropertyName, parameters, body, openBraceOffset, closeBraceOffset, bodyStartLine, m_lastTokenEndPosition.line, bodyStartColumn);
    }
    case NUMBER: {
        double propertyName = m_token.m_data.doubleValue;
        next();
        consumeOrFail(COLON);
        TreeExpression node = parseAssignmentExpression(context);
        failIfFalse(node);
        return context.template createProperty<complete>(const_cast<VM*>(m_vm), propertyName, node, PropertyNode::Constant);
    }
    default:
        failIfFalse(m_token.m_type & KeywordTokenFlag);
        goto namedProperty;
    }
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseObjectLiteral(TreeBuilder& context)
{
    int startOffset = m_token.m_data.offset;
    unsigned oldLineStartOffset = m_lexer->currentLineStartOffset();
    unsigned oldLastLineNumber = m_lexer->lastLineNumber();
    unsigned oldLineNumber = m_lexer->lineNumber();
    consumeOrFailWithFlags(OPENBRACE, TreeBuilder::DontBuildStrings);
    JSTokenLocation location(tokenLocation());

    int oldNonLHSCount = m_nonLHSCount;
    
    if (match(CLOSEBRACE)) {
        next();
        return context.createObjectLiteral(location);
    }
    
    TreeProperty property = parseProperty<false>(context);
    failIfFalse(property);
    if (!m_syntaxAlreadyValidated && context.getType(property) != PropertyNode::Constant) {
        m_lexer->setOffset(startOffset, oldLineStartOffset);
        next();
        m_lexer->setLastLineNumber(oldLastLineNumber);
        m_lexer->setLineNumber(oldLineNumber);
        return parseStrictObjectLiteral(context);
    }
    TreePropertyList propertyList = context.createPropertyList(location, property);
    TreePropertyList tail = propertyList;
    while (match(COMMA)) {
        next(TreeBuilder::DontBuildStrings);
        // allow extra comma, see http://bugs.webkit.org/show_bug.cgi?id=5939
        if (match(CLOSEBRACE))
            break;
        JSTokenLocation propertyLocation(tokenLocation());
        property = parseProperty<false>(context);
        failIfFalse(property);
        if (!m_syntaxAlreadyValidated && context.getType(property) != PropertyNode::Constant) {
            m_lexer->setOffset(startOffset, oldLineStartOffset);
            next();
            m_lexer->setLastLineNumber(oldLastLineNumber);
            m_lexer->setLineNumber(oldLineNumber);
            return parseStrictObjectLiteral(context);
        }
        tail = context.createPropertyList(propertyLocation, property, tail);
    }

    location = tokenLocation();
    consumeOrFail(CLOSEBRACE);
    
    m_nonLHSCount = oldNonLHSCount;
    
    return context.createObjectLiteral(location, propertyList);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseStrictObjectLiteral(TreeBuilder& context)
{
    consumeOrFail(OPENBRACE);
    
    int oldNonLHSCount = m_nonLHSCount;

    JSTokenLocation location(tokenLocation());
    if (match(CLOSEBRACE)) {
        next();
        return context.createObjectLiteral(location);
    }
    
    TreeProperty property = parseProperty<true>(context);
    failIfFalse(property);
    
    typedef HashMap<RefPtr<StringImpl>, unsigned, IdentifierRepHash> ObjectValidationMap;
    ObjectValidationMap objectValidator;
    // Add the first property
    if (!m_syntaxAlreadyValidated)
        objectValidator.add(context.getName(property).impl(), context.getType(property));
    
    TreePropertyList propertyList = context.createPropertyList(location, property);
    TreePropertyList tail = propertyList;
    while (match(COMMA)) {
        next();
        // allow extra comma, see http://bugs.webkit.org/show_bug.cgi?id=5939
        if (match(CLOSEBRACE))
            break;
        JSTokenLocation propertyLocation(tokenLocation());
        property = parseProperty<true>(context);
        failIfFalse(property);
        if (!m_syntaxAlreadyValidated) {
            ObjectValidationMap::AddResult propertyEntry = objectValidator.add(context.getName(property).impl(), context.getType(property));
            if (!propertyEntry.isNewEntry) {
                failIfTrue(propertyEntry.iterator->value == PropertyNode::Constant);
                failIfTrue(context.getType(property) == PropertyNode::Constant);
                failIfTrue(context.getType(property) & propertyEntry.iterator->value);
                propertyEntry.iterator->value |= context.getType(property);
            }
        }
        tail = context.createPropertyList(propertyLocation, property, tail);
    }

    location = tokenLocation();
    consumeOrFail(CLOSEBRACE);

    m_nonLHSCount = oldNonLHSCount;

    return context.createObjectLiteral(location, propertyList);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseArrayLiteral(TreeBuilder& context)
{
    consumeOrFailWithFlags(OPENBRACKET, TreeBuilder::DontBuildStrings);
    
    int oldNonLHSCount = m_nonLHSCount;
    
    int elisions = 0;
    while (match(COMMA)) {
        next(TreeBuilder::DontBuildStrings);
        elisions++;
    }
    if (match(CLOSEBRACKET)) {
        JSTokenLocation location(tokenLocation());
        next(TreeBuilder::DontBuildStrings);
        return context.createArray(location, elisions);
    }
    
    TreeExpression elem = parseAssignmentExpression(context);
    failIfFalse(elem);
    typename TreeBuilder::ElementList elementList = context.createElementList(elisions, elem);
    typename TreeBuilder::ElementList tail = elementList;
    elisions = 0;
    while (match(COMMA)) {
        next(TreeBuilder::DontBuildStrings);
        elisions = 0;
        
        while (match(COMMA)) {
            next();
            elisions++;
        }
        
        if (match(CLOSEBRACKET)) {
            JSTokenLocation location(tokenLocation());
            next(TreeBuilder::DontBuildStrings);
            return context.createArray(location, elisions, elementList);
        }
        TreeExpression elem = parseAssignmentExpression(context);
        failIfFalse(elem);
        tail = context.createElementList(tail, elisions, elem);
    }

    JSTokenLocation location(tokenLocation());
    consumeOrFail(CLOSEBRACKET);
    
    m_nonLHSCount = oldNonLHSCount;
    
    return context.createArray(location, elementList);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parsePrimaryExpression(TreeBuilder& context)
{
    failIfStackOverflow();
    switch (m_token.m_type) {
    case OPENBRACE:
        if (strictMode())
            return parseStrictObjectLiteral(context);
        return parseObjectLiteral(context);
    case OPENBRACKET:
        return parseArrayLiteral(context);
    case OPENPAREN: {
        next();
        int oldNonLHSCount = m_nonLHSCount;
        TreeExpression result = parseExpression(context);
        m_nonLHSCount = oldNonLHSCount;
        consumeOrFail(CLOSEPAREN);
        
        return result;
    }
    case THISTOKEN: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.thisExpr(location);
    }
    case IDENT: {
        JSTextPosition start = tokenStartPosition();
        const Identifier* ident = m_token.m_data.ident;
        JSTokenLocation location(tokenLocation());
        next();
        currentScope()->useVariable(ident, m_vm->propertyNames->eval == *ident);
        m_lastIdentifier = ident;
        return context.createResolve(location, ident, start);
    }
    case STRING: {
        const Identifier* ident = m_token.m_data.ident;
        JSTokenLocation location(tokenLocation());
        next();
        return context.createString(location, ident);
    }
    case NUMBER: {
        double d = m_token.m_data.doubleValue;
        JSTokenLocation location(tokenLocation());
        next();
        return context.createNumberExpr(location, d);
    }
    case NULLTOKEN: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.createNull(location);
    }
    case TRUETOKEN: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.createBoolean(location, true);
    }
    case FALSETOKEN: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.createBoolean(location, false);
    }
    case DIVEQUAL:
    case DIVIDE: {
        /* regexp */
        const Identifier* pattern;
        const Identifier* flags;
        if (match(DIVEQUAL))
            failIfFalse(m_lexer->scanRegExp(pattern, flags, '='));
        else
            failIfFalse(m_lexer->scanRegExp(pattern, flags));
        
        JSTextPosition start = tokenStartPosition();
        JSTokenLocation location(tokenLocation());
        next();
        TreeExpression re = context.createRegExp(location, *pattern, *flags, start);
        if (!re) {
            const char* yarrErrorMsg = Yarr::checkSyntax(pattern->string());
            failWithMessage(yarrErrorMsg);
        }
        return re;
    }
    default:
        fail();
    }
}

template <typename LexerType>
template <class TreeBuilder> TreeArguments Parser<LexerType>::parseArguments(TreeBuilder& context)
{
    consumeOrFailWithFlags(OPENPAREN, TreeBuilder::DontBuildStrings);
    JSTokenLocation location(tokenLocation());
    if (match(CLOSEPAREN)) {
        next(TreeBuilder::DontBuildStrings);
        return context.createArguments();
    }
    TreeExpression firstArg = parseAssignmentExpression(context);
    failIfFalse(firstArg);
    
    TreeArgumentsList argList = context.createArgumentsList(location, firstArg);
    TreeArgumentsList tail = argList;
    while (match(COMMA)) {
        JSTokenLocation argumentLocation(tokenLocation());
        next(TreeBuilder::DontBuildStrings);
        TreeExpression arg = parseAssignmentExpression(context);
        failIfFalse(arg);
        tail = context.createArgumentsList(argumentLocation, tail, arg);
    }
    consumeOrFail(CLOSEPAREN);
    return context.createArguments(argList);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseMemberExpression(TreeBuilder& context)
{
    TreeExpression base = 0;
    JSTextPosition expressionStart = tokenStartPosition();
    int newCount = 0;
    JSTokenLocation location;
    while (match(NEW)) {
        next();
        newCount++;
    }
    
    if (match(FUNCTION)) {
        const Identifier* name = &m_vm->propertyNames->nullIdentifier;
        TreeFormalParameterList parameters = 0;
        TreeFunctionBody body = 0;
        unsigned openBraceOffset = 0;
        unsigned closeBraceOffset = 0;
        int bodyStartLine = 0;
        unsigned bodyStartColumn = 0;
        location = tokenLocation();
        next();
        failIfFalse((parseFunctionInfo<FunctionNoRequirements, false>(context, name, parameters, body, openBraceOffset, closeBraceOffset, bodyStartLine, bodyStartColumn)));
        base = context.createFunctionExpr(location, name, body, parameters, openBraceOffset, closeBraceOffset, bodyStartLine, m_lastTokenEndPosition.line, bodyStartColumn);
    } else
        base = parsePrimaryExpression(context);
    
    failIfFalse(base);
    while (true) {
        location = tokenLocation();
        switch (m_token.m_type) {
        case OPENBRACKET: {
            m_nonTrivialExpressionCount++;
            JSTextPosition expressionEnd = lastTokenEndPosition();
            next();
            int nonLHSCount = m_nonLHSCount;
            int initialAssignments = m_assignmentCount;
            TreeExpression property = parseExpression(context);
            failIfFalse(property);
            base = context.createBracketAccess(location, base, property, initialAssignments != m_assignmentCount, expressionStart, expressionEnd, tokenEndPosition());
            consumeOrFail(CLOSEBRACKET);
            m_nonLHSCount = nonLHSCount;
            break;
        }
        case OPENPAREN: {
            m_nonTrivialExpressionCount++;
            int nonLHSCount = m_nonLHSCount;
            if (newCount) {
                newCount--;
                JSTextPosition expressionEnd = lastTokenEndPosition();
                TreeArguments arguments = parseArguments(context);
                failIfFalse(arguments);
                base = context.createNewExpr(location, base, arguments, expressionStart, expressionEnd, lastTokenEndPosition());
            } else {
                JSTextPosition expressionEnd = lastTokenEndPosition();
                TreeArguments arguments = parseArguments(context);
                failIfFalse(arguments);
                base = context.makeFunctionCallNode(location, base, arguments, expressionStart, expressionEnd, lastTokenEndPosition());
            }
            m_nonLHSCount = nonLHSCount;
            break;
        }
        case DOT: {
            m_nonTrivialExpressionCount++;
            JSTextPosition expressionEnd = lastTokenEndPosition();
            nextExpectIdentifier(LexerFlagsIgnoreReservedWords | TreeBuilder::DontBuildKeywords);
            matchOrFail(IDENT);
            base = context.createDotAccess(location, base, m_token.m_data.ident, expressionStart, expressionEnd, tokenEndPosition());
            next();
            break;
        }
        default:
            goto endMemberExpression;
        }
    }
endMemberExpression:
    while (newCount--)
        base = context.createNewExpr(location, base, expressionStart, lastTokenEndPosition());
    return base;
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseUnaryExpression(TreeBuilder& context)
{
    typename TreeBuilder::UnaryExprContext unaryExprContext(context);
    AllowInOverride allowInOverride(this);
    int tokenStackDepth = 0;
    bool modifiesExpr = false;
    bool requiresLExpr = false;
    while (isUnaryOp(m_token.m_type)) {
        if (strictMode()) {
            switch (m_token.m_type) {
            case PLUSPLUS:
            case MINUSMINUS:
            case AUTOPLUSPLUS:
            case AUTOMINUSMINUS:
                failIfTrue(requiresLExpr);
                modifiesExpr = true;
                requiresLExpr = true;
                break;
            case DELETETOKEN:
                failIfTrue(requiresLExpr);
                requiresLExpr = true;
                break;
            default:
                failIfTrue(requiresLExpr);
                break;
            }
        }
        m_nonLHSCount++;
        context.appendUnaryToken(tokenStackDepth, m_token.m_type, tokenStartPosition());
        next();
        m_nonTrivialExpressionCount++;
    }
    JSTextPosition subExprStart = tokenStartPosition();
    ASSERT(subExprStart.offset >= subExprStart.lineStartOffset);
    JSTokenLocation location(tokenLocation());
    TreeExpression expr = parseMemberExpression(context);
    failIfFalse(expr);
    bool isEvalOrArguments = false;
    if (strictMode() && !m_syntaxAlreadyValidated) {
        if (context.isResolve(expr))
            isEvalOrArguments = *m_lastIdentifier == m_vm->propertyNames->eval || *m_lastIdentifier == m_vm->propertyNames->arguments;
    }
    failIfTrueIfStrictWithNameAndMessage(isEvalOrArguments && modifiesExpr, "'", m_lastIdentifier->impl(), "' cannot be modified in strict mode");
    switch (m_token.m_type) {
    case PLUSPLUS:
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        expr = context.makePostfixNode(location, expr, OpPlusPlus, subExprStart, lastTokenEndPosition(), tokenEndPosition());
        m_assignmentCount++;
        failIfTrueIfStrictWithNameAndMessage(isEvalOrArguments, "'", m_lastIdentifier->impl(), "' cannot be modified in strict mode");
        failIfTrueIfStrict(requiresLExpr);
        next();
        break;
    case MINUSMINUS:
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        expr = context.makePostfixNode(location, expr, OpMinusMinus, subExprStart, lastTokenEndPosition(), tokenEndPosition());
        m_assignmentCount++;
        failIfTrueIfStrictWithNameAndMessage(isEvalOrArguments, "'", m_lastIdentifier->impl(), "' cannot be modified in strict mode");
        failIfTrueIfStrict(requiresLExpr);
        next();
        break;
    default:
        break;
    }
    
    JSTextPosition end = lastTokenEndPosition();

    if (!TreeBuilder::CreatesAST && (m_syntaxAlreadyValidated || !strictMode()))
        return expr;

    location = tokenLocation();
    location.line = m_lexer->lastLineNumber();
    while (tokenStackDepth) {
        switch (context.unaryTokenStackLastType(tokenStackDepth)) {
        case EXCLAMATION:
            expr = context.createLogicalNot(location, expr);
            break;
        case TILDE:
            expr = context.makeBitwiseNotNode(location, expr);
            break;
        case MINUS:
            expr = context.makeNegateNode(location, expr);
            break;
        case PLUS:
            expr = context.createUnaryPlus(location, expr);
            break;
        case PLUSPLUS:
        case AUTOPLUSPLUS:
            expr = context.makePrefixNode(location, expr, OpPlusPlus, context.unaryTokenStackLastStart(tokenStackDepth), subExprStart + 1, end);
            m_assignmentCount++;
            break;
        case MINUSMINUS:
        case AUTOMINUSMINUS:
            expr = context.makePrefixNode(location, expr, OpMinusMinus, context.unaryTokenStackLastStart(tokenStackDepth), subExprStart + 1, end);
            m_assignmentCount++;
            break;
        case TYPEOF:
            expr = context.makeTypeOfNode(location, expr);
            break;
        case VOIDTOKEN:
            expr = context.createVoid(location, expr);
            break;
        case DELETETOKEN:
            failIfTrueIfStrictWithNameAndMessage(context.isResolve(expr), "Cannot delete unqualified property", m_lastIdentifier->impl(), "in strict mode");
            expr = context.makeDeleteNode(location, expr, context.unaryTokenStackLastStart(tokenStackDepth), end, end);
            break;
        default:
            // If we get here something has gone horribly horribly wrong
            CRASH();
        }
        subExprStart = context.unaryTokenStackLastStart(tokenStackDepth);
        context.unaryTokenStackRemoveLast(tokenStackDepth);
    }
    return expr;
}

// Instantiate the two flavors of Parser we need instead of putting most of this file in Parser.h
template class Parser< Lexer<LChar> >;
template class Parser< Lexer<UChar> >;

} // namespace JSC
