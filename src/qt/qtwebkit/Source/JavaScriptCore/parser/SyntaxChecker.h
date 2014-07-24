/*
 * Copyright (C) 2010, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SyntaxChecker_h
#define SyntaxChecker_h

#include "Lexer.h"
#include <yarr/YarrSyntaxChecker.h>

namespace JSC {
    
class SyntaxChecker {
public:
    struct BinaryExprContext {
        BinaryExprContext(SyntaxChecker& context)
            : m_context(&context)
        {
            m_context->m_topBinaryExprs.append(m_context->m_topBinaryExpr);
            m_context->m_topBinaryExpr = 0;
        }
        ~BinaryExprContext()
        {
            m_context->m_topBinaryExpr = m_context->m_topBinaryExprs.last();
            m_context->m_topBinaryExprs.removeLast();
        }
    private:
        SyntaxChecker* m_context;
    };
    struct UnaryExprContext {
        UnaryExprContext(SyntaxChecker& context)
            : m_context(&context)
        {
            m_context->m_topUnaryTokens.append(m_context->m_topUnaryToken);
            m_context->m_topUnaryToken = 0;
        }
        ~UnaryExprContext()
        {
            m_context->m_topUnaryToken = m_context->m_topUnaryTokens.last();
            m_context->m_topUnaryTokens.removeLast();
        }
    private:
        SyntaxChecker* m_context;
    };
    
    SyntaxChecker(VM* , void*)
    {
    }

    typedef SyntaxChecker FunctionBodyBuilder;
    enum { NoneExpr = 0,
        ResolveEvalExpr, ResolveExpr, NumberExpr, StringExpr,
        ThisExpr, NullExpr, BoolExpr, RegExpExpr, ObjectLiteralExpr,
        FunctionExpr, BracketExpr, DotExpr, CallExpr,
        NewExpr, PreExpr, PostExpr, UnaryExpr, BinaryExpr,
        ConditionalExpr, AssignmentExpr, TypeofExpr,
        DeleteExpr, ArrayLiteralExpr };
    typedef int ExpressionType;

    typedef ExpressionType Expression;
    typedef int SourceElements;
    typedef int Arguments;
    typedef ExpressionType Comma;
    struct Property {
        ALWAYS_INLINE Property(void* = 0)
            : type((PropertyNode::Type)0)
        {
        }
        ALWAYS_INLINE Property(const Identifier* ident, PropertyNode::Type ty)
        : name(ident)
        , type(ty)
        {
        }
        ALWAYS_INLINE Property(PropertyNode::Type ty)
            : name(0)
            , type(ty)
        {
        }
        ALWAYS_INLINE bool operator!() { return !type; }
        const Identifier* name;
        PropertyNode::Type type;
    };
    typedef int PropertyList;
    typedef int ElementList;
    typedef int ArgumentsList;
    typedef int FormalParameterList;
    typedef int FunctionBody;
    typedef int Statement;
    typedef int ClauseList;
    typedef int Clause;
    typedef int ConstDeclList;
    typedef int BinaryOperand;
    
    static const bool CreatesAST = false;
    static const bool NeedsFreeVariableInfo = false;
    static const bool CanUseFunctionCache = true;
    static const unsigned DontBuildKeywords = LexexFlagsDontBuildKeywords;
    static const unsigned DontBuildStrings = LexerFlagsDontBuildStrings;

    int createSourceElements() { return 1; }
    ExpressionType makeFunctionCallNode(const JSTokenLocation&, int, int, int, int, int) { return CallExpr; }
    void appendToComma(ExpressionType& base, ExpressionType right) { base = right; }
    ExpressionType createCommaExpr(const JSTokenLocation&, ExpressionType, ExpressionType right) { return right; }
    ExpressionType makeAssignNode(const JSTokenLocation&, ExpressionType, Operator, ExpressionType, bool, bool, int, int, int) { return AssignmentExpr; }
    ExpressionType makePrefixNode(const JSTokenLocation&, ExpressionType, Operator, int, int, int) { return PreExpr; }
    ExpressionType makePostfixNode(const JSTokenLocation&, ExpressionType, Operator, int, int, int) { return PostExpr; }
    ExpressionType makeTypeOfNode(const JSTokenLocation&, ExpressionType) { return TypeofExpr; }
    ExpressionType makeDeleteNode(const JSTokenLocation&, ExpressionType, int, int, int) { return DeleteExpr; }
    ExpressionType makeNegateNode(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType makeBitwiseNotNode(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType createLogicalNot(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType createUnaryPlus(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType createVoid(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType thisExpr(const JSTokenLocation&) { return ThisExpr; }
    ExpressionType createResolve(const JSTokenLocation&, const Identifier*, int) { return ResolveExpr; }
    ExpressionType createObjectLiteral(const JSTokenLocation&) { return ObjectLiteralExpr; }
    ExpressionType createObjectLiteral(const JSTokenLocation&, int) { return ObjectLiteralExpr; }
    ExpressionType createArray(const JSTokenLocation&, int) { return ArrayLiteralExpr; }
    ExpressionType createArray(const JSTokenLocation&, int, int) { return ArrayLiteralExpr; }
    ExpressionType createNumberExpr(const JSTokenLocation&, double) { return NumberExpr; }
    ExpressionType createString(const JSTokenLocation&, const Identifier*) { return StringExpr; }
    ExpressionType createBoolean(const JSTokenLocation&, bool) { return BoolExpr; }
    ExpressionType createNull(const JSTokenLocation&) { return NullExpr; }
    ExpressionType createBracketAccess(const JSTokenLocation&, ExpressionType, ExpressionType, bool, int, int, int) { return BracketExpr; }
    ExpressionType createDotAccess(const JSTokenLocation&, ExpressionType, const Identifier*, int, int, int) { return DotExpr; }
    ExpressionType createRegExp(const JSTokenLocation&, const Identifier& pattern, const Identifier&, int) { return Yarr::checkSyntax(pattern.string()) ? 0 : RegExpExpr; }
    ExpressionType createNewExpr(const JSTokenLocation&, ExpressionType, int, int, int, int) { return NewExpr; }
    ExpressionType createNewExpr(const JSTokenLocation&, ExpressionType, int, int) { return NewExpr; }
    ExpressionType createConditionalExpr(const JSTokenLocation&, ExpressionType, ExpressionType, ExpressionType) { return ConditionalExpr; }
    ExpressionType createAssignResolve(const JSTokenLocation&, const Identifier&, ExpressionType, int, int, int) { return AssignmentExpr; }
    ExpressionType createFunctionExpr(const JSTokenLocation&, const Identifier*, int, int, int, int, int, int, int) { return FunctionExpr; }
    int createFunctionBody(const JSTokenLocation&, const JSTokenLocation&, int, bool) { return 1; }
    void setFunctionStart(int, int) { }
    int createArguments() { return 1; }
    int createArguments(int) { return 1; }
    int createArgumentsList(const JSTokenLocation&, int) { return 1; }
    int createArgumentsList(const JSTokenLocation&, int, int) { return 1; }
    template <bool complete> Property createProperty(const Identifier* name, int, PropertyNode::Type type)
    {
        if (!complete)
            return Property(type);
        ASSERT(name);
        return Property(name, type);
    }
    template <bool complete> Property createProperty(VM* vm, double name, int, PropertyNode::Type type)
    {
        if (!complete)
            return Property(type);
        return Property(&vm->parserArena->identifierArena().makeNumericIdentifier(vm, name), type);
    }
    int createPropertyList(const JSTokenLocation&, Property) { return 1; }
    int createPropertyList(const JSTokenLocation&, Property, int) { return 1; }
    int createElementList(int, int) { return 1; }
    int createElementList(int, int, int) { return 1; }
    int createFormalParameterList(const Identifier&) { return 1; }
    int createFormalParameterList(int, const Identifier&) { return 1; }
    int createClause(int, int) { return 1; }
    int createClauseList(int) { return 1; }
    int createClauseList(int, int) { return 1; }
    void setUsesArguments(int) { }
    int createFuncDeclStatement(const JSTokenLocation&, const Identifier*, int, int, int, int, int, int, int) { return 1; }
    int createBlockStatement(const JSTokenLocation&, int, int, int) { return 1; }
    int createExprStatement(const JSTokenLocation&, int, int, int) { return 1; }
    int createIfStatement(const JSTokenLocation&, int, int, int, int) { return 1; }
    int createIfStatement(const JSTokenLocation&, int, int, int, int, int) { return 1; }
    int createForLoop(const JSTokenLocation&, int, int, int, int, int, int) { return 1; }
    int createForInLoop(const JSTokenLocation&, const Identifier*, int, int, int, int, int, int, int, int, int, int) { return 1; }
    int createForInLoop(const JSTokenLocation&, int, int, int, int, int, int, int, int) { return 1; }
    int createEmptyStatement(const JSTokenLocation&) { return 1; }
    int createVarStatement(const JSTokenLocation&, int, int, int) { return 1; }
    int createReturnStatement(const JSTokenLocation&, int, int, int) { return 1; }
    int createBreakStatement(const JSTokenLocation&, int, int) { return 1; }
    int createBreakStatement(const JSTokenLocation&, const Identifier*, int, int) { return 1; }
    int createContinueStatement(const JSTokenLocation&, int, int) { return 1; }
    int createContinueStatement(const JSTokenLocation&, const Identifier*, int, int) { return 1; }
    int createTryStatement(const JSTokenLocation&, int, const Identifier*, int, int, int, int) { return 1; }
    int createSwitchStatement(const JSTokenLocation&, int, int, int, int, int, int) { return 1; }
    int createWhileStatement(const JSTokenLocation&, int, int, int, int) { return 1; }
    int createWithStatement(const JSTokenLocation&, int, int, int, int, int, int) { return 1; }
    int createDoWhileStatement(const JSTokenLocation&, int, int, int, int) { return 1; }
    int createLabelStatement(const JSTokenLocation&, const Identifier*, int, int, int) { return 1; }
    int createThrowStatement(const JSTokenLocation&, int, int, int) { return 1; }
    int createDebugger(const JSTokenLocation&, int, int) { return 1; }
    int createConstStatement(const JSTokenLocation&, int, int, int) { return 1; }
    int appendConstDecl(const JSTokenLocation&, int, const Identifier*, int) { return 1; }
    template <bool strict> Property createGetterOrSetterProperty(const JSTokenLocation&, PropertyNode::Type type, const Identifier* name, int, int, int, int, int, int, int)
    {
        ASSERT(name);
        if (!strict)
            return Property(type);
        return Property(name, type);
    }
    template <bool strict> Property createGetterOrSetterProperty(VM* vm, const JSTokenLocation&, PropertyNode::Type type, double name, int, int, int, int, int, int, int)
    {
        if (!strict)
            return Property(type);
        return Property(&vm->parserArena->identifierArena().makeNumericIdentifier(vm, name), type);
    }

    void appendStatement(int, int) { }
    void addVar(const Identifier*, bool) { }
    int combineCommaNodes(const JSTokenLocation&, int, int) { return 1; }
    int evalCount() const { return 0; }
    void appendBinaryExpressionInfo(int& operandStackDepth, int expr, int, int, int, bool)
    {
        if (!m_topBinaryExpr)
            m_topBinaryExpr = expr;
        else
            m_topBinaryExpr = BinaryExpr;
        operandStackDepth++;
    }
    
    // Logic to handle datastructures used during parsing of binary expressions
    void operatorStackPop(int& operatorStackDepth) { operatorStackDepth--; }
    bool operatorStackHasHigherPrecedence(int&, int) { return true; }
    BinaryOperand getFromOperandStack(int) { return m_topBinaryExpr; }
    void shrinkOperandStackBy(int& operandStackDepth, int amount) { operandStackDepth -= amount; }
    void appendBinaryOperation(const JSTokenLocation&, int& operandStackDepth, int&, BinaryOperand, BinaryOperand) { operandStackDepth++; }
    void operatorStackAppend(int& operatorStackDepth, int, int) { operatorStackDepth++; }
    int popOperandStack(int&) { int res = m_topBinaryExpr; m_topBinaryExpr = 0; return res; }
    
    void appendUnaryToken(int& stackDepth, int tok, int) { stackDepth = 1; m_topUnaryToken = tok; }
    int unaryTokenStackLastType(int&) { return m_topUnaryToken; }
    JSTextPosition unaryTokenStackLastStart(int&) { return JSTextPosition(0, 0, 0); }
    void unaryTokenStackRemoveLast(int& stackDepth) { stackDepth = 0; }
    
    void assignmentStackAppend(int, int, int, int, int, Operator) { }
    int createAssignment(const JSTokenLocation&, int, int, int, int, int) { RELEASE_ASSERT_NOT_REACHED(); return 1; }
    const Identifier& getName(const Property& property) const { ASSERT(property.name); return *property.name; }
    PropertyNode::Type getType(const Property& property) const { return property.type; }
    bool isResolve(ExpressionType expr) const { return expr == ResolveExpr || expr == ResolveEvalExpr; }
    
private:
    int m_topBinaryExpr;
    int m_topUnaryToken;
    Vector<int, 8> m_topBinaryExprs;
    Vector<int, 8> m_topUnaryTokens;
};

}

#endif
