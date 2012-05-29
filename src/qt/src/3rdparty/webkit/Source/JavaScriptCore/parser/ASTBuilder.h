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

#ifndef ASTBuilder_h
#define ASTBuilder_h

#include "NodeConstructors.h"
#include "SyntaxChecker.h"
#include <utility>

namespace JSC {

class ASTBuilder {
    struct BinaryOpInfo {
        BinaryOpInfo() {}
        BinaryOpInfo(int s, int d, int e, bool r)
            : start(s)
            , divot(d)
            , end(e)
            , hasAssignment(r)
        {
        }
        BinaryOpInfo(const BinaryOpInfo& lhs, const BinaryOpInfo& rhs)
            : start(lhs.start)
            , divot(rhs.start)
            , end(rhs.end)
            , hasAssignment(lhs.hasAssignment || rhs.hasAssignment)
        {
        }
        int start;
        int divot;
        int end;
        bool hasAssignment;
    };
    
    
    struct AssignmentInfo {
        AssignmentInfo() {}
        AssignmentInfo(ExpressionNode* node, int start, int divot, int initAssignments, Operator op)
            : m_node(node)
            , m_start(start)
            , m_divot(divot)
            , m_initAssignments(initAssignments)
            , m_op(op)
        {
        }
        ExpressionNode* m_node;
        int m_start;
        int m_divot;
        int m_initAssignments;
        Operator m_op;
    };
public:
    ASTBuilder(JSGlobalData* globalData, Lexer* lexer)
        : m_globalData(globalData)
        , m_lexer(lexer)
        , m_scope(globalData)
        , m_evalCount(0)
    {
    }
    
    struct BinaryExprContext {
        BinaryExprContext(ASTBuilder&) {}
    };
    struct UnaryExprContext {
        UnaryExprContext(ASTBuilder&) {}
    };

    typedef SyntaxChecker FunctionBodyBuilder;

    typedef ExpressionNode* Expression;
    typedef JSC::SourceElements* SourceElements;
    typedef ArgumentsNode* Arguments;
    typedef CommaNode* Comma;
    typedef PropertyNode* Property;
    typedef PropertyListNode* PropertyList;
    typedef ElementNode* ElementList;
    typedef ArgumentListNode* ArgumentsList;
    typedef ParameterNode* FormalParameterList;
    typedef FunctionBodyNode* FunctionBody;
    typedef StatementNode* Statement;
    typedef ClauseListNode* ClauseList;
    typedef CaseClauseNode* Clause;
    typedef ConstDeclNode* ConstDeclList;
    typedef std::pair<ExpressionNode*, BinaryOpInfo> BinaryOperand;
    
    static const bool CreatesAST = true;
    static const bool NeedsFreeVariableInfo = true;
    static const bool CanUseFunctionCache = true;

    ExpressionNode* makeBinaryNode(int token, std::pair<ExpressionNode*, BinaryOpInfo>, std::pair<ExpressionNode*, BinaryOpInfo>);
    ExpressionNode* makeFunctionCallNode(ExpressionNode* func, ArgumentsNode* args, int start, int divot, int end);

    JSC::SourceElements* createSourceElements() { return new (m_globalData) JSC::SourceElements(m_globalData); }

    ParserArenaData<DeclarationStacks::VarStack>* varDeclarations() { return m_scope.m_varDeclarations; }
    ParserArenaData<DeclarationStacks::FunctionStack>* funcDeclarations() { return m_scope.m_funcDeclarations; }
    int features() const { return m_scope.m_features; }
    int numConstants() const { return m_scope.m_numConstants; }

    void appendToComma(CommaNode* commaNode, ExpressionNode* expr) { commaNode->append(expr); }

    CommaNode* createCommaExpr(ExpressionNode* lhs, ExpressionNode* rhs) { return new (m_globalData) CommaNode(m_globalData, lhs, rhs); }

    ExpressionNode* makeAssignNode(ExpressionNode* left, Operator, ExpressionNode* right, bool leftHasAssignments, bool rightHasAssignments, int start, int divot, int end);
    ExpressionNode* makePrefixNode(ExpressionNode*, Operator, int start, int divot, int end);
    ExpressionNode* makePostfixNode(ExpressionNode*, Operator, int start, int divot, int end);
    ExpressionNode* makeTypeOfNode(ExpressionNode*);
    ExpressionNode* makeDeleteNode(ExpressionNode*, int start, int divot, int end);
    ExpressionNode* makeNegateNode(ExpressionNode*);
    ExpressionNode* makeBitwiseNotNode(ExpressionNode*);
    ExpressionNode* makeMultNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeDivNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeModNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeAddNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeSubNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeBitXOrNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeBitAndNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeBitOrNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeLeftShiftNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeRightShiftNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);
    ExpressionNode* makeURightShiftNode(ExpressionNode* left, ExpressionNode* right, bool rightHasAssignments);

    ExpressionNode* createLogicalNot(ExpressionNode* expr) { return new (m_globalData) LogicalNotNode(m_globalData, expr); }
    ExpressionNode* createUnaryPlus(ExpressionNode* expr) { return new (m_globalData) UnaryPlusNode(m_globalData, expr); }
    ExpressionNode* createVoid(ExpressionNode* expr)
    {
        incConstants();
        return new (m_globalData) VoidNode(m_globalData, expr);
    }
    ExpressionNode* thisExpr()
    {
        usesThis();
        return new (m_globalData) ThisNode(m_globalData);
    }
    ExpressionNode* createResolve(const Identifier* ident, int start)
    {
        if (m_globalData->propertyNames->arguments == *ident)
            usesArguments();
        return new (m_globalData) ResolveNode(m_globalData, *ident, start);
    }
    ExpressionNode* createObjectLiteral() { return new (m_globalData) ObjectLiteralNode(m_globalData); }
    ExpressionNode* createObjectLiteral(PropertyListNode* properties) { return new (m_globalData) ObjectLiteralNode(m_globalData, properties); }

    ExpressionNode* createArray(int elisions)
    {
        if (elisions)
            incConstants();
        return new (m_globalData) ArrayNode(m_globalData, elisions);
    }

    ExpressionNode* createArray(ElementNode* elems) { return new (m_globalData) ArrayNode(m_globalData, elems); }
    ExpressionNode* createArray(int elisions, ElementNode* elems)
    {
        if (elisions)
            incConstants();
        return new (m_globalData) ArrayNode(m_globalData, elisions, elems);
    }
    ExpressionNode* createNumberExpr(double d)
    {
        incConstants();
        return new (m_globalData) NumberNode(m_globalData, d);
    }

    ExpressionNode* createString(const Identifier* string)
    {
        incConstants();
        return new (m_globalData) StringNode(m_globalData, *string);
    }

    ExpressionNode* createBoolean(bool b)
    {
        incConstants();
        return new (m_globalData) BooleanNode(m_globalData, b);
    }

    ExpressionNode* createNull()
    {
        incConstants();
        return new (m_globalData) NullNode(m_globalData);
    }

    ExpressionNode* createBracketAccess(ExpressionNode* base, ExpressionNode* property, bool propertyHasAssignments, int start, int divot, int end)
    {
        BracketAccessorNode* node = new (m_globalData) BracketAccessorNode(m_globalData, base, property, propertyHasAssignments);
        setExceptionLocation(node, start, divot, end);
        return node;
    }

    ExpressionNode* createDotAccess(ExpressionNode* base, const Identifier& property, int start, int divot, int end)
    {
        DotAccessorNode* node = new (m_globalData) DotAccessorNode(m_globalData, base, property);
        setExceptionLocation(node, start, divot, end);
        return node;
    }

    ExpressionNode* createRegExp(const Identifier& pattern, const Identifier& flags, int start)
    {
        if (Yarr::checkSyntax(pattern.ustring()))
            return 0;
        RegExpNode* node = new (m_globalData) RegExpNode(m_globalData, pattern, flags);
        int size = pattern.length() + 2; // + 2 for the two /'s
        setExceptionLocation(node, start, start + size, start + size);
        return node;
    }

    ExpressionNode* createNewExpr(ExpressionNode* expr, ArgumentsNode* arguments, int start, int divot, int end)
    {
        NewExprNode* node = new (m_globalData) NewExprNode(m_globalData, expr, arguments);
        setExceptionLocation(node, start, divot, end);
        return node;
    }

    ExpressionNode* createNewExpr(ExpressionNode* expr, int start, int end)
    {
        NewExprNode* node = new (m_globalData) NewExprNode(m_globalData, expr);
        setExceptionLocation(node, start, end, end);
        return node;
    }

    ExpressionNode* createConditionalExpr(ExpressionNode* condition, ExpressionNode* lhs, ExpressionNode* rhs)
    {
        return new (m_globalData) ConditionalNode(m_globalData, condition, lhs, rhs);
    }

    ExpressionNode* createAssignResolve(const Identifier& ident, ExpressionNode* rhs, bool rhsHasAssignment, int start, int divot, int end)
    {
        AssignResolveNode* node = new (m_globalData) AssignResolveNode(m_globalData, ident, rhs, rhsHasAssignment);
        setExceptionLocation(node, start, divot, end);
        return node;
    }

    ExpressionNode* createFunctionExpr(const Identifier* name, FunctionBodyNode* body, ParameterNode* parameters, int openBracePos, int closeBracePos, int bodyStartLine, int bodyEndLine)
    {
        FuncExprNode* result = new (m_globalData) FuncExprNode(m_globalData, *name, body, m_lexer->sourceCode(openBracePos, closeBracePos, bodyStartLine), parameters);
        body->setLoc(bodyStartLine, bodyEndLine);
        return result;
    }

    FunctionBodyNode* createFunctionBody(bool inStrictContext)
    {
        usesClosures();
        return FunctionBodyNode::create(m_globalData, inStrictContext);
    }
    
    template <bool> PropertyNode* createGetterOrSetterProperty(PropertyNode::Type type, const Identifier* name, ParameterNode* params, FunctionBodyNode* body, int openBracePos, int closeBracePos, int bodyStartLine, int bodyEndLine)
    {
        ASSERT(name);
        body->setLoc(bodyStartLine, bodyEndLine);
        return new (m_globalData) PropertyNode(m_globalData, *name, new (m_globalData) FuncExprNode(m_globalData, m_globalData->propertyNames->nullIdentifier, body, m_lexer->sourceCode(openBracePos, closeBracePos, bodyStartLine), params), type);
    }
    

    ArgumentsNode* createArguments() { return new (m_globalData) ArgumentsNode(m_globalData); }
    ArgumentsNode* createArguments(ArgumentListNode* args) { return new (m_globalData) ArgumentsNode(m_globalData, args); }
    ArgumentListNode* createArgumentsList(ExpressionNode* arg) { return new (m_globalData) ArgumentListNode(m_globalData, arg); }
    ArgumentListNode* createArgumentsList(ArgumentListNode* args, ExpressionNode* arg) { return new (m_globalData) ArgumentListNode(m_globalData, args, arg); }

    template <bool> PropertyNode* createProperty(const Identifier* propertyName, ExpressionNode* node, PropertyNode::Type type) { return new (m_globalData) PropertyNode(m_globalData, *propertyName, node, type); }
    template <bool> PropertyNode* createProperty(JSGlobalData*, double propertyName, ExpressionNode* node, PropertyNode::Type type) { return new (m_globalData) PropertyNode(m_globalData, propertyName, node, type); }
    PropertyListNode* createPropertyList(PropertyNode* property) { return new (m_globalData) PropertyListNode(m_globalData, property); }
    PropertyListNode* createPropertyList(PropertyNode* property, PropertyListNode* tail) { return new (m_globalData) PropertyListNode(m_globalData, property, tail); }

    ElementNode* createElementList(int elisions, ExpressionNode* expr) { return new (m_globalData) ElementNode(m_globalData, elisions, expr); }
    ElementNode* createElementList(ElementNode* elems, int elisions, ExpressionNode* expr) { return new (m_globalData) ElementNode(m_globalData, elems, elisions, expr); }

    ParameterNode* createFormalParameterList(const Identifier& ident) { return new (m_globalData) ParameterNode(m_globalData, ident); }
    ParameterNode* createFormalParameterList(ParameterNode* list, const Identifier& ident) { return new (m_globalData) ParameterNode(m_globalData, list, ident); }

    CaseClauseNode* createClause(ExpressionNode* expr, JSC::SourceElements* statements) { return new (m_globalData) CaseClauseNode(m_globalData, expr, statements); }
    ClauseListNode* createClauseList(CaseClauseNode* clause) { return new (m_globalData) ClauseListNode(m_globalData, clause); }
    ClauseListNode* createClauseList(ClauseListNode* tail, CaseClauseNode* clause) { return new (m_globalData) ClauseListNode(m_globalData, tail, clause); }

    void setUsesArguments(FunctionBodyNode* node) { node->setUsesArguments(); }

    StatementNode* createFuncDeclStatement(const Identifier* name, FunctionBodyNode* body, ParameterNode* parameters, int openBracePos, int closeBracePos, int bodyStartLine, int bodyEndLine)
    {
        FuncDeclNode* decl = new (m_globalData) FuncDeclNode(m_globalData, *name, body, m_lexer->sourceCode(openBracePos, closeBracePos, bodyStartLine), parameters);
        if (*name == m_globalData->propertyNames->arguments)
            usesArguments();
        m_scope.m_funcDeclarations->data.append(decl->body());
        body->setLoc(bodyStartLine, bodyEndLine);
        return decl;
    }

    StatementNode* createBlockStatement(JSC::SourceElements* elements, int startLine, int endLine)
    {
        BlockNode* block = new (m_globalData) BlockNode(m_globalData, elements);
        block->setLoc(startLine, endLine);
        return block;
    }

    StatementNode* createExprStatement(ExpressionNode* expr, int start, int end)
    {
        ExprStatementNode* result = new (m_globalData) ExprStatementNode(m_globalData, expr);
        result->setLoc(start, end);
        return result;
    }

    StatementNode* createIfStatement(ExpressionNode* condition, StatementNode* trueBlock, int start, int end)
    {
        IfNode* result = new (m_globalData) IfNode(m_globalData, condition, trueBlock);
        result->setLoc(start, end);
        return result;
    }

    StatementNode* createIfStatement(ExpressionNode* condition, StatementNode* trueBlock, StatementNode* falseBlock, int start, int end)
    {
        IfNode* result = new (m_globalData) IfElseNode(m_globalData, condition, trueBlock, falseBlock);
        result->setLoc(start, end);
        return result;
    }

    StatementNode* createForLoop(ExpressionNode* initializer, ExpressionNode* condition, ExpressionNode* iter, StatementNode* statements, bool b, int start, int end)
    {
        ForNode* result = new (m_globalData) ForNode(m_globalData, initializer, condition, iter, statements, b);
        result->setLoc(start, end);
        return result;
    }

    StatementNode* createForInLoop(const Identifier* ident, ExpressionNode* initializer, ExpressionNode* iter, StatementNode* statements, int start, int divot, int end, int initStart, int initEnd, int startLine, int endLine)
    {
        ForInNode* result = new (m_globalData) ForInNode(m_globalData, *ident, initializer, iter, statements, initStart, initStart - start, initEnd - initStart);
        result->setLoc(startLine, endLine);
        setExceptionLocation(result, start, divot + 1, end);
        return result;
    }

    StatementNode* createForInLoop(ExpressionNode* lhs, ExpressionNode* iter, StatementNode* statements, int eStart, int eDivot, int eEnd, int start, int end)
    {
        ForInNode* result = new (m_globalData) ForInNode(m_globalData, lhs, iter, statements);
        result->setLoc(start, end);
        setExceptionLocation(result, eStart, eDivot, eEnd);
        return result;
    }

    StatementNode* createEmptyStatement() { return new (m_globalData) EmptyStatementNode(m_globalData); }

    StatementNode* createVarStatement(ExpressionNode* expr, int start, int end)
    {
        StatementNode* result;
        if (!expr)
            result = new (m_globalData) EmptyStatementNode(m_globalData);
        else
            result = new (m_globalData) VarStatementNode(m_globalData, expr);
        result->setLoc(start, end);
        return result;
    }

    StatementNode* createReturnStatement(ExpressionNode* expression, int eStart, int eEnd, int startLine, int endLine)
    {
        ReturnNode* result = new (m_globalData) ReturnNode(m_globalData, expression);
        setExceptionLocation(result, eStart, eEnd, eEnd);
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createBreakStatement(int eStart, int eEnd, int startLine, int endLine)
    {
        BreakNode* result = new (m_globalData) BreakNode(m_globalData);
        setExceptionLocation(result, eStart, eEnd, eEnd);
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createBreakStatement(const Identifier* ident, int eStart, int eEnd, int startLine, int endLine)
    {
        BreakNode* result = new (m_globalData) BreakNode(m_globalData, *ident);
        setExceptionLocation(result, eStart, eEnd, eEnd);
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createContinueStatement(int eStart, int eEnd, int startLine, int endLine)
    {
        ContinueNode* result = new (m_globalData) ContinueNode(m_globalData);
        setExceptionLocation(result, eStart, eEnd, eEnd);
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createContinueStatement(const Identifier* ident, int eStart, int eEnd, int startLine, int endLine)
    {
        ContinueNode* result = new (m_globalData) ContinueNode(m_globalData, *ident);
        setExceptionLocation(result, eStart, eEnd, eEnd);
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createTryStatement(StatementNode* tryBlock, const Identifier* ident, bool catchHasEval, StatementNode* catchBlock, StatementNode* finallyBlock, int startLine, int endLine)
    {
        TryNode* result = new (m_globalData) TryNode(m_globalData, tryBlock, *ident, catchHasEval, catchBlock, finallyBlock);
        if (catchBlock)
            usesCatch();
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createSwitchStatement(ExpressionNode* expr, ClauseListNode* firstClauses, CaseClauseNode* defaultClause, ClauseListNode* secondClauses, int startLine, int endLine)
    {
        CaseBlockNode* cases = new (m_globalData) CaseBlockNode(m_globalData, firstClauses, defaultClause, secondClauses);
        SwitchNode* result = new (m_globalData) SwitchNode(m_globalData, expr, cases);
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createWhileStatement(ExpressionNode* expr, StatementNode* statement, int startLine, int endLine)
    {
        WhileNode* result = new (m_globalData) WhileNode(m_globalData, expr, statement);
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createDoWhileStatement(StatementNode* statement, ExpressionNode* expr, int startLine, int endLine)
    {
        DoWhileNode* result = new (m_globalData) DoWhileNode(m_globalData, statement, expr);
        result->setLoc(startLine, endLine);
        return result;
    }

    StatementNode* createLabelStatement(const Identifier* ident, StatementNode* statement, int start, int end)
    {
        LabelNode* result = new (m_globalData) LabelNode(m_globalData, *ident, statement);
        setExceptionLocation(result, start, end, end);
        return result;
    }

    StatementNode* createWithStatement(ExpressionNode* expr, StatementNode* statement, int start, int end, int startLine, int endLine)
    {
        usesWith();
        WithNode* result = new (m_globalData) WithNode(m_globalData, expr, statement, end, end - start);
        result->setLoc(startLine, endLine);
        return result;
    }    
    
    StatementNode* createThrowStatement(ExpressionNode* expr, int start, int end, int startLine, int endLine)
    {
        ThrowNode* result = new (m_globalData) ThrowNode(m_globalData, expr);
        result->setLoc(startLine, endLine);
        setExceptionLocation(result, start, end, end);
        return result;
    }
    
    StatementNode* createDebugger(int startLine, int endLine)
    {
        DebuggerStatementNode* result = new (m_globalData) DebuggerStatementNode(m_globalData);
        result->setLoc(startLine, endLine);
        return result;
    }
    
    StatementNode* createConstStatement(ConstDeclNode* decls, int startLine, int endLine)
    {
        ConstStatementNode* result = new (m_globalData) ConstStatementNode(m_globalData, decls);
        result->setLoc(startLine, endLine);
        return result;
    }

    ConstDeclNode* appendConstDecl(ConstDeclNode* tail, const Identifier* name, ExpressionNode* initializer)
    {
        ConstDeclNode* result = new (m_globalData) ConstDeclNode(m_globalData, *name, initializer);
        if (tail)
            tail->m_next = result;
        return result;
    }

    void appendStatement(JSC::SourceElements* elements, JSC::StatementNode* statement)
    {
        elements->append(statement);
    }

    void addVar(const Identifier* ident, int attrs)
    {
        if (m_globalData->propertyNames->arguments == *ident)
            usesArguments();
        m_scope.m_varDeclarations->data.append(std::make_pair(ident, attrs));
    }

    ExpressionNode* combineCommaNodes(ExpressionNode* list, ExpressionNode* init)
    {
        if (!list)
            return init;
        if (list->isCommaNode()) {
            static_cast<CommaNode*>(list)->append(init);
            return list;
        }
        return new (m_globalData) CommaNode(m_globalData, list, init);
    }

    int evalCount() const { return m_evalCount; }

    void appendBinaryExpressionInfo(int& operandStackDepth, ExpressionNode* current, int exprStart, int lhs, int rhs, bool hasAssignments)
    {
        operandStackDepth++;
        m_binaryOperandStack.append(std::make_pair(current, BinaryOpInfo(exprStart, lhs, rhs, hasAssignments)));
    }

    // Logic to handle datastructures used during parsing of binary expressions
    void operatorStackPop(int& operatorStackDepth)
    {
        operatorStackDepth--;
        m_binaryOperatorStack.removeLast();
    }
    bool operatorStackHasHigherPrecedence(int&, int precedence)
    {
        return precedence <= m_binaryOperatorStack.last().second;
    }
    const BinaryOperand& getFromOperandStack(int i) { return m_binaryOperandStack[m_binaryOperandStack.size() + i]; }
    void shrinkOperandStackBy(int& operandStackDepth, int amount)
    {
        operandStackDepth -= amount;
        ASSERT(operandStackDepth >= 0);
        m_binaryOperandStack.resize(m_binaryOperandStack.size() - amount);
    }
    void appendBinaryOperation(int& operandStackDepth, int&, const BinaryOperand& lhs, const BinaryOperand& rhs)
    {
        operandStackDepth++;
        m_binaryOperandStack.append(std::make_pair(makeBinaryNode(m_binaryOperatorStack.last().first, lhs, rhs), BinaryOpInfo(lhs.second, rhs.second)));
    }
    void operatorStackAppend(int& operatorStackDepth, int op, int precedence)
    {
        operatorStackDepth++;
        m_binaryOperatorStack.append(std::make_pair(op, precedence));
    }
    ExpressionNode* popOperandStack(int&)
    {
        ExpressionNode* result = m_binaryOperandStack.last().first;
        m_binaryOperandStack.removeLast();
        return result;
    }
    
    void appendUnaryToken(int& tokenStackDepth, int type, int start)
    {
        tokenStackDepth++;
        m_unaryTokenStack.append(std::make_pair(type, start));
    }

    int unaryTokenStackLastType(int&)
    {
        return m_unaryTokenStack.last().first;
    }
    
    int unaryTokenStackLastStart(int&)
    {
        return m_unaryTokenStack.last().second;
    }
    
    void unaryTokenStackRemoveLast(int& tokenStackDepth)
    {
        tokenStackDepth--;
        m_unaryTokenStack.removeLast();
    }
    
    void assignmentStackAppend(int& assignmentStackDepth, ExpressionNode* node, int start, int divot, int assignmentCount, Operator op)
    {
        assignmentStackDepth++;
        m_assignmentInfoStack.append(AssignmentInfo(node, start, divot, assignmentCount, op));
    }

    ExpressionNode* createAssignment(int& assignmentStackDepth, ExpressionNode* rhs, int initialAssignmentCount, int currentAssignmentCount, int lastTokenEnd)
    {
        ExpressionNode* result = makeAssignNode(m_assignmentInfoStack.last().m_node, m_assignmentInfoStack.last().m_op, rhs, m_assignmentInfoStack.last().m_initAssignments != initialAssignmentCount, m_assignmentInfoStack.last().m_initAssignments != currentAssignmentCount, m_assignmentInfoStack.last().m_start, m_assignmentInfoStack.last().m_divot + 1, lastTokenEnd);
        m_assignmentInfoStack.removeLast();
        assignmentStackDepth--;
        return result;
    }
    
    const Identifier& getName(Property property) const { return property->name(); }
    PropertyNode::Type getType(Property property) const { return property->type(); }

    bool isResolve(ExpressionNode* expr) const { return expr->isResolveNode(); }

private:
    struct Scope {
        Scope(JSGlobalData* globalData)
            : m_varDeclarations(new (globalData) ParserArenaData<DeclarationStacks::VarStack>)
            , m_funcDeclarations(new (globalData) ParserArenaData<DeclarationStacks::FunctionStack>)
            , m_features(0)
            , m_numConstants(0)
        {
        }
        ParserArenaData<DeclarationStacks::VarStack>* m_varDeclarations;
        ParserArenaData<DeclarationStacks::FunctionStack>* m_funcDeclarations;
        int m_features;
        int m_numConstants;
    };

    static void setExceptionLocation(ThrowableExpressionData* node, unsigned start, unsigned divot, unsigned end)
    {
        node->setExceptionSourceCode(divot, divot - start, end - divot);
    }

    void incConstants() { m_scope.m_numConstants++; }
    void usesThis() { m_scope.m_features |= ThisFeature; }
    void usesCatch() { m_scope.m_features |= CatchFeature; }
    void usesClosures() { m_scope.m_features |= ClosureFeature; }
    void usesArguments() { m_scope.m_features |= ArgumentsFeature; }
    void usesAssignment() { m_scope.m_features |= AssignFeature; }
    void usesWith() { m_scope.m_features |= WithFeature; }
    void usesEval() 
    {
        m_evalCount++;
        m_scope.m_features |= EvalFeature;
    }
    ExpressionNode* createNumber(double d)
    {
        return new (m_globalData) NumberNode(m_globalData, d);
    }
    
    JSGlobalData* m_globalData;
    Lexer* m_lexer;
    Scope m_scope;
    Vector<BinaryOperand, 10> m_binaryOperandStack;
    Vector<AssignmentInfo, 10> m_assignmentInfoStack;
    Vector<pair<int, int>, 10> m_binaryOperatorStack;
    Vector<pair<int, int>, 10> m_unaryTokenStack;
    int m_evalCount;
};

ExpressionNode* ASTBuilder::makeTypeOfNode(ExpressionNode* expr)
{
    if (expr->isResolveNode()) {
        ResolveNode* resolve = static_cast<ResolveNode*>(expr);
        return new (m_globalData) TypeOfResolveNode(m_globalData, resolve->identifier());
    }
    return new (m_globalData) TypeOfValueNode(m_globalData, expr);
}

ExpressionNode* ASTBuilder::makeDeleteNode(ExpressionNode* expr, int start, int divot, int end)
{
    if (!expr->isLocation())
        return new (m_globalData) DeleteValueNode(m_globalData, expr);
    if (expr->isResolveNode()) {
        ResolveNode* resolve = static_cast<ResolveNode*>(expr);
        return new (m_globalData) DeleteResolveNode(m_globalData, resolve->identifier(), divot, divot - start, end - divot);
    }
    if (expr->isBracketAccessorNode()) {
        BracketAccessorNode* bracket = static_cast<BracketAccessorNode*>(expr);
        return new (m_globalData) DeleteBracketNode(m_globalData, bracket->base(), bracket->subscript(), divot, divot - start, end - divot);
    }
    ASSERT(expr->isDotAccessorNode());
    DotAccessorNode* dot = static_cast<DotAccessorNode*>(expr);
    return new (m_globalData) DeleteDotNode(m_globalData, dot->base(), dot->identifier(), divot, divot - start, end - divot);
}

ExpressionNode* ASTBuilder::makeNegateNode(ExpressionNode* n)
{
    if (n->isNumber()) {
        NumberNode* numberNode = static_cast<NumberNode*>(n);
        numberNode->setValue(-numberNode->value());
        return numberNode;
    }

    return new (m_globalData) NegateNode(m_globalData, n);
}

ExpressionNode* ASTBuilder::makeBitwiseNotNode(ExpressionNode* expr)
{
    if (expr->isNumber())
        return createNumber(~toInt32(static_cast<NumberNode*>(expr)->value()));
    return new (m_globalData) BitwiseNotNode(m_globalData, expr);
}

ExpressionNode* ASTBuilder::makeMultNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    expr1 = expr1->stripUnaryPlus();
    expr2 = expr2->stripUnaryPlus();

    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(static_cast<NumberNode*>(expr1)->value() * static_cast<NumberNode*>(expr2)->value());

    if (expr1->isNumber() && static_cast<NumberNode*>(expr1)->value() == 1)
        return new (m_globalData) UnaryPlusNode(m_globalData, expr2);

    if (expr2->isNumber() && static_cast<NumberNode*>(expr2)->value() == 1)
        return new (m_globalData) UnaryPlusNode(m_globalData, expr1);

    return new (m_globalData) MultNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeDivNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    expr1 = expr1->stripUnaryPlus();
    expr2 = expr2->stripUnaryPlus();

    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(static_cast<NumberNode*>(expr1)->value() / static_cast<NumberNode*>(expr2)->value());
    return new (m_globalData) DivNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeModNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    expr1 = expr1->stripUnaryPlus();
    expr2 = expr2->stripUnaryPlus();
    
    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(fmod(static_cast<NumberNode*>(expr1)->value(), static_cast<NumberNode*>(expr2)->value()));
    return new (m_globalData) ModNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeAddNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(static_cast<NumberNode*>(expr1)->value() + static_cast<NumberNode*>(expr2)->value());
    return new (m_globalData) AddNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeSubNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    expr1 = expr1->stripUnaryPlus();
    expr2 = expr2->stripUnaryPlus();

    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(static_cast<NumberNode*>(expr1)->value() - static_cast<NumberNode*>(expr2)->value());
    return new (m_globalData) SubNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeLeftShiftNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(toInt32(static_cast<NumberNode*>(expr1)->value()) << (toUInt32(static_cast<NumberNode*>(expr2)->value()) & 0x1f));
    return new (m_globalData) LeftShiftNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeRightShiftNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(toInt32(static_cast<NumberNode*>(expr1)->value()) >> (toUInt32(static_cast<NumberNode*>(expr2)->value()) & 0x1f));
    return new (m_globalData) RightShiftNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeURightShiftNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(toUInt32(static_cast<NumberNode*>(expr1)->value()) >> (toUInt32(static_cast<NumberNode*>(expr2)->value()) & 0x1f));
    return new (m_globalData) UnsignedRightShiftNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeBitOrNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(toInt32(static_cast<NumberNode*>(expr1)->value()) | toInt32(static_cast<NumberNode*>(expr2)->value()));
    return new (m_globalData) BitOrNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeBitAndNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(toInt32(static_cast<NumberNode*>(expr1)->value()) & toInt32(static_cast<NumberNode*>(expr2)->value()));
    return new (m_globalData) BitAndNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeBitXOrNode(ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
{
    if (expr1->isNumber() && expr2->isNumber())
        return createNumber(toInt32(static_cast<NumberNode*>(expr1)->value()) ^ toInt32(static_cast<NumberNode*>(expr2)->value()));
    return new (m_globalData) BitXOrNode(m_globalData, expr1, expr2, rightHasAssignments);
}

ExpressionNode* ASTBuilder::makeFunctionCallNode(ExpressionNode* func, ArgumentsNode* args, int start, int divot, int end)
{
    if (!func->isLocation())
        return new (m_globalData) FunctionCallValueNode(m_globalData, func, args, divot, divot - start, end - divot);
    if (func->isResolveNode()) {
        ResolveNode* resolve = static_cast<ResolveNode*>(func);
        const Identifier& identifier = resolve->identifier();
        if (identifier == m_globalData->propertyNames->eval) {
            usesEval();
            return new (m_globalData) EvalFunctionCallNode(m_globalData, args, divot, divot - start, end - divot);
        }
        return new (m_globalData) FunctionCallResolveNode(m_globalData, identifier, args, divot, divot - start, end - divot);
    }
    if (func->isBracketAccessorNode()) {
        BracketAccessorNode* bracket = static_cast<BracketAccessorNode*>(func);
        FunctionCallBracketNode* node = new (m_globalData) FunctionCallBracketNode(m_globalData, bracket->base(), bracket->subscript(), args, divot, divot - start, end - divot);
        node->setSubexpressionInfo(bracket->divot(), bracket->endOffset());
        return node;
    }
    ASSERT(func->isDotAccessorNode());
    DotAccessorNode* dot = static_cast<DotAccessorNode*>(func);
    FunctionCallDotNode* node;
    if (dot->identifier() == m_globalData->propertyNames->call)
        node = new (m_globalData) CallFunctionCallDotNode(m_globalData, dot->base(), dot->identifier(), args, divot, divot - start, end - divot);
    else if (dot->identifier() == m_globalData->propertyNames->apply)
        node = new (m_globalData) ApplyFunctionCallDotNode(m_globalData, dot->base(), dot->identifier(), args, divot, divot - start, end - divot);
    else
        node = new (m_globalData) FunctionCallDotNode(m_globalData, dot->base(), dot->identifier(), args, divot, divot - start, end - divot);
    node->setSubexpressionInfo(dot->divot(), dot->endOffset());
    return node;
}

ExpressionNode* ASTBuilder::makeBinaryNode(int token, pair<ExpressionNode*, BinaryOpInfo> lhs, pair<ExpressionNode*, BinaryOpInfo> rhs)
{
    switch (token) {
    case OR:
        return new (m_globalData) LogicalOpNode(m_globalData, lhs.first, rhs.first, OpLogicalOr);

    case AND:
        return new (m_globalData) LogicalOpNode(m_globalData, lhs.first, rhs.first, OpLogicalAnd);

    case BITOR:
        return makeBitOrNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case BITXOR:
        return makeBitXOrNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case BITAND:
        return makeBitAndNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case EQEQ:
        return new (m_globalData) EqualNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);

    case NE:
        return new (m_globalData) NotEqualNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);

    case STREQ:
        return new (m_globalData) StrictEqualNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);

    case STRNEQ:
        return new (m_globalData) NotStrictEqualNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);

    case LT:
        return new (m_globalData) LessNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);

    case GT:
        return new (m_globalData) GreaterNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);

    case LE:
        return new (m_globalData) LessEqNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);

    case GE:
        return new (m_globalData) GreaterEqNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);

    case INSTANCEOF: {
        InstanceOfNode* node = new (m_globalData) InstanceOfNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);
        setExceptionLocation(node, lhs.second.start, rhs.second.start, rhs.second.end);
        return node;
    }

    case INTOKEN: {
        InNode* node = new (m_globalData) InNode(m_globalData, lhs.first, rhs.first, rhs.second.hasAssignment);
        setExceptionLocation(node, lhs.second.start, rhs.second.start, rhs.second.end);
        return node;
    }

    case LSHIFT:
        return makeLeftShiftNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case RSHIFT:
        return makeRightShiftNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case URSHIFT:
        return makeURightShiftNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case PLUS:
        return makeAddNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case MINUS:
        return makeSubNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case TIMES:
        return makeMultNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case DIVIDE:
        return makeDivNode(lhs.first, rhs.first, rhs.second.hasAssignment);

    case MOD:
        return makeModNode(lhs.first, rhs.first, rhs.second.hasAssignment);
    }
    CRASH();
    return 0;
}

ExpressionNode* ASTBuilder::makeAssignNode(ExpressionNode* loc, Operator op, ExpressionNode* expr, bool locHasAssignments, bool exprHasAssignments, int start, int divot, int end)
{
    usesAssignment();
    if (!loc->isLocation())
        return new (m_globalData) AssignErrorNode(m_globalData, loc, op, expr, divot, divot - start, end - divot);

    if (loc->isResolveNode()) {
        ResolveNode* resolve = static_cast<ResolveNode*>(loc);
        if (op == OpEqual) {
            AssignResolveNode* node = new (m_globalData) AssignResolveNode(m_globalData, resolve->identifier(), expr, exprHasAssignments);
            setExceptionLocation(node, start, divot, end);
            return node;
        }
        return new (m_globalData) ReadModifyResolveNode(m_globalData, resolve->identifier(), op, expr, exprHasAssignments, divot, divot - start, end - divot);
    }
    if (loc->isBracketAccessorNode()) {
        BracketAccessorNode* bracket = static_cast<BracketAccessorNode*>(loc);
        if (op == OpEqual)
            return new (m_globalData) AssignBracketNode(m_globalData, bracket->base(), bracket->subscript(), expr, locHasAssignments, exprHasAssignments, bracket->divot(), bracket->divot() - start, end - bracket->divot());
        ReadModifyBracketNode* node = new (m_globalData) ReadModifyBracketNode(m_globalData, bracket->base(), bracket->subscript(), op, expr, locHasAssignments, exprHasAssignments, divot, divot - start, end - divot);
        node->setSubexpressionInfo(bracket->divot(), bracket->endOffset());
        return node;
    }
    ASSERT(loc->isDotAccessorNode());
    DotAccessorNode* dot = static_cast<DotAccessorNode*>(loc);
    if (op == OpEqual)
        return new (m_globalData) AssignDotNode(m_globalData, dot->base(), dot->identifier(), expr, exprHasAssignments, dot->divot(), dot->divot() - start, end - dot->divot());

    ReadModifyDotNode* node = new (m_globalData) ReadModifyDotNode(m_globalData, dot->base(), dot->identifier(), op, expr, exprHasAssignments, divot, divot - start, end - divot);
    node->setSubexpressionInfo(dot->divot(), dot->endOffset());
    return node;
}

ExpressionNode* ASTBuilder::makePrefixNode(ExpressionNode* expr, Operator op, int start, int divot, int end)
{
    usesAssignment();
    if (!expr->isLocation())
        return new (m_globalData) PrefixErrorNode(m_globalData, expr, op, divot, divot - start, end - divot);

    if (expr->isResolveNode()) {
        ResolveNode* resolve = static_cast<ResolveNode*>(expr);
        return new (m_globalData) PrefixResolveNode(m_globalData, resolve->identifier(), op, divot, divot - start, end - divot);
    }
    if (expr->isBracketAccessorNode()) {
        BracketAccessorNode* bracket = static_cast<BracketAccessorNode*>(expr);
        PrefixBracketNode* node = new (m_globalData) PrefixBracketNode(m_globalData, bracket->base(), bracket->subscript(), op, divot, divot - start, end - divot);
        node->setSubexpressionInfo(bracket->divot(), bracket->startOffset());
        return node;
    }
    ASSERT(expr->isDotAccessorNode());
    DotAccessorNode* dot = static_cast<DotAccessorNode*>(expr);
    PrefixDotNode* node = new (m_globalData) PrefixDotNode(m_globalData, dot->base(), dot->identifier(), op, divot, divot - start, end - divot);
    node->setSubexpressionInfo(dot->divot(), dot->startOffset());
    return node;
}

ExpressionNode* ASTBuilder::makePostfixNode(ExpressionNode* expr, Operator op, int start, int divot, int end)
{
    usesAssignment();
    if (!expr->isLocation())
        return new (m_globalData) PostfixErrorNode(m_globalData, expr, op, divot, divot - start, end - divot);

    if (expr->isResolveNode()) {
        ResolveNode* resolve = static_cast<ResolveNode*>(expr);
        return new (m_globalData) PostfixResolveNode(m_globalData, resolve->identifier(), op, divot, divot - start, end - divot);
    }
    if (expr->isBracketAccessorNode()) {
        BracketAccessorNode* bracket = static_cast<BracketAccessorNode*>(expr);
        PostfixBracketNode* node = new (m_globalData) PostfixBracketNode(m_globalData, bracket->base(), bracket->subscript(), op, divot, divot - start, end - divot);
        node->setSubexpressionInfo(bracket->divot(), bracket->endOffset());
        return node;

    }
    ASSERT(expr->isDotAccessorNode());
    DotAccessorNode* dot = static_cast<DotAccessorNode*>(expr);
    PostfixDotNode* node = new (m_globalData) PostfixDotNode(m_globalData, dot->base(), dot->identifier(), op, divot, divot - start, end - divot);
    node->setSubexpressionInfo(dot->divot(), dot->endOffset());
    return node;
}

}

#endif
