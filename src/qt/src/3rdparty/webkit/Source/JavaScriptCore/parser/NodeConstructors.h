/*
 *  Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef NodeConstructors_h
#define NodeConstructors_h

#include "Nodes.h"
#include "Lexer.h"
#include "Parser.h"

namespace JSC {

    inline void* ParserArenaFreeable::operator new(size_t size, JSGlobalData* globalData)
    {
        return globalData->parser->arena().allocateFreeable(size);
    }

    inline void* ParserArenaDeletable::operator new(size_t size, JSGlobalData* globalData)
    {
        return globalData->parser->arena().allocateDeletable(size);
    }

    inline ParserArenaRefCounted::ParserArenaRefCounted(JSGlobalData* globalData)
    {
        globalData->parser->arena().derefWithArena(adoptRef(this));
    }

    inline Node::Node(JSGlobalData* globalData)
        : m_line(globalData->lexer->lastLineNumber())
    {
    }

    inline ExpressionNode::ExpressionNode(JSGlobalData* globalData, ResultType resultType)
        : Node(globalData)
        , m_resultType(resultType)
    {
    }

    inline StatementNode::StatementNode(JSGlobalData* globalData)
        : Node(globalData)
        , m_lastLine(-1)
    {
    }

    inline NullNode::NullNode(JSGlobalData* globalData)
        : ExpressionNode(globalData, ResultType::nullType())
    {
    }

    inline BooleanNode::BooleanNode(JSGlobalData* globalData, bool value)
        : ExpressionNode(globalData, ResultType::booleanType())
        , m_value(value)
    {
    }

    inline NumberNode::NumberNode(JSGlobalData* globalData, double value)
        : ExpressionNode(globalData, ResultType::numberType())
        , m_value(value)
    {
    }

    inline StringNode::StringNode(JSGlobalData* globalData, const Identifier& value)
        : ExpressionNode(globalData, ResultType::stringType())
        , m_value(value)
    {
    }

    inline RegExpNode::RegExpNode(JSGlobalData* globalData, const Identifier& pattern, const Identifier& flags)
        : ExpressionNode(globalData)
        , m_pattern(pattern)
        , m_flags(flags)
    {
    }

    inline ThisNode::ThisNode(JSGlobalData* globalData)
        : ExpressionNode(globalData)
    {
    }

    inline ResolveNode::ResolveNode(JSGlobalData* globalData, const Identifier& ident, int startOffset)
        : ExpressionNode(globalData)
        , m_ident(ident)
        , m_startOffset(startOffset)
    {
    }

    inline ElementNode::ElementNode(JSGlobalData*, int elision, ExpressionNode* node)
        : m_next(0)
        , m_elision(elision)
        , m_node(node)
    {
    }

    inline ElementNode::ElementNode(JSGlobalData*, ElementNode* l, int elision, ExpressionNode* node)
        : m_next(0)
        , m_elision(elision)
        , m_node(node)
    {
        l->m_next = this;
    }

    inline ArrayNode::ArrayNode(JSGlobalData* globalData, int elision)
        : ExpressionNode(globalData)
        , m_element(0)
        , m_elision(elision)
        , m_optional(true)
    {
    }

    inline ArrayNode::ArrayNode(JSGlobalData* globalData, ElementNode* element)
        : ExpressionNode(globalData)
        , m_element(element)
        , m_elision(0)
        , m_optional(false)
    {
    }

    inline ArrayNode::ArrayNode(JSGlobalData* globalData, int elision, ElementNode* element)
        : ExpressionNode(globalData)
        , m_element(element)
        , m_elision(elision)
        , m_optional(true)
    {
    }

    inline PropertyNode::PropertyNode(JSGlobalData*, const Identifier& name, ExpressionNode* assign, Type type)
        : m_name(name)
        , m_assign(assign)
        , m_type(type)
    {
    }

    inline PropertyNode::PropertyNode(JSGlobalData* globalData, double name, ExpressionNode* assign, Type type)
        : m_name(globalData->parser->arena().identifierArena().makeNumericIdentifier(globalData, name))
        , m_assign(assign)
        , m_type(type)
    {
    }

    inline PropertyListNode::PropertyListNode(JSGlobalData* globalData, PropertyNode* node)
        : Node(globalData)
        , m_node(node)
        , m_next(0)
    {
    }

    inline PropertyListNode::PropertyListNode(JSGlobalData* globalData, PropertyNode* node, PropertyListNode* list)
        : Node(globalData)
        , m_node(node)
        , m_next(0)
    {
        list->m_next = this;
    }

    inline ObjectLiteralNode::ObjectLiteralNode(JSGlobalData* globalData)
        : ExpressionNode(globalData)
        , m_list(0)
    {
    }

    inline ObjectLiteralNode::ObjectLiteralNode(JSGlobalData* globalData, PropertyListNode* list)
        : ExpressionNode(globalData)
        , m_list(list)
    {
    }

    inline BracketAccessorNode::BracketAccessorNode(JSGlobalData* globalData, ExpressionNode* base, ExpressionNode* subscript, bool subscriptHasAssignments)
        : ExpressionNode(globalData)
        , m_base(base)
        , m_subscript(subscript)
        , m_subscriptHasAssignments(subscriptHasAssignments)
    {
    }

    inline DotAccessorNode::DotAccessorNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident)
        : ExpressionNode(globalData)
        , m_base(base)
        , m_ident(ident)
    {
    }

    inline ArgumentListNode::ArgumentListNode(JSGlobalData* globalData, ExpressionNode* expr)
        : Node(globalData)
        , m_next(0)
        , m_expr(expr)
    {
    }

    inline ArgumentListNode::ArgumentListNode(JSGlobalData* globalData, ArgumentListNode* listNode, ExpressionNode* expr)
        : Node(globalData)
        , m_next(0)
        , m_expr(expr)
    {
        listNode->m_next = this;
    }

    inline ArgumentsNode::ArgumentsNode(JSGlobalData*)
        : m_listNode(0)
    {
    }

    inline ArgumentsNode::ArgumentsNode(JSGlobalData*, ArgumentListNode* listNode)
        : m_listNode(listNode)
    {
    }

    inline NewExprNode::NewExprNode(JSGlobalData* globalData, ExpressionNode* expr)
        : ExpressionNode(globalData)
        , m_expr(expr)
        , m_args(0)
    {
    }

    inline NewExprNode::NewExprNode(JSGlobalData* globalData, ExpressionNode* expr, ArgumentsNode* args)
        : ExpressionNode(globalData)
        , m_expr(expr)
        , m_args(args)
    {
    }

    inline EvalFunctionCallNode::EvalFunctionCallNode(JSGlobalData* globalData, ArgumentsNode* args, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_args(args)
    {
    }

    inline FunctionCallValueNode::FunctionCallValueNode(JSGlobalData* globalData, ExpressionNode* expr, ArgumentsNode* args, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_expr(expr)
        , m_args(args)
    {
    }

    inline FunctionCallResolveNode::FunctionCallResolveNode(JSGlobalData* globalData, const Identifier& ident, ArgumentsNode* args, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_ident(ident)
        , m_args(args)
    {
    }

    inline FunctionCallBracketNode::FunctionCallBracketNode(JSGlobalData* globalData, ExpressionNode* base, ExpressionNode* subscript, ArgumentsNode* args, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableSubExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_subscript(subscript)
        , m_args(args)
    {
    }

    inline FunctionCallDotNode::FunctionCallDotNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident, ArgumentsNode* args, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableSubExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_ident(ident)
        , m_args(args)
    {
    }

    inline CallFunctionCallDotNode::CallFunctionCallDotNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident, ArgumentsNode* args, unsigned divot, unsigned startOffset, unsigned endOffset)
        : FunctionCallDotNode(globalData, base, ident, args, divot, startOffset, endOffset)
    {
    }

    inline ApplyFunctionCallDotNode::ApplyFunctionCallDotNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident, ArgumentsNode* args, unsigned divot, unsigned startOffset, unsigned endOffset)
        : FunctionCallDotNode(globalData, base, ident, args, divot, startOffset, endOffset)
    {
    }

    inline PrePostResolveNode::PrePostResolveNode(JSGlobalData* globalData, const Identifier& ident, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData, ResultType::numberType()) // could be reusable for pre?
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_ident(ident)
    {
    }

    inline PostfixResolveNode::PostfixResolveNode(JSGlobalData* globalData, const Identifier& ident, Operator oper, unsigned divot, unsigned startOffset, unsigned endOffset)
        : PrePostResolveNode(globalData, ident, divot, startOffset, endOffset)
        , m_operator(oper)
    {
    }

    inline PostfixBracketNode::PostfixBracketNode(JSGlobalData* globalData, ExpressionNode* base, ExpressionNode* subscript, Operator oper, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableSubExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_subscript(subscript)
        , m_operator(oper)
    {
    }

    inline PostfixDotNode::PostfixDotNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident, Operator oper, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableSubExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_ident(ident)
        , m_operator(oper)
    {
    }

    inline PostfixErrorNode::PostfixErrorNode(JSGlobalData* globalData, ExpressionNode* expr, Operator oper, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableSubExpressionData(divot, startOffset, endOffset)
        , m_expr(expr)
        , m_operator(oper)
    {
    }

    inline DeleteResolveNode::DeleteResolveNode(JSGlobalData* globalData, const Identifier& ident, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_ident(ident)
    {
    }

    inline DeleteBracketNode::DeleteBracketNode(JSGlobalData* globalData, ExpressionNode* base, ExpressionNode* subscript, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_subscript(subscript)
    {
    }

    inline DeleteDotNode::DeleteDotNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_ident(ident)
    {
    }

    inline DeleteValueNode::DeleteValueNode(JSGlobalData* globalData, ExpressionNode* expr)
        : ExpressionNode(globalData)
        , m_expr(expr)
    {
    }

    inline VoidNode::VoidNode(JSGlobalData* globalData, ExpressionNode* expr)
        : ExpressionNode(globalData)
        , m_expr(expr)
    {
    }

    inline TypeOfResolveNode::TypeOfResolveNode(JSGlobalData* globalData, const Identifier& ident)
        : ExpressionNode(globalData, ResultType::stringType())
        , m_ident(ident)
    {
    }

    inline TypeOfValueNode::TypeOfValueNode(JSGlobalData* globalData, ExpressionNode* expr)
        : ExpressionNode(globalData, ResultType::stringType())
        , m_expr(expr)
    {
    }

    inline PrefixResolveNode::PrefixResolveNode(JSGlobalData* globalData, const Identifier& ident, Operator oper, unsigned divot, unsigned startOffset, unsigned endOffset)
        : PrePostResolveNode(globalData, ident, divot, startOffset, endOffset)
        , m_operator(oper)
    {
    }

    inline PrefixBracketNode::PrefixBracketNode(JSGlobalData* globalData, ExpressionNode* base, ExpressionNode* subscript, Operator oper, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowablePrefixedSubExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_subscript(subscript)
        , m_operator(oper)
    {
    }

    inline PrefixDotNode::PrefixDotNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident, Operator oper, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowablePrefixedSubExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_ident(ident)
        , m_operator(oper)
    {
    }

    inline PrefixErrorNode::PrefixErrorNode(JSGlobalData* globalData, ExpressionNode* expr, Operator oper, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_expr(expr)
        , m_operator(oper)
    {
    }

    inline UnaryOpNode::UnaryOpNode(JSGlobalData* globalData, ResultType type, ExpressionNode* expr, OpcodeID opcodeID)
        : ExpressionNode(globalData, type)
        , m_expr(expr)
        , m_opcodeID(opcodeID)
    {
    }

    inline UnaryPlusNode::UnaryPlusNode(JSGlobalData* globalData, ExpressionNode* expr)
        : UnaryOpNode(globalData, ResultType::numberType(), expr, op_to_jsnumber)
    {
    }

    inline NegateNode::NegateNode(JSGlobalData* globalData, ExpressionNode* expr)
        : UnaryOpNode(globalData, ResultType::numberTypeCanReuse(), expr, op_negate)
    {
    }

    inline BitwiseNotNode::BitwiseNotNode(JSGlobalData* globalData, ExpressionNode* expr)
        : UnaryOpNode(globalData, ResultType::forBitOp(), expr, op_bitnot)
    {
    }

    inline LogicalNotNode::LogicalNotNode(JSGlobalData* globalData, ExpressionNode* expr)
        : UnaryOpNode(globalData, ResultType::booleanType(), expr, op_not)
    {
    }

    inline BinaryOpNode::BinaryOpNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID opcodeID, bool rightHasAssignments)
        : ExpressionNode(globalData)
        , m_expr1(expr1)
        , m_expr2(expr2)
        , m_opcodeID(opcodeID)
        , m_rightHasAssignments(rightHasAssignments)
    {
    }

    inline BinaryOpNode::BinaryOpNode(JSGlobalData* globalData, ResultType type, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID opcodeID, bool rightHasAssignments)
        : ExpressionNode(globalData, type)
        , m_expr1(expr1)
        , m_expr2(expr2)
        , m_opcodeID(opcodeID)
        , m_rightHasAssignments(rightHasAssignments)
    {
    }

    inline ReverseBinaryOpNode::ReverseBinaryOpNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID opcodeID, bool rightHasAssignments)
        : BinaryOpNode(globalData, expr1, expr2, opcodeID, rightHasAssignments)
    {
    }

    inline ReverseBinaryOpNode::ReverseBinaryOpNode(JSGlobalData* globalData, ResultType type, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID opcodeID, bool rightHasAssignments)
        : BinaryOpNode(globalData, type, expr1, expr2, opcodeID, rightHasAssignments)
    {
    }

    inline MultNode::MultNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::numberTypeCanReuse(), expr1, expr2, op_mul, rightHasAssignments)
    {
    }

    inline DivNode::DivNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::numberTypeCanReuse(), expr1, expr2, op_div, rightHasAssignments)
    {
    }


    inline ModNode::ModNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::numberTypeCanReuse(), expr1, expr2, op_mod, rightHasAssignments)
    {
    }

    inline AddNode::AddNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::forAdd(expr1->resultDescriptor(), expr2->resultDescriptor()), expr1, expr2, op_add, rightHasAssignments)
    {
    }

    inline SubNode::SubNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::numberTypeCanReuse(), expr1, expr2, op_sub, rightHasAssignments)
    {
    }

    inline LeftShiftNode::LeftShiftNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::forBitOp(), expr1, expr2, op_lshift, rightHasAssignments)
    {
    }

    inline RightShiftNode::RightShiftNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::forBitOp(), expr1, expr2, op_rshift, rightHasAssignments)
    {
    }

    inline UnsignedRightShiftNode::UnsignedRightShiftNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::numberTypeCanReuse(), expr1, expr2, op_urshift, rightHasAssignments)
    {
    }

    inline LessNode::LessNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_less, rightHasAssignments)
    {
    }

    inline GreaterNode::GreaterNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : ReverseBinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_less, rightHasAssignments)
    {
    }

    inline LessEqNode::LessEqNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_lesseq, rightHasAssignments)
    {
    }

    inline GreaterEqNode::GreaterEqNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : ReverseBinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_lesseq, rightHasAssignments)
    {
    }

    inline ThrowableBinaryOpNode::ThrowableBinaryOpNode(JSGlobalData* globalData, ResultType type, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID opcodeID, bool rightHasAssignments)
        : BinaryOpNode(globalData, type, expr1, expr2, opcodeID, rightHasAssignments)
    {
    }

    inline ThrowableBinaryOpNode::ThrowableBinaryOpNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, OpcodeID opcodeID, bool rightHasAssignments)
        : BinaryOpNode(globalData, expr1, expr2, opcodeID, rightHasAssignments)
    {
    }

    inline InstanceOfNode::InstanceOfNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : ThrowableBinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_instanceof, rightHasAssignments)
    {
    }

    inline InNode::InNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : ThrowableBinaryOpNode(globalData, expr1, expr2, op_in, rightHasAssignments)
    {
    }

    inline EqualNode::EqualNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_eq, rightHasAssignments)
    {
    }

    inline NotEqualNode::NotEqualNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_neq, rightHasAssignments)
    {
    }

    inline StrictEqualNode::StrictEqualNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_stricteq, rightHasAssignments)
    {
    }

    inline NotStrictEqualNode::NotStrictEqualNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::booleanType(), expr1, expr2, op_nstricteq, rightHasAssignments)
    {
    }

    inline BitAndNode::BitAndNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::forBitOp(), expr1, expr2, op_bitand, rightHasAssignments)
    {
    }

    inline BitOrNode::BitOrNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::forBitOp(), expr1, expr2, op_bitor, rightHasAssignments)
    {
    }

    inline BitXOrNode::BitXOrNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, bool rightHasAssignments)
        : BinaryOpNode(globalData, ResultType::forBitOp(), expr1, expr2, op_bitxor, rightHasAssignments)
    {
    }

    inline LogicalOpNode::LogicalOpNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, LogicalOperator oper)
        : ExpressionNode(globalData, ResultType::booleanType())
        , m_expr1(expr1)
        , m_expr2(expr2)
        , m_operator(oper)
    {
    }

    inline ConditionalNode::ConditionalNode(JSGlobalData* globalData, ExpressionNode* logical, ExpressionNode* expr1, ExpressionNode* expr2)
        : ExpressionNode(globalData)
        , m_logical(logical)
        , m_expr1(expr1)
        , m_expr2(expr2)
    {
    }

    inline ReadModifyResolveNode::ReadModifyResolveNode(JSGlobalData* globalData, const Identifier& ident, Operator oper, ExpressionNode*  right, bool rightHasAssignments, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_ident(ident)
        , m_right(right)
        , m_operator(oper)
        , m_rightHasAssignments(rightHasAssignments)
    {
    }

    inline AssignResolveNode::AssignResolveNode(JSGlobalData* globalData, const Identifier& ident, ExpressionNode* right, bool rightHasAssignments)
        : ExpressionNode(globalData)
        , m_ident(ident)
        , m_right(right)
        , m_rightHasAssignments(rightHasAssignments)
    {
    }

    inline ReadModifyBracketNode::ReadModifyBracketNode(JSGlobalData* globalData, ExpressionNode* base, ExpressionNode* subscript, Operator oper, ExpressionNode* right, bool subscriptHasAssignments, bool rightHasAssignments, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableSubExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_subscript(subscript)
        , m_right(right)
        , m_operator(oper)
        , m_subscriptHasAssignments(subscriptHasAssignments)
        , m_rightHasAssignments(rightHasAssignments)
    {
    }

    inline AssignBracketNode::AssignBracketNode(JSGlobalData* globalData, ExpressionNode* base, ExpressionNode* subscript, ExpressionNode* right, bool subscriptHasAssignments, bool rightHasAssignments, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_subscript(subscript)
        , m_right(right)
        , m_subscriptHasAssignments(subscriptHasAssignments)
        , m_rightHasAssignments(rightHasAssignments)
    {
    }

    inline AssignDotNode::AssignDotNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident, ExpressionNode* right, bool rightHasAssignments, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_ident(ident)
        , m_right(right)
        , m_rightHasAssignments(rightHasAssignments)
    {
    }

    inline ReadModifyDotNode::ReadModifyDotNode(JSGlobalData* globalData, ExpressionNode* base, const Identifier& ident, Operator oper, ExpressionNode* right, bool rightHasAssignments, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableSubExpressionData(divot, startOffset, endOffset)
        , m_base(base)
        , m_ident(ident)
        , m_right(right)
        , m_operator(oper)
        , m_rightHasAssignments(rightHasAssignments)
    {
    }

    inline AssignErrorNode::AssignErrorNode(JSGlobalData* globalData, ExpressionNode* left, Operator oper, ExpressionNode* right, unsigned divot, unsigned startOffset, unsigned endOffset)
        : ExpressionNode(globalData)
        , ThrowableExpressionData(divot, startOffset, endOffset)
        , m_left(left)
        , m_operator(oper)
        , m_right(right)
    {
    }

    inline CommaNode::CommaNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2)
        : ExpressionNode(globalData)
    {
        m_expressions.append(expr1);
        m_expressions.append(expr2);
    }

    inline ConstStatementNode::ConstStatementNode(JSGlobalData* globalData, ConstDeclNode* next)
        : StatementNode(globalData)
        , m_next(next)
    {
    }

    inline SourceElements::SourceElements(JSGlobalData*)
    {
    }

    inline EmptyStatementNode::EmptyStatementNode(JSGlobalData* globalData)
        : StatementNode(globalData)
    {
    }

    inline DebuggerStatementNode::DebuggerStatementNode(JSGlobalData* globalData)
        : StatementNode(globalData)
    {
    }
    
    inline ExprStatementNode::ExprStatementNode(JSGlobalData* globalData, ExpressionNode* expr)
        : StatementNode(globalData)
        , m_expr(expr)
    {
    }

    inline VarStatementNode::VarStatementNode(JSGlobalData* globalData, ExpressionNode* expr)
        : StatementNode(globalData)
        , m_expr(expr)
    {
    }
    
    inline IfNode::IfNode(JSGlobalData* globalData, ExpressionNode* condition, StatementNode* ifBlock)
        : StatementNode(globalData)
        , m_condition(condition)
        , m_ifBlock(ifBlock)
    {
    }

    inline IfElseNode::IfElseNode(JSGlobalData* globalData, ExpressionNode* condition, StatementNode* ifBlock, StatementNode* elseBlock)
        : IfNode(globalData, condition, ifBlock)
        , m_elseBlock(elseBlock)
    {
    }

    inline DoWhileNode::DoWhileNode(JSGlobalData* globalData, StatementNode* statement, ExpressionNode* expr)
        : StatementNode(globalData)
        , m_statement(statement)
        , m_expr(expr)
    {
    }

    inline WhileNode::WhileNode(JSGlobalData* globalData, ExpressionNode* expr, StatementNode* statement)
        : StatementNode(globalData)
        , m_expr(expr)
        , m_statement(statement)
    {
    }

    inline ForNode::ForNode(JSGlobalData* globalData, ExpressionNode* expr1, ExpressionNode* expr2, ExpressionNode* expr3, StatementNode* statement, bool expr1WasVarDecl)
        : StatementNode(globalData)
        , m_expr1(expr1)
        , m_expr2(expr2)
        , m_expr3(expr3)
        , m_statement(statement)
        , m_expr1WasVarDecl(expr1 && expr1WasVarDecl)
    {
        ASSERT(statement);
    }

    inline ContinueNode::ContinueNode(JSGlobalData* globalData)
        : StatementNode(globalData)
        , m_ident(globalData->propertyNames->nullIdentifier)
    {
    }

    inline ContinueNode::ContinueNode(JSGlobalData* globalData, const Identifier& ident)
        : StatementNode(globalData)
        , m_ident(ident)
    {
    }
    
    inline BreakNode::BreakNode(JSGlobalData* globalData)
        : StatementNode(globalData)
        , m_ident(globalData->propertyNames->nullIdentifier)
    {
    }

    inline BreakNode::BreakNode(JSGlobalData* globalData, const Identifier& ident)
        : StatementNode(globalData)
        , m_ident(ident)
    {
    }
    
    inline ReturnNode::ReturnNode(JSGlobalData* globalData, ExpressionNode* value)
        : StatementNode(globalData)
        , m_value(value)
    {
    }

    inline WithNode::WithNode(JSGlobalData* globalData, ExpressionNode* expr, StatementNode* statement, uint32_t divot, uint32_t expressionLength)
        : StatementNode(globalData)
        , m_expr(expr)
        , m_statement(statement)
        , m_divot(divot)
        , m_expressionLength(expressionLength)
    {
    }

    inline LabelNode::LabelNode(JSGlobalData* globalData, const Identifier& name, StatementNode* statement)
        : StatementNode(globalData)
        , m_name(name)
        , m_statement(statement)
    {
    }

    inline ThrowNode::ThrowNode(JSGlobalData* globalData, ExpressionNode* expr)
        : StatementNode(globalData)
        , m_expr(expr)
    {
    }

    inline TryNode::TryNode(JSGlobalData* globalData, StatementNode* tryBlock, const Identifier& exceptionIdent, bool catchHasEval, StatementNode* catchBlock, StatementNode* finallyBlock)
        : StatementNode(globalData)
        , m_tryBlock(tryBlock)
        , m_exceptionIdent(exceptionIdent)
        , m_catchBlock(catchBlock)
        , m_finallyBlock(finallyBlock)
        , m_catchHasEval(catchHasEval)
    {
    }

    inline ParameterNode::ParameterNode(JSGlobalData*, const Identifier& ident)
        : m_ident(ident)
        , m_next(0)
    {
    }

    inline ParameterNode::ParameterNode(JSGlobalData*, ParameterNode* l, const Identifier& ident)
        : m_ident(ident)
        , m_next(0)
    {
        l->m_next = this;
    }

    inline FuncExprNode::FuncExprNode(JSGlobalData* globalData, const Identifier& ident, FunctionBodyNode* body, const SourceCode& source, ParameterNode* parameter)
        : ExpressionNode(globalData)
        , m_body(body)
    {
        m_body->finishParsing(source, parameter, ident);
    }

    inline FuncDeclNode::FuncDeclNode(JSGlobalData* globalData, const Identifier& ident, FunctionBodyNode* body, const SourceCode& source, ParameterNode* parameter)
        : StatementNode(globalData)
        , m_body(body)
    {
        m_body->finishParsing(source, parameter, ident);
    }

    inline CaseClauseNode::CaseClauseNode(JSGlobalData*, ExpressionNode* expr, SourceElements* statements)
        : m_expr(expr)
        , m_statements(statements)
    {
    }

    inline ClauseListNode::ClauseListNode(JSGlobalData*, CaseClauseNode* clause)
        : m_clause(clause)
        , m_next(0)
    {
    }

    inline ClauseListNode::ClauseListNode(JSGlobalData*, ClauseListNode* clauseList, CaseClauseNode* clause)
        : m_clause(clause)
        , m_next(0)
    {
        clauseList->m_next = this;
    }

    inline CaseBlockNode::CaseBlockNode(JSGlobalData*, ClauseListNode* list1, CaseClauseNode* defaultClause, ClauseListNode* list2)
        : m_list1(list1)
        , m_defaultClause(defaultClause)
        , m_list2(list2)
    {
    }

    inline SwitchNode::SwitchNode(JSGlobalData* globalData, ExpressionNode* expr, CaseBlockNode* block)
        : StatementNode(globalData)
        , m_expr(expr)
        , m_block(block)
    {
    }

    inline ConstDeclNode::ConstDeclNode(JSGlobalData* globalData, const Identifier& ident, ExpressionNode* init)
        : ExpressionNode(globalData)
        , m_ident(ident)
        , m_next(0)
        , m_init(init)
    {
    }

    inline BlockNode::BlockNode(JSGlobalData* globalData, SourceElements* statements)
        : StatementNode(globalData)
        , m_statements(statements)
    {
    }

    inline ForInNode::ForInNode(JSGlobalData* globalData, ExpressionNode* l, ExpressionNode* expr, StatementNode* statement)
        : StatementNode(globalData)
        , m_ident(globalData->propertyNames->nullIdentifier)
        , m_init(0)
        , m_lexpr(l)
        , m_expr(expr)
        , m_statement(statement)
        , m_identIsVarDecl(false)
    {
    }

    inline ForInNode::ForInNode(JSGlobalData* globalData, const Identifier& ident, ExpressionNode* in, ExpressionNode* expr, StatementNode* statement, int divot, int startOffset, int endOffset)
        : StatementNode(globalData)
        , m_ident(ident)
        , m_init(0)
        , m_lexpr(new (globalData) ResolveNode(globalData, ident, divot - startOffset))
        , m_expr(expr)
        , m_statement(statement)
        , m_identIsVarDecl(true)
    {
        if (in) {
            AssignResolveNode* node = new (globalData) AssignResolveNode(globalData, ident, in, true);
            node->setExceptionSourceCode(divot, divot - startOffset, endOffset - divot);
            m_init = node;
        }
        // for( var foo = bar in baz )
    }

} // namespace JSC

#endif // NodeConstructors_h
