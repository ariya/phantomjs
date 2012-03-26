/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
    
    SyntaxChecker(JSGlobalData* , Lexer*)
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

    int createSourceElements() { return 1; }
    ExpressionType makeFunctionCallNode(int, int, int, int, int) { return CallExpr; }
    void appendToComma(ExpressionType& base, ExpressionType right) { base = right; }
    ExpressionType createCommaExpr(ExpressionType, ExpressionType right) { return right; }
    ExpressionType makeAssignNode(ExpressionType, Operator, ExpressionType, bool, bool, int, int, int) { return AssignmentExpr; }
    ExpressionType makePrefixNode(ExpressionType, Operator, int, int, int) { return PreExpr; }
    ExpressionType makePostfixNode(ExpressionType, Operator, int, int, int) { return PostExpr; }
    ExpressionType makeTypeOfNode(ExpressionType) { return TypeofExpr; }
    ExpressionType makeDeleteNode(ExpressionType, int, int, int) { return DeleteExpr; }
    ExpressionType makeNegateNode(ExpressionType) { return UnaryExpr; }
    ExpressionType makeBitwiseNotNode(ExpressionType) { return UnaryExpr; }
    ExpressionType createLogicalNot(ExpressionType) { return UnaryExpr; }
    ExpressionType createUnaryPlus(ExpressionType) { return UnaryExpr; }
    ExpressionType createVoid(ExpressionType) { return UnaryExpr; }
    ExpressionType thisExpr() { return ThisExpr; }
    ExpressionType createResolve(const Identifier*, int) { return ResolveExpr; }
    ExpressionType createObjectLiteral() { return ObjectLiteralExpr; }
    ExpressionType createObjectLiteral(int) { return ObjectLiteralExpr; }
    ExpressionType createArray(int) { return ArrayLiteralExpr; }
    ExpressionType createArray(int, int) { return ArrayLiteralExpr; }
    ExpressionType createNumberExpr(double) { return NumberExpr; }
    ExpressionType createString(const Identifier*) { return StringExpr; }
    ExpressionType createBoolean(bool) { return BoolExpr; }
    ExpressionType createNull() { return NullExpr; }
    ExpressionType createBracketAccess(ExpressionType, ExpressionType, bool, int, int, int) { return BracketExpr; }
    ExpressionType createDotAccess(ExpressionType, const Identifier&, int, int, int) { return DotExpr; }
    ExpressionType createRegExp(const Identifier& pattern, const Identifier&, int) { return Yarr::checkSyntax(pattern.ustring()) ? 0 : RegExpExpr; }
    ExpressionType createNewExpr(ExpressionType, int, int, int, int) { return NewExpr; }
    ExpressionType createNewExpr(ExpressionType, int, int) { return NewExpr; }
    ExpressionType createConditionalExpr(ExpressionType, ExpressionType, ExpressionType) { return ConditionalExpr; }
    ExpressionType createAssignResolve(const Identifier&, ExpressionType, bool, int, int, int) { return AssignmentExpr; }
    ExpressionType createFunctionExpr(const Identifier*, int, int, int, int, int, int) { return FunctionExpr; }
    int createFunctionBody(bool) { return 1; }
    int createArguments() { return 1; }
    int createArguments(int) { return 1; }
    int createArgumentsList(int) { return 1; }
    int createArgumentsList(int, int) { return 1; }
    template <bool complete> Property createProperty(const Identifier* name, int, PropertyNode::Type type)
    {
        ASSERT(name);
        if (!complete)
            return Property(type);
        return Property(name, type);
    }
    template <bool complete> Property createProperty(JSGlobalData* globalData, double name, int, PropertyNode::Type type)
    {
        if (!complete)
            return Property(type);
        return Property(&globalData->parser->arena().identifierArena().makeNumericIdentifier(globalData, name), type);
    }
    int createPropertyList(Property) { return 1; }
    int createPropertyList(Property, int) { return 1; }
    int createElementList(int, int) { return 1; }
    int createElementList(int, int, int) { return 1; }
    int createFormalParameterList(const Identifier&) { return 1; }
    int createFormalParameterList(int, const Identifier&) { return 1; }
    int createClause(int, int) { return 1; }
    int createClauseList(int) { return 1; }
    int createClauseList(int, int) { return 1; }
    void setUsesArguments(int) { }
    int createFuncDeclStatement(const Identifier*, int, int, int, int, int, int) { return 1; }
    int createBlockStatement(int, int, int) { return 1; }
    int createExprStatement(int, int, int) { return 1; }
    int createIfStatement(int, int, int, int) { return 1; }
    int createIfStatement(int, int, int, int, int) { return 1; }
    int createForLoop(int, int, int, int, bool, int, int) { return 1; }
    int createForInLoop(const Identifier*, int, int, int, int, int, int, int, int, int, int) { return 1; }
    int createForInLoop(int, int, int, int, int, int, int, int) { return 1; }
    int createEmptyStatement() { return 1; }
    int createVarStatement(int, int, int) { return 1; }
    int createReturnStatement(int, int, int, int, int) { return 1; }
    int createBreakStatement(int, int, int, int) { return 1; }
    int createBreakStatement(const Identifier*, int, int, int, int) { return 1; }
    int createContinueStatement(int, int, int, int) { return 1; }
    int createContinueStatement(const Identifier*, int, int, int, int) { return 1; }
    int createTryStatement(int, const Identifier*, bool, int, int, int, int) { return 1; }
    int createSwitchStatement(int, int, int, int, int, int) { return 1; }
    int createWhileStatement(int, int, int, int) { return 1; }
    int createWithStatement(int, int, int, int, int, int) { return 1; }
    int createDoWhileStatement(int, int, int, int) { return 1; }
    int createLabelStatement(const Identifier*, int, int, int) { return 1; }
    int createThrowStatement(int, int, int, int, int) { return 1; }
    int createDebugger(int, int) { return 1; }
    int createConstStatement(int, int, int) { return 1; }
    int appendConstDecl(int, const Identifier*, int) { return 1; }
    template <bool strict> Property createGetterOrSetterProperty(PropertyNode::Type type, const Identifier* name, int, int, int, int, int, int)
    {
        ASSERT(name);
        if (!strict)
            return Property(type);
        return Property(name, type);
    }

    void appendStatement(int, int) { }
    void addVar(const Identifier*, bool) { }
    int combineCommaNodes(int, int) { return 1; }
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
    void appendBinaryOperation(int& operandStackDepth, int&, BinaryOperand, BinaryOperand) { operandStackDepth++; }
    void operatorStackAppend(int& operatorStackDepth, int, int) { operatorStackDepth++; }
    int popOperandStack(int&) { int res = m_topBinaryExpr; m_topBinaryExpr = 0; return res; }
    
    void appendUnaryToken(int& stackDepth, int tok, int) { stackDepth = 1; m_topUnaryToken = tok; }
    int unaryTokenStackLastType(int&) { return m_topUnaryToken; }
    int unaryTokenStackLastStart(int&) { return 0; }
    void unaryTokenStackRemoveLast(int& stackDepth) { stackDepth = 0; }
    
    void assignmentStackAppend(int, int, int, int, int, Operator) { }
    int createAssignment(int, int, int, int, int) { ASSERT_NOT_REACHED(); return 1; }
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
