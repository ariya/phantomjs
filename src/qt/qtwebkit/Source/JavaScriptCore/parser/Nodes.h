/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#ifndef Nodes_h
#define Nodes_h

#include "Error.h"
#include "JITCode.h"
#include "Opcode.h"
#include "ParserArena.h"
#include "ParserTokens.h"
#include "ResultType.h"
#include "SourceCode.h"
#include "SymbolTable.h"
#include <wtf/MathExtras.h>

namespace JSC {

    class ArgumentListNode;
    class BytecodeGenerator;
    class FunctionBodyNode;
    class Label;
    class PropertyListNode;
    class ReadModifyResolveNode;
    class RegisterID;
    class JSScope;
    class ScopeNode;

    enum Operator {
        OpEqual,
        OpPlusEq,
        OpMinusEq,
        OpMultEq,
        OpDivEq,
        OpPlusPlus,
        OpMinusMinus,
        OpAndEq,
        OpXOrEq,
        OpOrEq,
        OpModEq,
        OpLShift,
        OpRShift,
        OpURShift
    };
    
    enum LogicalOperator {
        OpLogicalAnd,
        OpLogicalOr
    };

    enum FallThroughMode {
        FallThroughMeansTrue = 0,
        FallThroughMeansFalse = 1
    };
    inline FallThroughMode invert(FallThroughMode fallThroughMode) { return static_cast<FallThroughMode>(!fallThroughMode); }

    typedef HashSet<RefPtr<StringImpl>, IdentifierRepHash> IdentifierSet;

    namespace DeclarationStacks {
        enum VarAttrs { IsConstant = 1, HasInitializer = 2 };
        typedef Vector<std::pair<const Identifier*, unsigned> > VarStack;
        typedef Vector<FunctionBodyNode*> FunctionStack;
    }

    struct SwitchInfo {
        enum SwitchType { SwitchNone, SwitchImmediate, SwitchCharacter, SwitchString };
        uint32_t bytecodeOffset;
        SwitchType switchType;
    };

    class ParserArenaFreeable {
    public:
        // ParserArenaFreeable objects are are freed when the arena is deleted.
        // Destructors are not called. Clients must not call delete on such objects.
        void* operator new(size_t, VM*);
    };

    class ParserArenaDeletable {
    public:
        virtual ~ParserArenaDeletable() { }

        // ParserArenaDeletable objects are deleted when the arena is deleted.
        // Clients must not call delete directly on such objects.
        void* operator new(size_t, VM*);
    };

    template <typename T>
    struct ParserArenaData : ParserArenaDeletable {
        T data;
    };

    class ParserArenaRefCounted : public RefCounted<ParserArenaRefCounted> {
    protected:
        ParserArenaRefCounted(VM*);

    public:
        virtual ~ParserArenaRefCounted()
        {
            ASSERT(deletionHasBegun());
        }
    };

    class Node : public ParserArenaFreeable {
    protected:
        Node(const JSTokenLocation&);

    public:
        virtual ~Node() { }

        int lineNo() const { return m_position.line; }
        int startOffset() const { return m_position.offset; }
        int lineStartOffset() const { return m_position.lineStartOffset; }
        const JSTextPosition& position() const { return m_position; }

    protected:
        JSTextPosition m_position;
    };

    class ExpressionNode : public Node {
    protected:
        ExpressionNode(const JSTokenLocation&, ResultType = ResultType::unknownType());

    public:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* destination = 0) = 0;

        virtual bool isNumber() const { return false; }
        virtual bool isString() const { return false; }
        virtual bool isNull() const { return false; }
        virtual bool isPure(BytecodeGenerator&) const { return false; }        
        virtual bool isConstant() const { return false; }
        virtual bool isLocation() const { return false; }
        virtual bool isResolveNode() const { return false; }
        virtual bool isBracketAccessorNode() const { return false; }
        virtual bool isDotAccessorNode() const { return false; }
        virtual bool isFuncExprNode() const { return false; }
        virtual bool isCommaNode() const { return false; }
        virtual bool isSimpleArray() const { return false; }
        virtual bool isAdd() const { return false; }
        virtual bool isSubtract() const { return false; }
        virtual bool isBoolean() const { return false; }

        virtual void emitBytecodeInConditionContext(BytecodeGenerator&, Label*, Label*, FallThroughMode);

        virtual ExpressionNode* stripUnaryPlus() { return this; }

        ResultType resultDescriptor() const { return m_resultType; }

    private:
        ResultType m_resultType;
    };

    class StatementNode : public Node {
    protected:
        StatementNode(const JSTokenLocation&);

    public:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* destination = 0) = 0;

        void setLoc(unsigned firstLine, unsigned lastLine, int startOffset, int lineStartOffset);
        unsigned firstLine() const { return lineNo(); }
        unsigned lastLine() const { return m_lastLine; }

        virtual bool isEmptyStatement() const { return false; }
        virtual bool isReturnNode() const { return false; }
        virtual bool isExprStatement() const { return false; }
        virtual bool isBreak() const { return false; }
        virtual bool isContinue() const { return false; }
        virtual bool isBlock() const { return false; }

    private:
        int m_lastLine;
    };

    class ConstantNode : public ExpressionNode {
    public:
        ConstantNode(const JSTokenLocation&, ResultType);
        virtual bool isPure(BytecodeGenerator&) const { return true; }
        virtual bool isConstant() const { return true; }
        virtual JSValue jsValue(BytecodeGenerator&) const = 0;
    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
        void emitBytecodeInConditionContext(BytecodeGenerator&, Label* trueTarget, Label* falseTarget, FallThroughMode);
    };

    class NullNode : public ConstantNode {
    public:
        NullNode(const JSTokenLocation&);

    private:
        virtual bool isNull() const { return true; }
        virtual JSValue jsValue(BytecodeGenerator&) const { return jsNull(); }
    };

    class BooleanNode : public ConstantNode {
    public:
        BooleanNode(const JSTokenLocation&, bool value);
        bool value() { return m_value; }

    private:
        virtual bool isBoolean() const { return true; }
        virtual JSValue jsValue(BytecodeGenerator&) const { return jsBoolean(m_value); }

        bool m_value;
    };

    class NumberNode : public ConstantNode {
    public:
        NumberNode(const JSTokenLocation&, double value);
        double value() { return m_value; }
        void setValue(double value) { m_value = value; }

    private:
        virtual bool isNumber() const { return true; }
        virtual JSValue jsValue(BytecodeGenerator&) const { return jsNumber(m_value); }

        double m_value;
    };

    class StringNode : public ConstantNode {
    public:
        StringNode(const JSTokenLocation&, const Identifier&);
        const Identifier& value() { return m_value; }

    private:
        virtual bool isString() const { return true; }
        virtual JSValue jsValue(BytecodeGenerator&) const;

        const Identifier& m_value;
    };
    
    class ThrowableExpressionData {
    public:
        ThrowableExpressionData()
            : m_divot(-1, -1, -1)
            , m_divotStart(-1, -1, -1)
            , m_divotEnd(-1, -1, -1)
        {
        }
        
        ThrowableExpressionData(const JSTextPosition& divot, const JSTextPosition& start, const JSTextPosition& end)
            : m_divot(divot)
            , m_divotStart(start)
            , m_divotEnd(end)
        {
            ASSERT(m_divot.offset >= m_divot.lineStartOffset);
            ASSERT(m_divotStart.offset >= m_divotStart.lineStartOffset);
            ASSERT(m_divotEnd.offset >= m_divotEnd.lineStartOffset);
        }

        void setExceptionSourceCode(const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
        {
            ASSERT(divot.offset >= divot.lineStartOffset);
            ASSERT(divotStart.offset >= divotStart.lineStartOffset);
            ASSERT(divotEnd.offset >= divotEnd.lineStartOffset);
            m_divot = divot;
            m_divotStart = divotStart;
            m_divotEnd = divotEnd;
        }

        const JSTextPosition& divot() const { return m_divot; }
        const JSTextPosition& divotStart() const { return m_divotStart; }
        const JSTextPosition& divotEnd() const { return m_divotEnd; }

    protected:
        RegisterID* emitThrowReferenceError(BytecodeGenerator&, const String& message);

    private:
        JSTextPosition m_divot;
        JSTextPosition m_divotStart;
        JSTextPosition m_divotEnd;
    };

    class ThrowableSubExpressionData : public ThrowableExpressionData {
    public:
        ThrowableSubExpressionData()
            : m_subexpressionDivotOffset(0)
            , m_subexpressionEndOffset(0)
            , m_subexpressionLineOffset(0)
            , m_subexpressionLineStartOffset(0)
        {
        }

        ThrowableSubExpressionData(const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
            : ThrowableExpressionData(divot, divotStart, divotEnd)
            , m_subexpressionDivotOffset(0)
            , m_subexpressionEndOffset(0)
            , m_subexpressionLineOffset(0)
            , m_subexpressionLineStartOffset(0)
        {
        }

        void setSubexpressionInfo(const JSTextPosition& subexpressionDivot, int subexpressionOffset)
        {
            ASSERT(subexpressionDivot.offset <= divot().offset);
            // Overflow means we can't do this safely, so just point at the primary divot,
            // divotLine, or divotLineStart.
            if ((divot() - subexpressionDivot.offset) & ~0xFFFF)
                return;
            if ((divot().line - subexpressionDivot.line) & ~0xFFFF)
                return;
            if ((divot().lineStartOffset - subexpressionDivot.lineStartOffset) & ~0xFFFF)
                return;
            if ((divotEnd() - subexpressionOffset) & ~0xFFFF)
                return;
            m_subexpressionDivotOffset = divot() - subexpressionDivot.offset;
            m_subexpressionEndOffset = divotEnd() - subexpressionOffset;
            m_subexpressionLineOffset = divot().line - subexpressionDivot.line;
            m_subexpressionLineStartOffset = divot().lineStartOffset - subexpressionDivot.lineStartOffset;
        }

        JSTextPosition subexpressionDivot()
        {
            int newLine = divot().line - m_subexpressionLineOffset;
            int newOffset = divot().offset - m_subexpressionDivotOffset;
            int newLineStartOffset = divot().lineStartOffset - m_subexpressionLineStartOffset;
            return JSTextPosition(newLine, newOffset, newLineStartOffset);
        }
        JSTextPosition subexpressionStart() { return divotStart(); }
        JSTextPosition subexpressionEnd() { return divotEnd() - static_cast<int>(m_subexpressionEndOffset); }

    protected:
        uint16_t m_subexpressionDivotOffset;
        uint16_t m_subexpressionEndOffset;
        uint16_t m_subexpressionLineOffset;
        uint16_t m_subexpressionLineStartOffset;
    };
    
    class ThrowablePrefixedSubExpressionData : public ThrowableExpressionData {
    public:
        ThrowablePrefixedSubExpressionData()
            : m_subexpressionDivotOffset(0)
            , m_subexpressionStartOffset(0)
            , m_subexpressionLineOffset(0)
            , m_subexpressionLineStartOffset(0)
        {
        }

        ThrowablePrefixedSubExpressionData(const JSTextPosition& divot, const JSTextPosition& start, const JSTextPosition& end)
            : ThrowableExpressionData(divot, start, end)
            , m_subexpressionDivotOffset(0)
            , m_subexpressionStartOffset(0)
            , m_subexpressionLineOffset(0)
            , m_subexpressionLineStartOffset(0)
        {
        }

        void setSubexpressionInfo(const JSTextPosition& subexpressionDivot, int subexpressionOffset)
        {
            ASSERT(subexpressionDivot.offset >= divot().offset);
            // Overflow means we can't do this safely, so just point at the primary divot,
            // divotLine, or divotLineStart.
            if ((subexpressionDivot.offset - divot()) & ~0xFFFF) 
                return;
            if ((subexpressionDivot.line - divot().line) & ~0xFFFF)
                return;
            if ((subexpressionDivot.lineStartOffset - divot().lineStartOffset) & ~0xFFFF)
                return;
            if ((subexpressionOffset - divotStart()) & ~0xFFFF) 
                return;
            m_subexpressionDivotOffset = subexpressionDivot.offset - divot();
            m_subexpressionStartOffset = subexpressionOffset - divotStart();
            m_subexpressionLineOffset = subexpressionDivot.line - divot().line;
            m_subexpressionLineStartOffset = subexpressionDivot.lineStartOffset - divot().lineStartOffset;
        }

        JSTextPosition subexpressionDivot()
        {
            int newLine = divot().line + m_subexpressionLineOffset;
            int newOffset = divot().offset + m_subexpressionDivotOffset;
            int newLineStartOffset = divot().lineStartOffset + m_subexpressionLineStartOffset;
            return JSTextPosition(newLine, newOffset, newLineStartOffset);
        }
        JSTextPosition subexpressionStart() { return divotStart() + static_cast<int>(m_subexpressionStartOffset); }
        JSTextPosition subexpressionEnd() { return divotEnd(); }

    protected:
        uint16_t m_subexpressionDivotOffset;
        uint16_t m_subexpressionStartOffset;
        uint16_t m_subexpressionLineOffset;
        uint16_t m_subexpressionLineStartOffset;
    };

    class RegExpNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        RegExpNode(const JSTokenLocation&, const Identifier& pattern, const Identifier& flags);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_pattern;
        const Identifier& m_flags;
    };

    class ThisNode : public ExpressionNode {
    public:
        ThisNode(const JSTokenLocation&);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };

    class ResolveNode : public ExpressionNode {
    public:
        ResolveNode(const JSTokenLocation&, const Identifier&, const JSTextPosition& start);

        const Identifier& identifier() const { return m_ident; }

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        virtual bool isPure(BytecodeGenerator&) const ;
        virtual bool isLocation() const { return true; }
        virtual bool isResolveNode() const { return true; }

        const Identifier& m_ident;
        JSTextPosition m_start;
    };

    class ElementNode : public ParserArenaFreeable {
    public:
        ElementNode(int elision, ExpressionNode*);
        ElementNode(ElementNode*, int elision, ExpressionNode*);

        int elision() const { return m_elision; }
        ExpressionNode* value() { return m_node; }
        ElementNode* next() { return m_next; }

    private:
        ElementNode* m_next;
        int m_elision;
        ExpressionNode* m_node;
    };

    class ArrayNode : public ExpressionNode {
    public:
        ArrayNode(const JSTokenLocation&, int elision);
        ArrayNode(const JSTokenLocation&, ElementNode*);
        ArrayNode(const JSTokenLocation&, int elision, ElementNode*);

        ArgumentListNode* toArgumentList(VM*, int, int) const;

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        virtual bool isSimpleArray() const ;

        ElementNode* m_element;
        int m_elision;
        bool m_optional;
    };

    class PropertyNode : public ParserArenaFreeable {
    public:
        enum Type { Constant = 1, Getter = 2, Setter = 4 };

        PropertyNode(VM*, const Identifier&, ExpressionNode*, Type);
        PropertyNode(VM*, double, ExpressionNode*, Type);

        const Identifier& name() const { return m_name; }
        Type type() const { return m_type; }

    private:
        friend class PropertyListNode;
        const Identifier& m_name;
        ExpressionNode* m_assign;
        Type m_type;
    };

    class PropertyListNode : public ExpressionNode {
    public:
        PropertyListNode(const JSTokenLocation&, PropertyNode*);
        PropertyListNode(const JSTokenLocation&, PropertyNode*, PropertyListNode*);

        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

    private:
        PropertyNode* m_node;
        PropertyListNode* m_next;
    };

    class ObjectLiteralNode : public ExpressionNode {
    public:
        ObjectLiteralNode(const JSTokenLocation&);
        ObjectLiteralNode(const JSTokenLocation&, PropertyListNode*);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        PropertyListNode* m_list;
    };
    
    class BracketAccessorNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        BracketAccessorNode(const JSTokenLocation&, ExpressionNode* base, ExpressionNode* subscript, bool subscriptHasAssignments);

        ExpressionNode* base() const { return m_base; }
        ExpressionNode* subscript() const { return m_subscript; }

        bool subscriptHasAssignments() const { return m_subscriptHasAssignments; }

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        virtual bool isLocation() const { return true; }
        virtual bool isBracketAccessorNode() const { return true; }

        ExpressionNode* m_base;
        ExpressionNode* m_subscript;
        bool m_subscriptHasAssignments;
    };

    class DotAccessorNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        DotAccessorNode(const JSTokenLocation&, ExpressionNode* base, const Identifier&);

        ExpressionNode* base() const { return m_base; }
        const Identifier& identifier() const { return m_ident; }

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        virtual bool isLocation() const { return true; }
        virtual bool isDotAccessorNode() const { return true; }

        ExpressionNode* m_base;
        const Identifier& m_ident;
    };

    class ArgumentListNode : public ExpressionNode {
    public:
        ArgumentListNode(const JSTokenLocation&, ExpressionNode*);
        ArgumentListNode(const JSTokenLocation&, ArgumentListNode*, ExpressionNode*);

        ArgumentListNode* m_next;
        ExpressionNode* m_expr;

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };

    class ArgumentsNode : public ParserArenaFreeable {
    public:
        ArgumentsNode();
        ArgumentsNode(ArgumentListNode*);

        ArgumentListNode* m_listNode;
    };

    class NewExprNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        NewExprNode(const JSTokenLocation&, ExpressionNode*);
        NewExprNode(const JSTokenLocation&, ExpressionNode*, ArgumentsNode*);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
        ArgumentsNode* m_args;
    };

    class EvalFunctionCallNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        EvalFunctionCallNode(const JSTokenLocation&, ArgumentsNode*, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ArgumentsNode* m_args;
    };

    class FunctionCallValueNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        FunctionCallValueNode(const JSTokenLocation&, ExpressionNode*, ArgumentsNode*, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
        ArgumentsNode* m_args;
    };

    class FunctionCallResolveNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        FunctionCallResolveNode(const JSTokenLocation&, const Identifier&, ArgumentsNode*, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_ident;
        ArgumentsNode* m_args;
    };
    
    class FunctionCallBracketNode : public ExpressionNode, public ThrowableSubExpressionData {
    public:
        FunctionCallBracketNode(const JSTokenLocation&, ExpressionNode* base, ExpressionNode* subscript, ArgumentsNode*, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_base;
        ExpressionNode* m_subscript;
        ArgumentsNode* m_args;
    };

    class FunctionCallDotNode : public ExpressionNode, public ThrowableSubExpressionData {
    public:
        FunctionCallDotNode(const JSTokenLocation&, ExpressionNode* base, const Identifier&, ArgumentsNode*, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

    protected:
        ExpressionNode* m_base;
        const Identifier& m_ident;
        ArgumentsNode* m_args;
    };

    class CallFunctionCallDotNode : public FunctionCallDotNode {
    public:
        CallFunctionCallDotNode(const JSTokenLocation&, ExpressionNode* base, const Identifier&, ArgumentsNode*, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };
    
    class ApplyFunctionCallDotNode : public FunctionCallDotNode {
    public:
        ApplyFunctionCallDotNode(const JSTokenLocation&, ExpressionNode* base, const Identifier&, ArgumentsNode*, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };

    class DeleteResolveNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        DeleteResolveNode(const JSTokenLocation&, const Identifier&, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_ident;
    };

    class DeleteBracketNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        DeleteBracketNode(const JSTokenLocation&, ExpressionNode* base, ExpressionNode* subscript, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_base;
        ExpressionNode* m_subscript;
    };

    class DeleteDotNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        DeleteDotNode(const JSTokenLocation&, ExpressionNode* base, const Identifier&, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_base;
        const Identifier& m_ident;
    };

    class DeleteValueNode : public ExpressionNode {
    public:
        DeleteValueNode(const JSTokenLocation&, ExpressionNode*);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
    };

    class VoidNode : public ExpressionNode {
    public:
        VoidNode(const JSTokenLocation&, ExpressionNode*);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
    };

    class TypeOfResolveNode : public ExpressionNode {
    public:
        TypeOfResolveNode(const JSTokenLocation&, const Identifier&);

        const Identifier& identifier() const { return m_ident; }

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_ident;
    };

    class TypeOfValueNode : public ExpressionNode {
    public:
        TypeOfValueNode(const JSTokenLocation&, ExpressionNode*);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
    };

    class PrefixNode : public ExpressionNode, public ThrowablePrefixedSubExpressionData {
    public:
        PrefixNode(const JSTokenLocation&, ExpressionNode*, Operator, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    protected:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
        virtual RegisterID* emitResolve(BytecodeGenerator&, RegisterID* = 0);
        virtual RegisterID* emitBracket(BytecodeGenerator&, RegisterID* = 0);
        virtual RegisterID* emitDot(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
        Operator m_operator;
    };

    class PostfixNode : public PrefixNode {
    public:
        PostfixNode(const JSTokenLocation&, ExpressionNode*, Operator, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
        virtual RegisterID* emitResolve(BytecodeGenerator&, RegisterID* = 0);
        virtual RegisterID* emitBracket(BytecodeGenerator&, RegisterID* = 0);
        virtual RegisterID* emitDot(BytecodeGenerator&, RegisterID* = 0);
    };

    class UnaryOpNode : public ExpressionNode {
    public:
        UnaryOpNode(const JSTokenLocation&, ResultType, ExpressionNode*, OpcodeID);

    protected:
        ExpressionNode* expr() { return m_expr; }
        const ExpressionNode* expr() const { return m_expr; }

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        OpcodeID opcodeID() const { return m_opcodeID; }

        ExpressionNode* m_expr;
        OpcodeID m_opcodeID;
    };

    class UnaryPlusNode : public UnaryOpNode {
    public:
        UnaryPlusNode(const JSTokenLocation&, ExpressionNode*);

    private:
        virtual ExpressionNode* stripUnaryPlus() { return expr(); }
    };

    class NegateNode : public UnaryOpNode {
    public:
        NegateNode(const JSTokenLocation&, ExpressionNode*);
    };

    class BitwiseNotNode : public ExpressionNode {
    public:
        BitwiseNotNode(const JSTokenLocation&, ExpressionNode*);

    protected:
        ExpressionNode* expr() { return m_expr; }
        const ExpressionNode* expr() const { return m_expr; }

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
    };
 
    class LogicalNotNode : public UnaryOpNode {
    public:
        LogicalNotNode(const JSTokenLocation&, ExpressionNode*);
    private:
        void emitBytecodeInConditionContext(BytecodeGenerator&, Label* trueTarget, Label* falseTarget, FallThroughMode);
    };

    class BinaryOpNode : public ExpressionNode {
    public:
        BinaryOpNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID, bool rightHasAssignments);
        BinaryOpNode(const JSTokenLocation&, ResultType, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID, bool rightHasAssignments);

        RegisterID* emitStrcat(BytecodeGenerator& generator, RegisterID* destination, RegisterID* lhs = 0, ReadModifyResolveNode* emitExpressionInfoForMe = 0);
        void emitBytecodeInConditionContext(BytecodeGenerator&, Label* trueTarget, Label* falseTarget, FallThroughMode);

        ExpressionNode* lhs() { return m_expr1; };
        ExpressionNode* rhs() { return m_expr2; };

    private:
        void tryFoldToBranch(BytecodeGenerator&, TriState& branchCondition, ExpressionNode*& branchExpression);
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

    protected:
        OpcodeID opcodeID() const { return m_opcodeID; }

    protected:
        ExpressionNode* m_expr1;
        ExpressionNode* m_expr2;
    private:
        OpcodeID m_opcodeID;
    protected:
        bool m_rightHasAssignments;
    };

    class MultNode : public BinaryOpNode {
    public:
        MultNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class DivNode : public BinaryOpNode {
    public:
        DivNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class ModNode : public BinaryOpNode {
    public:
        ModNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class AddNode : public BinaryOpNode {
    public:
        AddNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);

        virtual bool isAdd() const { return true; }
    };

    class SubNode : public BinaryOpNode {
    public:
        SubNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);

        virtual bool isSubtract() const { return true; }
    };

    class LeftShiftNode : public BinaryOpNode {
    public:
        LeftShiftNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class RightShiftNode : public BinaryOpNode {
    public:
        RightShiftNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class UnsignedRightShiftNode : public BinaryOpNode {
    public:
        UnsignedRightShiftNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class LessNode : public BinaryOpNode {
    public:
        LessNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class GreaterNode : public BinaryOpNode {
    public:
        GreaterNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class LessEqNode : public BinaryOpNode {
    public:
        LessEqNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class GreaterEqNode : public BinaryOpNode {
    public:
        GreaterEqNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class ThrowableBinaryOpNode : public BinaryOpNode, public ThrowableExpressionData {
    public:
        ThrowableBinaryOpNode(const JSTokenLocation&, ResultType, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID, bool rightHasAssignments);
        ThrowableBinaryOpNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID, bool rightHasAssignments);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };
    
    class InstanceOfNode : public ThrowableBinaryOpNode {
    public:
        InstanceOfNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };

    class InNode : public ThrowableBinaryOpNode {
    public:
        InNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class EqualNode : public BinaryOpNode {
    public:
        EqualNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };

    class NotEqualNode : public BinaryOpNode {
    public:
        NotEqualNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class StrictEqualNode : public BinaryOpNode {
    public:
        StrictEqualNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };

    class NotStrictEqualNode : public BinaryOpNode {
    public:
        NotStrictEqualNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class BitAndNode : public BinaryOpNode {
    public:
        BitAndNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class BitOrNode : public BinaryOpNode {
    public:
        BitOrNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    class BitXOrNode : public BinaryOpNode {
    public:
        BitXOrNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments);
    };

    // m_expr1 && m_expr2, m_expr1 || m_expr2
    class LogicalOpNode : public ExpressionNode {
    public:
        LogicalOpNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, LogicalOperator);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
        void emitBytecodeInConditionContext(BytecodeGenerator&, Label* trueTarget, Label* falseTarget, FallThroughMode);

        ExpressionNode* m_expr1;
        ExpressionNode* m_expr2;
        LogicalOperator m_operator;
    };

    // The ternary operator, "m_logical ? m_expr1 : m_expr2"
    class ConditionalNode : public ExpressionNode {
    public:
        ConditionalNode(const JSTokenLocation&, ExpressionNode* logical, ExpressionNode* expr1, ExpressionNode* expr2);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_logical;
        ExpressionNode* m_expr1;
        ExpressionNode* m_expr2;
    };

    class ReadModifyResolveNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        ReadModifyResolveNode(const JSTokenLocation&, const Identifier&, Operator, ExpressionNode*  right, bool rightHasAssignments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_ident;
        ExpressionNode* m_right;
        Operator m_operator;
        bool m_rightHasAssignments;
    };

    class AssignResolveNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        AssignResolveNode(const JSTokenLocation&, const Identifier&, ExpressionNode* right);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_ident;
        ExpressionNode* m_right;
    };

    class ReadModifyBracketNode : public ExpressionNode, public ThrowableSubExpressionData {
    public:
        ReadModifyBracketNode(const JSTokenLocation&, ExpressionNode* base, ExpressionNode* subscript, Operator, ExpressionNode* right, bool subscriptHasAssignments, bool rightHasAssignments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_base;
        ExpressionNode* m_subscript;
        ExpressionNode* m_right;
        Operator m_operator : 30;
        bool m_subscriptHasAssignments : 1;
        bool m_rightHasAssignments : 1;
    };

    class AssignBracketNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        AssignBracketNode(const JSTokenLocation&, ExpressionNode* base, ExpressionNode* subscript, ExpressionNode* right, bool subscriptHasAssignments, bool rightHasAssignments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_base;
        ExpressionNode* m_subscript;
        ExpressionNode* m_right;
        bool m_subscriptHasAssignments : 1;
        bool m_rightHasAssignments : 1;
    };

    class AssignDotNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        AssignDotNode(const JSTokenLocation&, ExpressionNode* base, const Identifier&, ExpressionNode* right, bool rightHasAssignments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_base;
        const Identifier& m_ident;
        ExpressionNode* m_right;
        bool m_rightHasAssignments;
    };

    class ReadModifyDotNode : public ExpressionNode, public ThrowableSubExpressionData {
    public:
        ReadModifyDotNode(const JSTokenLocation&, ExpressionNode* base, const Identifier&, Operator, ExpressionNode* right, bool rightHasAssignments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_base;
        const Identifier& m_ident;
        ExpressionNode* m_right;
        Operator m_operator : 31;
        bool m_rightHasAssignments : 1;
    };

    class AssignErrorNode : public ExpressionNode, public ThrowableExpressionData {
    public:
        AssignErrorNode(const JSTokenLocation&, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };
    
    typedef Vector<ExpressionNode*, 8> ExpressionVector;

    class CommaNode : public ExpressionNode, public ParserArenaDeletable {
    public:
        CommaNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2);

        using ParserArenaDeletable::operator new;

        void append(ExpressionNode* expr) { m_expressions.append(expr); }

    private:
        virtual bool isCommaNode() const { return true; }
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionVector m_expressions;
    };
    
    class ConstDeclNode : public ExpressionNode {
    public:
        ConstDeclNode(const JSTokenLocation&, const Identifier&, ExpressionNode*);

        bool hasInitializer() const { return m_init; }
        const Identifier& ident() { return m_ident; }

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);
        virtual RegisterID* emitCodeSingle(BytecodeGenerator&);

        const Identifier& m_ident;

    public:
        ConstDeclNode* m_next;

    private:
        ExpressionNode* m_init;
    };

    class ConstStatementNode : public StatementNode {
    public:
        ConstStatementNode(const JSTokenLocation&, ConstDeclNode* next);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ConstDeclNode* m_next;
    };

    class SourceElements : public ParserArenaDeletable {
    public:
        SourceElements();

        void append(StatementNode*);

        StatementNode* singleStatement() const;
        StatementNode* lastStatement() const;

        void emitBytecode(BytecodeGenerator&, RegisterID* destination);

    private:
        Vector<StatementNode*> m_statements;
    };

    class BlockNode : public StatementNode {
    public:
        BlockNode(const JSTokenLocation&, SourceElements* = 0);

        StatementNode* singleStatement() const;
        StatementNode* lastStatement() const;

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        virtual bool isBlock() const { return true; }

        SourceElements* m_statements;
    };

    class EmptyStatementNode : public StatementNode {
    public:
        EmptyStatementNode(const JSTokenLocation&);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        virtual bool isEmptyStatement() const { return true; }
    };
    
    class DebuggerStatementNode : public StatementNode {
    public:
        DebuggerStatementNode(const JSTokenLocation&);
        
    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };

    class ExprStatementNode : public StatementNode {
    public:
        ExprStatementNode(const JSTokenLocation&, ExpressionNode*);

        ExpressionNode* expr() const { return m_expr; }

    private:
        virtual bool isExprStatement() const { return true; }

        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
    };

    class VarStatementNode : public StatementNode {
    public:
        VarStatementNode(const JSTokenLocation&, ExpressionNode*);
    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
    };

    class IfElseNode : public StatementNode {
    public:
        IfElseNode(const JSTokenLocation&, ExpressionNode* condition, StatementNode* ifBlock, StatementNode* elseBlock);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);
        bool tryFoldBreakAndContinue(BytecodeGenerator&, StatementNode* ifBlock,
            Label*& trueTarget, FallThroughMode&);

        ExpressionNode* m_condition;
        StatementNode* m_ifBlock;
        StatementNode* m_elseBlock;
    };

    class DoWhileNode : public StatementNode {
    public:
        DoWhileNode(const JSTokenLocation&, StatementNode*, ExpressionNode*);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        StatementNode* m_statement;
        ExpressionNode* m_expr;
    };

    class WhileNode : public StatementNode {
    public:
        WhileNode(const JSTokenLocation&, ExpressionNode*, StatementNode*);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
        StatementNode* m_statement;
    };

    class ForNode : public StatementNode {
    public:
        ForNode(const JSTokenLocation&, ExpressionNode* expr1, ExpressionNode* expr2, ExpressionNode* expr3, StatementNode*);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr1;
        ExpressionNode* m_expr2;
        ExpressionNode* m_expr3;
        StatementNode* m_statement;
    };

    class ForInNode : public StatementNode, public ThrowableExpressionData {
    public:
        ForInNode(const JSTokenLocation&, ExpressionNode*, ExpressionNode*, StatementNode*);
        ForInNode(VM*, const JSTokenLocation&, const Identifier&, ExpressionNode*, ExpressionNode*, StatementNode*, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_init;
        ExpressionNode* m_lexpr;
        ExpressionNode* m_expr;
        StatementNode* m_statement;
    };

    class ContinueNode : public StatementNode, public ThrowableExpressionData {
    public:
        ContinueNode(VM*, const JSTokenLocation&);
        ContinueNode(const JSTokenLocation&, const Identifier&);
        Label* trivialTarget(BytecodeGenerator&);
        
    private:
        virtual bool isContinue() const { return true; }
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_ident;
    };

    class BreakNode : public StatementNode, public ThrowableExpressionData {
    public:
        BreakNode(VM*, const JSTokenLocation&);
        BreakNode(const JSTokenLocation&, const Identifier&);
        Label* trivialTarget(BytecodeGenerator&);
        
    private:
        virtual bool isBreak() const { return true; }
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_ident;
    };

    class ReturnNode : public StatementNode, public ThrowableExpressionData {
    public:
        ReturnNode(const JSTokenLocation&, ExpressionNode* value);

        ExpressionNode* value() { return m_value; }

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        virtual bool isReturnNode() const { return true; }

        ExpressionNode* m_value;
    };

    class WithNode : public StatementNode {
    public:
        WithNode(const JSTokenLocation&, ExpressionNode*, StatementNode*, const JSTextPosition& divot, uint32_t expressionLength);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
        StatementNode* m_statement;
        JSTextPosition m_divot;
        uint32_t m_expressionLength;
    };

    class LabelNode : public StatementNode, public ThrowableExpressionData {
    public:
        LabelNode(const JSTokenLocation&, const Identifier& name, StatementNode*);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        const Identifier& m_name;
        StatementNode* m_statement;
    };

    class ThrowNode : public StatementNode, public ThrowableExpressionData {
    public:
        ThrowNode(const JSTokenLocation&, ExpressionNode*);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
    };

    class TryNode : public StatementNode {
    public:
        TryNode(const JSTokenLocation&, StatementNode* tryBlock, const Identifier& exceptionIdent, StatementNode* catchBlock, StatementNode* finallyBlock);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        StatementNode* m_tryBlock;
        const Identifier& m_exceptionIdent;
        StatementNode* m_catchBlock;
        StatementNode* m_finallyBlock;
    };

    class ParameterNode : public ParserArenaFreeable {
    public:
        ParameterNode(const Identifier&);
        ParameterNode(ParameterNode*, const Identifier&);

        const Identifier& ident() const { return m_ident; }
        ParameterNode* nextParam() const { return m_next; }

    private:
        const Identifier& m_ident;
        ParameterNode* m_next;
    };

    class ScopeNode : public StatementNode, public ParserArenaRefCounted {
    public:
        typedef DeclarationStacks::VarStack VarStack;
        typedef DeclarationStacks::FunctionStack FunctionStack;

        ScopeNode(VM*, const JSTokenLocation& start, const JSTokenLocation& end, bool inStrictContext);
        ScopeNode(VM*, const JSTokenLocation& start, const JSTokenLocation& end, const SourceCode&, SourceElements*, VarStack*, FunctionStack*, IdentifierSet&, CodeFeatures, int numConstants);

        using ParserArenaRefCounted::operator new;

        void destroyData()
        {
            m_arena.reset();
            m_varStack.clear();
            m_functionStack.clear();
            m_statements = 0;
            m_capturedVariables.clear();
        }

        const SourceCode& source() const { return m_source; }
        const String& sourceURL() const { return m_source.provider()->url(); }
        intptr_t sourceID() const { return m_source.providerID(); }

        int startLine() const { return m_startLineNumber; }
        int startStartOffset() const { return m_startStartOffset; }
        int startLineStartOffset() const { return m_startLineStartOffset; }

        void setFeatures(CodeFeatures features) { m_features = features; }
        CodeFeatures features() { return m_features; }

        bool usesEval() const { return m_features & EvalFeature; }
        bool usesArguments() const { return (m_features & ArgumentsFeature) && !(m_features & ShadowsArgumentsFeature); }
        bool isStrictMode() const { return m_features & StrictModeFeature; }
        void setUsesArguments() { m_features |= ArgumentsFeature; }
        bool usesThis() const { return m_features & ThisFeature; }
        bool needsActivationForMoreThanVariables() const { return m_features & (EvalFeature | WithFeature | CatchFeature); }
        bool needsActivation() const { return (hasCapturedVariables()) || (m_features & (EvalFeature | WithFeature | CatchFeature)); }
        bool hasCapturedVariables() const { return !!m_capturedVariables.size(); }
        size_t capturedVariableCount() const { return m_capturedVariables.size(); }
        bool captures(const Identifier& ident) { return m_capturedVariables.contains(ident.impl()); }

        VarStack& varStack() { return m_varStack; }
        FunctionStack& functionStack() { return m_functionStack; }

        int neededConstants()
        {
            // We may need 2 more constants than the count given by the parser,
            // because of the various uses of jsUndefined() and jsNull().
            return m_numConstants + 2;
        }

        StatementNode* singleStatement() const;

        void emitStatementsBytecode(BytecodeGenerator&, RegisterID* destination);

    protected:
        void setSource(const SourceCode& source) { m_source = source; }
        ParserArena m_arena;

        int m_startLineNumber;
        unsigned m_startStartOffset;
        unsigned m_startLineStartOffset;

    private:
        CodeFeatures m_features;
        SourceCode m_source;
        VarStack m_varStack;
        FunctionStack m_functionStack;
        int m_numConstants;
        SourceElements* m_statements;
        IdentifierSet m_capturedVariables;
    };

    class ProgramNode : public ScopeNode {
    public:
        static const bool isFunctionNode = false;
        static PassRefPtr<ProgramNode> create(VM*, const JSTokenLocation& start, const JSTokenLocation& end, unsigned startColumn, SourceElements*, VarStack*, FunctionStack*, IdentifierSet&, const SourceCode&, CodeFeatures, int numConstants);

        unsigned startColumn() { return m_startColumn; }

        static const bool scopeIsFunction = false;

    private:
        ProgramNode(VM*, const JSTokenLocation& start, const JSTokenLocation& end, unsigned startColumn, SourceElements*, VarStack*, FunctionStack*, IdentifierSet&, const SourceCode&, CodeFeatures, int numConstants);

        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        unsigned m_startColumn;
    };

    class EvalNode : public ScopeNode {
    public:
        static const bool isFunctionNode = false;
        static PassRefPtr<EvalNode> create(VM*, const JSTokenLocation& start, const JSTokenLocation& end, unsigned, SourceElements*, VarStack*, FunctionStack*, IdentifierSet&, const SourceCode&, CodeFeatures, int numConstants);

        unsigned startColumn() { return 1; }

        static const bool scopeIsFunction = false;

    private:
        EvalNode(VM*, const JSTokenLocation& start, const JSTokenLocation& end, SourceElements*, VarStack*, FunctionStack*, IdentifierSet&, const SourceCode&, CodeFeatures, int numConstants);

        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);
    };

    class FunctionParameters : public RefCounted<FunctionParameters> {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        static PassRefPtr<FunctionParameters> create(ParameterNode*);
        ~FunctionParameters();

        unsigned size() const { return m_size; }
        const Identifier& at(unsigned index) const { ASSERT(index < m_size); return identifiers()[index]; }

    private:
        FunctionParameters(ParameterNode*, unsigned size);

        Identifier* identifiers() { return reinterpret_cast<Identifier*>(&m_storage); }
        const Identifier* identifiers() const { return reinterpret_cast<const Identifier*>(&m_storage); }

        unsigned m_size;
        void* m_storage;
    };

    class FunctionBodyNode : public ScopeNode {
    public:
        static const bool isFunctionNode = true;
        static FunctionBodyNode* create(VM*, const JSTokenLocation& start, const JSTokenLocation& end, unsigned startColumn, bool isStrictMode);
        static PassRefPtr<FunctionBodyNode> create(VM*, const JSTokenLocation& start, const JSTokenLocation& end, unsigned startColumn, SourceElements*, VarStack*, FunctionStack*, IdentifierSet&, const SourceCode&, CodeFeatures, int numConstants);

        FunctionParameters* parameters() const { return m_parameters.get(); }
        size_t parameterCount() const { return m_parameters->size(); }

        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        void finishParsing(const SourceCode&, ParameterNode*, const Identifier&, FunctionNameIsInScopeToggle);
        void finishParsing(PassRefPtr<FunctionParameters>, const Identifier&, FunctionNameIsInScopeToggle);
        
        const Identifier& ident() { return m_ident; }
        void setInferredName(const Identifier& inferredName) { ASSERT(!inferredName.isNull()); m_inferredName = inferredName; }
        const Identifier& inferredName() { return m_inferredName.isEmpty() ? m_ident : m_inferredName; }

        bool functionNameIsInScope() { return m_functionNameIsInScopeToggle == FunctionNameIsInScope; }
        FunctionNameIsInScopeToggle functionNameIsInScopeToggle() { return m_functionNameIsInScopeToggle; }

        void setFunctionStart(int functionStart) { m_functionStart = functionStart; }
        int functionStart() const { return m_functionStart; }
        unsigned startColumn() const { return m_startColumn; }

        static const bool scopeIsFunction = true;

    private:
        FunctionBodyNode(VM*, const JSTokenLocation& start, const JSTokenLocation& end, unsigned startColumn, bool inStrictContext);
        FunctionBodyNode(VM*, const JSTokenLocation& start, const JSTokenLocation& end, unsigned startColumn, SourceElements*, VarStack*, FunctionStack*, IdentifierSet&, const SourceCode&, CodeFeatures, int numConstants);

        Identifier m_ident;
        Identifier m_inferredName;
        FunctionNameIsInScopeToggle m_functionNameIsInScopeToggle;
        RefPtr<FunctionParameters> m_parameters;
        int m_functionStart;
        unsigned m_startColumn;
    };

    class FuncExprNode : public ExpressionNode {
    public:
        FuncExprNode(const JSTokenLocation&, const Identifier&, FunctionBodyNode*, const SourceCode&, ParameterNode* = 0);

        FunctionBodyNode* body() { return m_body; }

    private:
        virtual RegisterID* emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        virtual bool isFuncExprNode() const { return true; } 

        FunctionBodyNode* m_body;
    };

    class FuncDeclNode : public StatementNode {
    public:
        FuncDeclNode(const JSTokenLocation&, const Identifier&, FunctionBodyNode*, const SourceCode&, ParameterNode* = 0);

        FunctionBodyNode* body() { return m_body; }

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        FunctionBodyNode* m_body;
    };

    class CaseClauseNode : public ParserArenaFreeable {
    public:
        CaseClauseNode(ExpressionNode*, SourceElements* = 0);

        ExpressionNode* expr() const { return m_expr; }

        void emitBytecode(BytecodeGenerator&, RegisterID* destination);

    private:
        ExpressionNode* m_expr;
        SourceElements* m_statements;
    };

    class ClauseListNode : public ParserArenaFreeable {
    public:
        ClauseListNode(CaseClauseNode*);
        ClauseListNode(ClauseListNode*, CaseClauseNode*);

        CaseClauseNode* getClause() const { return m_clause; }
        ClauseListNode* getNext() const { return m_next; }

    private:
        CaseClauseNode* m_clause;
        ClauseListNode* m_next;
    };

    class CaseBlockNode : public ParserArenaFreeable {
    public:
        CaseBlockNode(ClauseListNode* list1, CaseClauseNode* defaultClause, ClauseListNode* list2);

        void emitBytecodeForBlock(BytecodeGenerator&, RegisterID* input, RegisterID* destination);

    private:
        SwitchInfo::SwitchType tryTableSwitch(Vector<ExpressionNode*, 8>& literalVector, int32_t& min_num, int32_t& max_num);
        static const size_t s_tableSwitchMinimum = 10;
        ClauseListNode* m_list1;
        CaseClauseNode* m_defaultClause;
        ClauseListNode* m_list2;
    };

    class SwitchNode : public StatementNode {
    public:
        SwitchNode(const JSTokenLocation&, ExpressionNode*, CaseBlockNode*);

    private:
        virtual void emitBytecode(BytecodeGenerator&, RegisterID* = 0);

        ExpressionNode* m_expr;
        CaseBlockNode* m_block;
    };

    struct ElementList {
        ElementNode* head;
        ElementNode* tail;
    };

    struct PropertyList {
        PropertyListNode* head;
        PropertyListNode* tail;
    };

    struct ArgumentList {
        ArgumentListNode* head;
        ArgumentListNode* tail;
    };

    struct ConstDeclList {
        ConstDeclNode* head;
        ConstDeclNode* tail;
    };

    struct ParameterList {
        ParameterNode* head;
        ParameterNode* tail;
    };

    struct ClauseList {
        ClauseListNode* head;
        ClauseListNode* tail;
    };

} // namespace JSC

#endif // Nodes_h
