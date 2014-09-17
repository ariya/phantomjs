/*
*  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
*  Copyright (C) 2001 Peter Kelly (pmk@post.com)
*  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#include "config.h"
#include "Nodes.h"
#include "NodeConstructors.h"

#include "BytecodeGenerator.h"
#include "CallFrame.h"
#include "Debugger.h"
#include "JIT.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSStaticScopeObject.h"
#include "LabelScope.h"
#include "Lexer.h"
#include "Operations.h"
#include "Parser.h"
#include "PropertyNameArray.h"
#include "RegExpCache.h"
#include "RegExpObject.h"
#include "SamplingTool.h"
#include "UStringConcatenate.h"
#include <wtf/Assertions.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/Threading.h>

using namespace WTF;

namespace JSC {

/*
    Details of the emitBytecode function.

    Return value: The register holding the production's value.
             dst: An optional parameter specifying the most efficient destination at
                  which to store the production's value. The callee must honor dst.

    The dst argument provides for a crude form of copy propagation. For example,

        x = 1

    becomes
    
        load r[x], 1
    
    instead of 

        load r0, 1
        mov r[x], r0
    
    because the assignment node, "x =", passes r[x] as dst to the number node, "1".
*/

// ------------------------------ ThrowableExpressionData --------------------------------

RegisterID* ThrowableExpressionData::emitThrowReferenceError(BytecodeGenerator& generator, const UString& message)
{
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitThrowReferenceError(message);
    return generator.newTemporary();
}

// ------------------------------ NullNode -------------------------------------

RegisterID* NullNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return 0;
    return generator.emitLoad(dst, jsNull());
}

// ------------------------------ BooleanNode ----------------------------------

RegisterID* BooleanNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return 0;
    return generator.emitLoad(dst, m_value);
}

// ------------------------------ NumberNode -----------------------------------

RegisterID* NumberNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return 0;
    return generator.emitLoad(dst, m_value);
}

// ------------------------------ StringNode -----------------------------------

RegisterID* StringNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return 0;
    return generator.emitLoad(dst, m_value);
}

// ------------------------------ RegExpNode -----------------------------------

RegisterID* RegExpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return 0;
    return generator.emitNewRegExp(generator.finalDestination(dst),
        generator.globalData()->regExpCache()->lookupOrCreate(m_pattern.ustring(), regExpFlags(m_flags.ustring())));
}

// ------------------------------ ThisNode -------------------------------------

RegisterID* ThisNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return 0;
    return generator.moveToDestinationIfNeeded(dst, generator.thisRegister());
}

// ------------------------------ ResolveNode ----------------------------------

bool ResolveNode::isPure(BytecodeGenerator& generator) const
{
    return generator.isLocal(m_ident);
}

RegisterID* ResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (RegisterID* local = generator.registerFor(m_ident)) {
        if (dst == generator.ignoredResult())
            return 0;
        return generator.moveToDestinationIfNeeded(dst, local);
    }
    
    generator.emitExpressionInfo(m_startOffset + m_ident.length(), m_ident.length(), 0);
    return generator.emitResolve(generator.finalDestination(dst), m_ident);
}

// ------------------------------ ArrayNode ------------------------------------

RegisterID* ArrayNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    // FIXME: Should we put all of this code into emitNewArray?

    unsigned length = 0;
    ElementNode* firstPutElement;
    for (firstPutElement = m_element; firstPutElement; firstPutElement = firstPutElement->next()) {
        if (firstPutElement->elision())
            break;
        ++length;
    }

    if (!firstPutElement && !m_elision)
        return generator.emitNewArray(generator.finalDestination(dst), m_element);

    RefPtr<RegisterID> array = generator.emitNewArray(generator.tempDestination(dst), m_element);

    for (ElementNode* n = firstPutElement; n; n = n->next()) {
        RegisterID* value = generator.emitNode(n->value());
        length += n->elision();
        generator.emitPutByIndex(array.get(), length++, value);
    }

    if (m_elision) {
        RegisterID* value = generator.emitLoad(0, jsNumber(m_elision + length));
        generator.emitPutById(array.get(), generator.propertyNames().length, value);
    }

    return generator.moveToDestinationIfNeeded(dst, array.get());
}

bool ArrayNode::isSimpleArray() const
{
    if (m_elision || m_optional)
        return false;
    for (ElementNode* ptr = m_element; ptr; ptr = ptr->next()) {
        if (ptr->elision())
            return false;
    }
    return true;
}

ArgumentListNode* ArrayNode::toArgumentList(JSGlobalData* globalData) const
{
    ASSERT(!m_elision && !m_optional);
    ElementNode* ptr = m_element;
    if (!ptr)
        return 0;
    ArgumentListNode* head = new (globalData) ArgumentListNode(globalData, ptr->value());
    ArgumentListNode* tail = head;
    ptr = ptr->next();
    for (; ptr; ptr = ptr->next()) {
        ASSERT(!ptr->elision());
        tail = new (globalData) ArgumentListNode(globalData, tail, ptr->value());
    }
    return head;
}

// ------------------------------ ObjectLiteralNode ----------------------------

RegisterID* ObjectLiteralNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
     if (!m_list) {
         if (dst == generator.ignoredResult())
             return 0;
         return generator.emitNewObject(generator.finalDestination(dst));
     }
     return generator.emitNode(dst, m_list);
}

// ------------------------------ PropertyListNode -----------------------------

RegisterID* PropertyListNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> newObj = generator.tempDestination(dst);
    
    generator.emitNewObject(newObj.get());
    
    for (PropertyListNode* p = this; p; p = p->m_next) {
        RegisterID* value = generator.emitNode(p->m_node->m_assign);
        
        switch (p->m_node->m_type) {
            case PropertyNode::Constant: {
                generator.emitDirectPutById(newObj.get(), p->m_node->name(), value);
                break;
            }
            case PropertyNode::Getter: {
                generator.emitPutGetter(newObj.get(), p->m_node->name(), value);
                break;
            }
            case PropertyNode::Setter: {
                generator.emitPutSetter(newObj.get(), p->m_node->name(), value);
                break;
            }
            default:
                ASSERT_NOT_REACHED();
        }
    }
    
    return generator.moveToDestinationIfNeeded(dst, newObj.get());
}

// ------------------------------ BracketAccessorNode --------------------------------

RegisterID* BracketAccessorNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_base->isResolveNode() && generator.willResolveToArguments(static_cast<ResolveNode*>(m_base)->identifier())) {
        RegisterID* property = generator.emitNode(m_subscript);
        generator.emitExpressionInfo(divot(), startOffset(), endOffset());    
        return generator.emitGetArgumentByVal(generator.finalDestination(dst), generator.uncheckedRegisterForArguments(), property);
    }

    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_subscriptHasAssignments, m_subscript->isPure(generator));
    RegisterID* property = generator.emitNode(m_subscript);
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    return generator.emitGetByVal(generator.finalDestination(dst), base.get(), property);
}

// ------------------------------ DotAccessorNode --------------------------------

RegisterID* DotAccessorNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_ident == generator.propertyNames().length) {
        if (!m_base->isResolveNode())
            goto nonArgumentsPath;
        ResolveNode* resolveNode = static_cast<ResolveNode*>(m_base);
        if (!generator.willResolveToArguments(resolveNode->identifier()))
            goto nonArgumentsPath;
        generator.emitExpressionInfo(divot(), startOffset(), endOffset());    
        return generator.emitGetArgumentsLength(generator.finalDestination(dst), generator.uncheckedRegisterForArguments());
    }

nonArgumentsPath:
    RegisterID* base = generator.emitNode(m_base);
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    return generator.emitGetById(generator.finalDestination(dst), base, m_ident);
}

// ------------------------------ ArgumentListNode -----------------------------

RegisterID* ArgumentListNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ASSERT(m_expr);
    return generator.emitNode(dst, m_expr);
}

// ------------------------------ NewExprNode ----------------------------------

RegisterID* NewExprNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> func = generator.emitNode(m_expr);
    CallArguments callArguments(generator, m_args);
    return generator.emitConstruct(generator.finalDestinationOrIgnored(dst), func.get(), callArguments, divot(), startOffset(), endOffset());
}

CallArguments::CallArguments(BytecodeGenerator& generator, ArgumentsNode* argumentsNode)
    : m_argumentsNode(argumentsNode)
{
    if (generator.shouldEmitProfileHooks())
        m_profileHookRegister = generator.newTemporary();
    m_argv.append(generator.newTemporary());
    if (argumentsNode) {
        for (ArgumentListNode* n = argumentsNode->m_listNode; n; n = n->m_next) {
            m_argv.append(generator.newTemporary());
            // op_call requires the arguments to be a sequential range of registers
            ASSERT(m_argv[m_argv.size() - 1]->index() == m_argv[m_argv.size() - 2]->index() + 1);
        }
    }
}

// ------------------------------ EvalFunctionCallNode ----------------------------------

RegisterID* EvalFunctionCallNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> func = generator.tempDestination(dst);
    CallArguments callArguments(generator, m_args);
    generator.emitExpressionInfo(divot() - startOffset() + 4, 4, 0);
    generator.emitResolveWithBase(callArguments.thisRegister(), func.get(), generator.propertyNames().eval);
    return generator.emitCallEval(generator.finalDestination(dst, func.get()), func.get(), callArguments, divot(), startOffset(), endOffset());
}

// ------------------------------ FunctionCallValueNode ----------------------------------

RegisterID* FunctionCallValueNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> func = generator.emitNode(m_expr);
    CallArguments callArguments(generator, m_args);
    generator.emitLoad(callArguments.thisRegister(), jsUndefined());
    return generator.emitCall(generator.finalDestinationOrIgnored(dst, func.get()), func.get(), callArguments, divot(), startOffset(), endOffset());
}

// ------------------------------ FunctionCallResolveNode ----------------------------------

RegisterID* FunctionCallResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (RefPtr<RegisterID> local = generator.registerFor(m_ident)) {
        CallArguments callArguments(generator, m_args);
        generator.emitLoad(callArguments.thisRegister(), jsUndefined());
        return generator.emitCall(generator.finalDestinationOrIgnored(dst, callArguments.thisRegister()), local.get(), callArguments, divot(), startOffset(), endOffset());
    }

    int index = 0;
    size_t depth = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    if (generator.findScopedProperty(m_ident, index, depth, false, requiresDynamicChecks, globalObject) && index != missingSymbolMarker() && !requiresDynamicChecks) {
        RefPtr<RegisterID> func = generator.emitGetScopedVar(generator.newTemporary(), depth, index, globalObject);
        CallArguments callArguments(generator, m_args);
        generator.emitLoad(callArguments.thisRegister(), jsUndefined());
        return generator.emitCall(generator.finalDestinationOrIgnored(dst, func.get()), func.get(), callArguments, divot(), startOffset(), endOffset());
    }

    RefPtr<RegisterID> func = generator.newTemporary();
    CallArguments callArguments(generator, m_args);
    int identifierStart = divot() - startOffset();
    generator.emitExpressionInfo(identifierStart + m_ident.length(), m_ident.length(), 0);
    generator.emitResolveWithBase(callArguments.thisRegister(), func.get(), m_ident);
    return generator.emitCall(generator.finalDestinationOrIgnored(dst, func.get()), func.get(), callArguments, divot(), startOffset(), endOffset());
}

// ------------------------------ FunctionCallBracketNode ----------------------------------

RegisterID* FunctionCallBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNode(m_base);
    RegisterID* property = generator.emitNode(m_subscript);
    generator.emitExpressionInfo(divot() - m_subexpressionDivotOffset, startOffset() - m_subexpressionDivotOffset, m_subexpressionEndOffset);
    RefPtr<RegisterID> function = generator.emitGetByVal(generator.tempDestination(dst), base.get(), property);
    CallArguments callArguments(generator, m_args);
    generator.emitMove(callArguments.thisRegister(), base.get());
    return generator.emitCall(generator.finalDestinationOrIgnored(dst, function.get()), function.get(), callArguments, divot(), startOffset(), endOffset());
}

// ------------------------------ FunctionCallDotNode ----------------------------------

RegisterID* FunctionCallDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> function = generator.tempDestination(dst);
    CallArguments callArguments(generator, m_args);
    generator.emitNode(callArguments.thisRegister(), m_base);
    generator.emitExpressionInfo(divot() - m_subexpressionDivotOffset, startOffset() - m_subexpressionDivotOffset, m_subexpressionEndOffset);
    generator.emitMethodCheck();
    generator.emitGetById(function.get(), callArguments.thisRegister(), m_ident);
    return generator.emitCall(generator.finalDestinationOrIgnored(dst, function.get()), function.get(), callArguments, divot(), startOffset(), endOffset());
}

RegisterID* CallFunctionCallDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<Label> realCall = generator.newLabel();
    RefPtr<Label> end = generator.newLabel();
    RefPtr<RegisterID> base = generator.emitNode(m_base);
    generator.emitExpressionInfo(divot() - m_subexpressionDivotOffset, startOffset() - m_subexpressionDivotOffset, m_subexpressionEndOffset);
    RefPtr<RegisterID> function = generator.emitGetById(generator.tempDestination(dst), base.get(), m_ident);
    RefPtr<RegisterID> finalDestinationOrIgnored = generator.finalDestinationOrIgnored(dst, function.get());
    generator.emitJumpIfNotFunctionCall(function.get(), realCall.get());
    {
        if (m_args->m_listNode && m_args->m_listNode->m_expr) {
            ArgumentListNode* oldList = m_args->m_listNode;
            m_args->m_listNode = m_args->m_listNode->m_next;

            RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
            CallArguments callArguments(generator, m_args);
            generator.emitNode(callArguments.thisRegister(), oldList->m_expr);
            generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), callArguments, divot(), startOffset(), endOffset());
            generator.emitJump(end.get());

            m_args->m_listNode = oldList;

        } else {
            RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
            CallArguments callArguments(generator, m_args);
            generator.emitLoad(callArguments.thisRegister(), jsUndefined());
            generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), callArguments, divot(), startOffset(), endOffset());
            generator.emitJump(end.get());
        }
    }
    generator.emitLabel(realCall.get());
    {
        CallArguments callArguments(generator, m_args);
        generator.emitMove(callArguments.thisRegister(), base.get());
        generator.emitCall(finalDestinationOrIgnored.get(), function.get(), callArguments, divot(), startOffset(), endOffset());
    }
    generator.emitLabel(end.get());
    return finalDestinationOrIgnored.get();
}

static bool areTrivialApplyArguments(ArgumentsNode* args)
{
    return !args->m_listNode || !args->m_listNode->m_expr || !args->m_listNode->m_next
        || (!args->m_listNode->m_next->m_next && args->m_listNode->m_next->m_expr->isSimpleArray());
}

RegisterID* ApplyFunctionCallDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    // A few simple cases can be trivially handled as ordinary function calls.
    // function.apply(), function.apply(arg) -> identical to function.call
    // function.apply(thisArg, [arg0, arg1, ...]) -> can be trivially coerced into function.call(thisArg, arg0, arg1, ...) and saves object allocation
    bool mayBeCall = areTrivialApplyArguments(m_args);

    RefPtr<Label> realCall = generator.newLabel();
    RefPtr<Label> end = generator.newLabel();
    RefPtr<RegisterID> base = generator.emitNode(m_base);
    generator.emitExpressionInfo(divot() - m_subexpressionDivotOffset, startOffset() - m_subexpressionDivotOffset, m_subexpressionEndOffset);
    RefPtr<RegisterID> function = generator.emitGetById(generator.tempDestination(dst), base.get(), m_ident);
    RefPtr<RegisterID> finalDestinationOrIgnored = generator.finalDestinationOrIgnored(dst, function.get());
    generator.emitJumpIfNotFunctionApply(function.get(), realCall.get());
    {
        if (mayBeCall) {
            if (m_args->m_listNode && m_args->m_listNode->m_expr) {
                ArgumentListNode* oldList = m_args->m_listNode;
                if (m_args->m_listNode->m_next) {
                    ASSERT(m_args->m_listNode->m_next->m_expr->isSimpleArray());
                    ASSERT(!m_args->m_listNode->m_next->m_next);
                    m_args->m_listNode = static_cast<ArrayNode*>(m_args->m_listNode->m_next->m_expr)->toArgumentList(generator.globalData());
                    RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
                    CallArguments callArguments(generator, m_args);
                    generator.emitNode(callArguments.thisRegister(), oldList->m_expr);
                    generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), callArguments, divot(), startOffset(), endOffset());
                } else {
                    m_args->m_listNode = m_args->m_listNode->m_next;
                    RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
                    CallArguments callArguments(generator, m_args);
                    generator.emitNode(callArguments.thisRegister(), oldList->m_expr);
                    generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), callArguments, divot(), startOffset(), endOffset());
                }
                m_args->m_listNode = oldList;
            } else {
                RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
                CallArguments callArguments(generator, m_args);
                generator.emitLoad(callArguments.thisRegister(), jsUndefined());
                generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), callArguments, divot(), startOffset(), endOffset());
            }
        } else {
            ASSERT(m_args->m_listNode && m_args->m_listNode->m_next);
            RefPtr<RegisterID> realFunction = generator.emitMove(generator.newTemporary(), base.get());
            RefPtr<RegisterID> argsCountRegister = generator.newTemporary();
            RefPtr<RegisterID> thisRegister = generator.newTemporary();
            RefPtr<RegisterID> argsRegister = generator.newTemporary();
            generator.emitNode(thisRegister.get(), m_args->m_listNode->m_expr);
            ArgumentListNode* args = m_args->m_listNode->m_next;
            bool isArgumentsApply = false;
            if (args->m_expr->isResolveNode()) {
                ResolveNode* resolveNode = static_cast<ResolveNode*>(args->m_expr);
                isArgumentsApply = generator.willResolveToArguments(resolveNode->identifier());
                if (isArgumentsApply)
                    generator.emitMove(argsRegister.get(), generator.uncheckedRegisterForArguments());
            }
            if (!isArgumentsApply)
                generator.emitNode(argsRegister.get(), args->m_expr);
            while ((args = args->m_next))
                generator.emitNode(args->m_expr);

            generator.emitLoadVarargs(argsCountRegister.get(), thisRegister.get(), argsRegister.get());
            generator.emitCallVarargs(finalDestinationOrIgnored.get(), realFunction.get(), thisRegister.get(), argsCountRegister.get(), divot(), startOffset(), endOffset());
        }
        generator.emitJump(end.get());
    }
    generator.emitLabel(realCall.get());
    {
        CallArguments callArguments(generator, m_args);
        generator.emitMove(callArguments.thisRegister(), base.get());
        generator.emitCall(finalDestinationOrIgnored.get(), function.get(), callArguments, divot(), startOffset(), endOffset());
    }
    generator.emitLabel(end.get());
    return finalDestinationOrIgnored.get();
}

// ------------------------------ PostfixResolveNode ----------------------------------

static RegisterID* emitPreIncOrDec(BytecodeGenerator& generator, RegisterID* srcDst, Operator oper)
{
    return (oper == OpPlusPlus) ? generator.emitPreInc(srcDst) : generator.emitPreDec(srcDst);
}

static RegisterID* emitPostIncOrDec(BytecodeGenerator& generator, RegisterID* dst, RegisterID* srcDst, Operator oper)
{
    if (srcDst == dst)
        return generator.emitToJSNumber(dst, srcDst);
    return (oper == OpPlusPlus) ? generator.emitPostInc(dst, srcDst) : generator.emitPostDec(dst, srcDst);
}

RegisterID* PostfixResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (RegisterID* local = generator.registerFor(m_ident)) {
        if (generator.isLocalConstant(m_ident)) {
            if (dst == generator.ignoredResult())
                return 0;
            return generator.emitToJSNumber(generator.finalDestination(dst), local);
        }

        if (dst == generator.ignoredResult())
            return emitPreIncOrDec(generator, local, m_operator);
        return emitPostIncOrDec(generator, generator.finalDestination(dst), local, m_operator);
    }

    int index = 0;
    size_t depth = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    if (generator.findScopedProperty(m_ident, index, depth, true, requiresDynamicChecks, globalObject) && index != missingSymbolMarker() && !requiresDynamicChecks) {
        RefPtr<RegisterID> value = generator.emitGetScopedVar(generator.newTemporary(), depth, index, globalObject);
        RegisterID* oldValue;
        if (dst == generator.ignoredResult()) {
            oldValue = 0;
            emitPreIncOrDec(generator, value.get(), m_operator);
        } else {
            oldValue = emitPostIncOrDec(generator, generator.finalDestination(dst), value.get(), m_operator);
        }
        generator.emitPutScopedVar(depth, index, value.get(), globalObject);
        return oldValue;
    }

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    RefPtr<RegisterID> value = generator.newTemporary();
    RefPtr<RegisterID> base = generator.emitResolveWithBase(generator.newTemporary(), value.get(), m_ident);
    RegisterID* oldValue;
    if (dst == generator.ignoredResult()) {
        oldValue = 0;
        emitPreIncOrDec(generator, value.get(), m_operator);
    } else {
        oldValue = emitPostIncOrDec(generator, generator.finalDestination(dst), value.get(), m_operator);
    }
    generator.emitPutById(base.get(), m_ident, value.get());
    return oldValue;
}

// ------------------------------ PostfixBracketNode ----------------------------------

RegisterID* PostfixBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNode(m_base);
    RefPtr<RegisterID> property = generator.emitNode(m_subscript);

    generator.emitExpressionInfo(divot() - m_subexpressionDivotOffset, startOffset() - m_subexpressionDivotOffset, m_subexpressionEndOffset);
    RefPtr<RegisterID> value = generator.emitGetByVal(generator.newTemporary(), base.get(), property.get());
    RegisterID* oldValue;
    if (dst == generator.ignoredResult()) {
        oldValue = 0;
        if (m_operator == OpPlusPlus)
            generator.emitPreInc(value.get());
        else
            generator.emitPreDec(value.get());
    } else {
        oldValue = (m_operator == OpPlusPlus) ? generator.emitPostInc(generator.finalDestination(dst), value.get()) : generator.emitPostDec(generator.finalDestination(dst), value.get());
    }
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitPutByVal(base.get(), property.get(), value.get());
    return oldValue;
}

// ------------------------------ PostfixDotNode ----------------------------------

RegisterID* PostfixDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNode(m_base);

    generator.emitExpressionInfo(divot() - m_subexpressionDivotOffset, startOffset() - m_subexpressionDivotOffset, m_subexpressionEndOffset);
    RefPtr<RegisterID> value = generator.emitGetById(generator.newTemporary(), base.get(), m_ident);
    RegisterID* oldValue;
    if (dst == generator.ignoredResult()) {
        oldValue = 0;
        if (m_operator == OpPlusPlus)
            generator.emitPreInc(value.get());
        else
            generator.emitPreDec(value.get());
    } else {
        oldValue = (m_operator == OpPlusPlus) ? generator.emitPostInc(generator.finalDestination(dst), value.get()) : generator.emitPostDec(generator.finalDestination(dst), value.get());
    }
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitPutById(base.get(), m_ident, value.get());
    return oldValue;
}

// ------------------------------ PostfixErrorNode -----------------------------------

RegisterID* PostfixErrorNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    return emitThrowReferenceError(generator, m_operator == OpPlusPlus
        ? "Postfix ++ operator applied to value that is not a reference."
        : "Postfix -- operator applied to value that is not a reference.");
}

// ------------------------------ DeleteResolveNode -----------------------------------

RegisterID* DeleteResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (generator.registerFor(m_ident))
        return generator.emitLoad(generator.finalDestination(dst), false);

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    RegisterID* base = generator.emitResolveBase(generator.tempDestination(dst), m_ident);
    return generator.emitDeleteById(generator.finalDestination(dst, base), base, m_ident);
}

// ------------------------------ DeleteBracketNode -----------------------------------

RegisterID* DeleteBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> r0 = generator.emitNode(m_base);
    RegisterID* r1 = generator.emitNode(m_subscript);

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    return generator.emitDeleteByVal(generator.finalDestination(dst), r0.get(), r1);
}

// ------------------------------ DeleteDotNode -----------------------------------

RegisterID* DeleteDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RegisterID* r0 = generator.emitNode(m_base);

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    return generator.emitDeleteById(generator.finalDestination(dst), r0, m_ident);
}

// ------------------------------ DeleteValueNode -----------------------------------

RegisterID* DeleteValueNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitNode(generator.ignoredResult(), m_expr);

    // delete on a non-location expression ignores the value and returns true
    return generator.emitLoad(generator.finalDestination(dst), true);
}

// ------------------------------ VoidNode -------------------------------------

RegisterID* VoidNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult()) {
        generator.emitNode(generator.ignoredResult(), m_expr);
        return 0;
    }
    RefPtr<RegisterID> r0 = generator.emitNode(m_expr);
    return generator.emitLoad(dst, jsUndefined());
}

// ------------------------------ TypeOfValueNode -----------------------------------

RegisterID* TypeOfResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (RegisterID* local = generator.registerFor(m_ident)) {
        if (dst == generator.ignoredResult())
            return 0;
        return generator.emitTypeOf(generator.finalDestination(dst), local);
    }

    RefPtr<RegisterID> scratch = generator.emitResolveBase(generator.tempDestination(dst), m_ident);
    generator.emitGetById(scratch.get(), scratch.get(), m_ident);
    if (dst == generator.ignoredResult())
        return 0;
    return generator.emitTypeOf(generator.finalDestination(dst, scratch.get()), scratch.get());
}

// ------------------------------ TypeOfValueNode -----------------------------------

RegisterID* TypeOfValueNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult()) {
        generator.emitNode(generator.ignoredResult(), m_expr);
        return 0;
    }
    RefPtr<RegisterID> src = generator.emitNode(m_expr);
    return generator.emitTypeOf(generator.finalDestination(dst), src.get());
}

// ------------------------------ PrefixResolveNode ----------------------------------

RegisterID* PrefixResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (RegisterID* local = generator.registerFor(m_ident)) {
        if (generator.isLocalConstant(m_ident)) {
            if (dst == generator.ignoredResult())
                return 0;
            RefPtr<RegisterID> r0 = generator.emitLoad(generator.finalDestination(dst), (m_operator == OpPlusPlus) ? 1.0 : -1.0);
            return generator.emitBinaryOp(op_add, r0.get(), local, r0.get(), OperandTypes());
        }

        emitPreIncOrDec(generator, local, m_operator);
        return generator.moveToDestinationIfNeeded(dst, local);
    }

    int index = 0;
    size_t depth = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    if (generator.findScopedProperty(m_ident, index, depth, false, requiresDynamicChecks, globalObject) && index != missingSymbolMarker() && !requiresDynamicChecks) {
        RefPtr<RegisterID> propDst = generator.emitGetScopedVar(generator.tempDestination(dst), depth, index, globalObject);
        emitPreIncOrDec(generator, propDst.get(), m_operator);
        generator.emitPutScopedVar(depth, index, propDst.get(), globalObject);
        return generator.moveToDestinationIfNeeded(dst, propDst.get());
    }

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    RefPtr<RegisterID> propDst = generator.tempDestination(dst);
    RefPtr<RegisterID> base = generator.emitResolveWithBase(generator.newTemporary(), propDst.get(), m_ident);
    emitPreIncOrDec(generator, propDst.get(), m_operator);
    generator.emitPutById(base.get(), m_ident, propDst.get());
    return generator.moveToDestinationIfNeeded(dst, propDst.get());
}

// ------------------------------ PrefixBracketNode ----------------------------------

RegisterID* PrefixBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNode(m_base);
    RefPtr<RegisterID> property = generator.emitNode(m_subscript);
    RefPtr<RegisterID> propDst = generator.tempDestination(dst);

    generator.emitExpressionInfo(divot() + m_subexpressionDivotOffset, m_subexpressionStartOffset, endOffset() - m_subexpressionDivotOffset);
    RegisterID* value = generator.emitGetByVal(propDst.get(), base.get(), property.get());
    if (m_operator == OpPlusPlus)
        generator.emitPreInc(value);
    else
        generator.emitPreDec(value);
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitPutByVal(base.get(), property.get(), value);
    return generator.moveToDestinationIfNeeded(dst, propDst.get());
}

// ------------------------------ PrefixDotNode ----------------------------------

RegisterID* PrefixDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNode(m_base);
    RefPtr<RegisterID> propDst = generator.tempDestination(dst);

    generator.emitExpressionInfo(divot() + m_subexpressionDivotOffset, m_subexpressionStartOffset, endOffset() - m_subexpressionDivotOffset);
    RegisterID* value = generator.emitGetById(propDst.get(), base.get(), m_ident);
    if (m_operator == OpPlusPlus)
        generator.emitPreInc(value);
    else
        generator.emitPreDec(value);
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitPutById(base.get(), m_ident, value);
    return generator.moveToDestinationIfNeeded(dst, propDst.get());
}

// ------------------------------ PrefixErrorNode -----------------------------------

RegisterID* PrefixErrorNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    return emitThrowReferenceError(generator, m_operator == OpPlusPlus
        ? "Prefix ++ operator applied to value that is not a reference."
        : "Prefix -- operator applied to value that is not a reference.");
}

// ------------------------------ Unary Operation Nodes -----------------------------------

RegisterID* UnaryOpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RegisterID* src = generator.emitNode(m_expr);
    return generator.emitUnaryOp(opcodeID(), generator.finalDestination(dst), src);
}


// ------------------------------ LogicalNotNode -----------------------------------

void LogicalNotNode::emitBytecodeInConditionContext(BytecodeGenerator& generator, Label* trueTarget, Label* falseTarget, bool fallThroughMeansTrue)
{
    ASSERT(expr()->hasConditionContextCodegen());

    // reverse the true and false targets
    generator.emitNodeInConditionContext(expr(), falseTarget, trueTarget, !fallThroughMeansTrue);
}


// ------------------------------ Binary Operation Nodes -----------------------------------

// BinaryOpNode::emitStrcat:
//
// This node generates an op_strcat operation.  This opcode can handle concatenation of three or
// more values, where we can determine a set of separate op_add operations would be operating on
// string values.
//
// This function expects to be operating on a graph of AST nodes looking something like this:
//
//     (a)...     (b)
//          \   /
//           (+)     (c)
//              \   /
//      [d]     ((+))
//         \    /
//          [+=]
//
// The assignment operation is optional, if it exists the register holding the value on the
// lefthand side of the assignment should be passing as the optional 'lhs' argument.
//
// The method should be called on the node at the root of the tree of regular binary add
// operations (marked in the diagram with a double set of parentheses).  This node must
// be performing a string concatenation (determined by statically detecting that at least
// one child must be a string).  
//
// Since the minimum number of values being concatenated together is expected to be 3, if
// a lhs to a concatenating assignment is not provided then the  root add should have at
// least one left child that is also an add that can be determined to be operating on strings.
//
RegisterID* BinaryOpNode::emitStrcat(BytecodeGenerator& generator, RegisterID* dst, RegisterID* lhs, ReadModifyResolveNode* emitExpressionInfoForMe)
{
    ASSERT(isAdd());
    ASSERT(resultDescriptor().definitelyIsString());

    // Create a list of expressions for all the adds in the tree of nodes we can convert into
    // a string concatenation.  The rightmost node (c) is added first.  The rightmost node is
    // added first, and the leftmost child is never added, so the vector produced for the
    // example above will be [ c, b ].
    Vector<ExpressionNode*, 16> reverseExpressionList;
    reverseExpressionList.append(m_expr2);

    // Examine the left child of the add.  So long as this is a string add, add its right-child
    // to the list, and keep processing along the left fork.
    ExpressionNode* leftMostAddChild = m_expr1;
    while (leftMostAddChild->isAdd() && leftMostAddChild->resultDescriptor().definitelyIsString()) {
        reverseExpressionList.append(static_cast<AddNode*>(leftMostAddChild)->m_expr2);
        leftMostAddChild = static_cast<AddNode*>(leftMostAddChild)->m_expr1;
    }

    Vector<RefPtr<RegisterID>, 16> temporaryRegisters;

    // If there is an assignment, allocate a temporary to hold the lhs after conversion.
    // We could possibly avoid this (the lhs is converted last anyway, we could let the
    // op_strcat node handle its conversion if required).
    if (lhs)
        temporaryRegisters.append(generator.newTemporary());

    // Emit code for the leftmost node ((a) in the example).
    temporaryRegisters.append(generator.newTemporary());
    RegisterID* leftMostAddChildTempRegister = temporaryRegisters.last().get();
    generator.emitNode(leftMostAddChildTempRegister, leftMostAddChild);

    // Note on ordering of conversions:
    //
    // We maintain the same ordering of conversions as we would see if the concatenations
    // was performed as a sequence of adds (otherwise this optimization could change
    // behaviour should an object have been provided a valueOf or toString method).
    //
    // Considering the above example, the sequnce of execution is:
    //     * evaluate operand (a)
    //     * evaluate operand (b)
    //     * convert (a) to primitive   <-  (this would be triggered by the first add)
    //     * convert (b) to primitive   <-  (ditto)
    //     * evaluate operand (c)
    //     * convert (c) to primitive   <-  (this would be triggered by the second add)
    // And optionally, if there is an assignment:
    //     * convert (d) to primitive   <-  (this would be triggered by the assigning addition)
    //
    // As such we do not plant an op to convert the leftmost child now.  Instead, use
    // 'leftMostAddChildTempRegister' as a flag to trigger generation of the conversion
    // once the second node has been generated.  However, if the leftmost child is an
    // immediate we can trivially determine that no conversion will be required.
    // If this is the case
    if (leftMostAddChild->isString())
        leftMostAddChildTempRegister = 0;

    while (reverseExpressionList.size()) {
        ExpressionNode* node = reverseExpressionList.last();
        reverseExpressionList.removeLast();

        // Emit the code for the current node.
        temporaryRegisters.append(generator.newTemporary());
        generator.emitNode(temporaryRegisters.last().get(), node);

        // On the first iteration of this loop, when we first reach this point we have just
        // generated the second node, which means it is time to convert the leftmost operand.
        if (leftMostAddChildTempRegister) {
            generator.emitToPrimitive(leftMostAddChildTempRegister, leftMostAddChildTempRegister);
            leftMostAddChildTempRegister = 0; // Only do this once.
        }
        // Plant a conversion for this node, if necessary.
        if (!node->isString())
            generator.emitToPrimitive(temporaryRegisters.last().get(), temporaryRegisters.last().get());
    }
    ASSERT(temporaryRegisters.size() >= 3);

    // Certain read-modify nodes require expression info to be emitted *after* m_right has been generated.
    // If this is required the node is passed as 'emitExpressionInfoForMe'; do so now.
    if (emitExpressionInfoForMe)
        generator.emitExpressionInfo(emitExpressionInfoForMe->divot(), emitExpressionInfoForMe->startOffset(), emitExpressionInfoForMe->endOffset());

    // If there is an assignment convert the lhs now.  This will also copy lhs to
    // the temporary register we allocated for it.
    if (lhs)
        generator.emitToPrimitive(temporaryRegisters[0].get(), lhs);

    return generator.emitStrcat(generator.finalDestination(dst, temporaryRegisters[0].get()), temporaryRegisters[0].get(), temporaryRegisters.size());
}

RegisterID* BinaryOpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    OpcodeID opcodeID = this->opcodeID();

    if (opcodeID == op_add && m_expr1->isAdd() && m_expr1->resultDescriptor().definitelyIsString())
        return emitStrcat(generator, dst);

    if (opcodeID == op_neq) {
        if (m_expr1->isNull() || m_expr2->isNull()) {
            RefPtr<RegisterID> src = generator.tempDestination(dst);
            generator.emitNode(src.get(), m_expr1->isNull() ? m_expr2 : m_expr1);
            return generator.emitUnaryOp(op_neq_null, generator.finalDestination(dst, src.get()), src.get());
        }
    }

    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(m_expr1, m_rightHasAssignments, m_expr2->isPure(generator));
    RegisterID* src2 = generator.emitNode(m_expr2);
    return generator.emitBinaryOp(opcodeID, generator.finalDestination(dst, src1.get()), src1.get(), src2, OperandTypes(m_expr1->resultDescriptor(), m_expr2->resultDescriptor()));
}

RegisterID* EqualNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_expr1->isNull() || m_expr2->isNull()) {
        RefPtr<RegisterID> src = generator.tempDestination(dst);
        generator.emitNode(src.get(), m_expr1->isNull() ? m_expr2 : m_expr1);
        return generator.emitUnaryOp(op_eq_null, generator.finalDestination(dst, src.get()), src.get());
    }

    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(m_expr1, m_rightHasAssignments, m_expr2->isPure(generator));
    RegisterID* src2 = generator.emitNode(m_expr2);
    return generator.emitEqualityOp(op_eq, generator.finalDestination(dst, src1.get()), src1.get(), src2);
}

RegisterID* StrictEqualNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(m_expr1, m_rightHasAssignments, m_expr2->isPure(generator));
    RegisterID* src2 = generator.emitNode(m_expr2);
    return generator.emitEqualityOp(op_stricteq, generator.finalDestination(dst, src1.get()), src1.get(), src2);
}

RegisterID* ReverseBinaryOpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(m_expr1, m_rightHasAssignments, m_expr2->isPure(generator));
    RegisterID* src2 = generator.emitNode(m_expr2);
    return generator.emitBinaryOp(opcodeID(), generator.finalDestination(dst, src1.get()), src2, src1.get(), OperandTypes(m_expr2->resultDescriptor(), m_expr1->resultDescriptor()));
}

RegisterID* ThrowableBinaryOpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(m_expr1, m_rightHasAssignments, m_expr2->isPure(generator));
    RegisterID* src2 = generator.emitNode(m_expr2);
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    return generator.emitBinaryOp(opcodeID(), generator.finalDestination(dst, src1.get()), src1.get(), src2, OperandTypes(m_expr1->resultDescriptor(), m_expr2->resultDescriptor()));
}

RegisterID* InstanceOfNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(m_expr1, m_rightHasAssignments, m_expr2->isPure(generator));
    RefPtr<RegisterID> src2 = generator.emitNode(m_expr2);

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitCheckHasInstance(src2.get());

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    RegisterID* src2Prototype = generator.emitGetById(generator.newTemporary(), src2.get(), generator.globalData()->propertyNames->prototype);

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    return generator.emitInstanceOf(generator.finalDestination(dst, src1.get()), src1.get(), src2.get(), src2Prototype);
}

// ------------------------------ LogicalOpNode ----------------------------

RegisterID* LogicalOpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> temp = generator.tempDestination(dst);
    RefPtr<Label> target = generator.newLabel();
    
    generator.emitNode(temp.get(), m_expr1);
    if (m_operator == OpLogicalAnd)
        generator.emitJumpIfFalse(temp.get(), target.get());
    else
        generator.emitJumpIfTrue(temp.get(), target.get());
    generator.emitNode(temp.get(), m_expr2);
    generator.emitLabel(target.get());

    return generator.moveToDestinationIfNeeded(dst, temp.get());
}

void LogicalOpNode::emitBytecodeInConditionContext(BytecodeGenerator& generator, Label* trueTarget, Label* falseTarget, bool fallThroughMeansTrue)
{
    if (m_expr1->hasConditionContextCodegen()) {
        RefPtr<Label> afterExpr1 = generator.newLabel();
        if (m_operator == OpLogicalAnd)
            generator.emitNodeInConditionContext(m_expr1, afterExpr1.get(), falseTarget, true);
        else 
            generator.emitNodeInConditionContext(m_expr1, trueTarget, afterExpr1.get(), false);
        generator.emitLabel(afterExpr1.get());
    } else {
        RegisterID* temp = generator.emitNode(m_expr1);
        if (m_operator == OpLogicalAnd)
            generator.emitJumpIfFalse(temp, falseTarget);
        else
            generator.emitJumpIfTrue(temp, trueTarget);
    }

    if (m_expr2->hasConditionContextCodegen())
        generator.emitNodeInConditionContext(m_expr2, trueTarget, falseTarget, fallThroughMeansTrue);
    else {
        RegisterID* temp = generator.emitNode(m_expr2);
        if (fallThroughMeansTrue)
            generator.emitJumpIfFalse(temp, falseTarget);
        else
            generator.emitJumpIfTrue(temp, trueTarget);
    }
}

// ------------------------------ ConditionalNode ------------------------------

RegisterID* ConditionalNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> newDst = generator.finalDestination(dst);
    RefPtr<Label> beforeElse = generator.newLabel();
    RefPtr<Label> afterElse = generator.newLabel();

    if (m_logical->hasConditionContextCodegen()) {
        RefPtr<Label> beforeThen = generator.newLabel();
        generator.emitNodeInConditionContext(m_logical, beforeThen.get(), beforeElse.get(), true);
        generator.emitLabel(beforeThen.get());
    } else {
        RegisterID* cond = generator.emitNode(m_logical);
        generator.emitJumpIfFalse(cond, beforeElse.get());
    }

    generator.emitNode(newDst.get(), m_expr1);
    generator.emitJump(afterElse.get());

    generator.emitLabel(beforeElse.get());
    generator.emitNode(newDst.get(), m_expr2);

    generator.emitLabel(afterElse.get());

    return newDst.get();
}

// ------------------------------ ReadModifyResolveNode -----------------------------------

// FIXME: should this be moved to be a method on BytecodeGenerator?
static ALWAYS_INLINE RegisterID* emitReadModifyAssignment(BytecodeGenerator& generator, RegisterID* dst, RegisterID* src1, ExpressionNode* m_right, Operator oper, OperandTypes types, ReadModifyResolveNode* emitExpressionInfoForMe = 0)
{
    OpcodeID opcodeID;
    switch (oper) {
        case OpMultEq:
            opcodeID = op_mul;
            break;
        case OpDivEq:
            opcodeID = op_div;
            break;
        case OpPlusEq:
            if (m_right->isAdd() && m_right->resultDescriptor().definitelyIsString())
                return static_cast<AddNode*>(m_right)->emitStrcat(generator, dst, src1, emitExpressionInfoForMe);
            opcodeID = op_add;
            break;
        case OpMinusEq:
            opcodeID = op_sub;
            break;
        case OpLShift:
            opcodeID = op_lshift;
            break;
        case OpRShift:
            opcodeID = op_rshift;
            break;
        case OpURShift:
            opcodeID = op_urshift;
            break;
        case OpAndEq:
            opcodeID = op_bitand;
            break;
        case OpXOrEq:
            opcodeID = op_bitxor;
            break;
        case OpOrEq:
            opcodeID = op_bitor;
            break;
        case OpModEq:
            opcodeID = op_mod;
            break;
        default:
            ASSERT_NOT_REACHED();
            return dst;
    }

    RegisterID* src2 = generator.emitNode(m_right);

    // Certain read-modify nodes require expression info to be emitted *after* m_right has been generated.
    // If this is required the node is passed as 'emitExpressionInfoForMe'; do so now.
    if (emitExpressionInfoForMe)
        generator.emitExpressionInfo(emitExpressionInfoForMe->divot(), emitExpressionInfoForMe->startOffset(), emitExpressionInfoForMe->endOffset());

    return generator.emitBinaryOp(opcodeID, dst, src1, src2, types);
}

RegisterID* ReadModifyResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (RegisterID* local = generator.registerFor(m_ident)) {
        if (generator.isLocalConstant(m_ident)) {
            return emitReadModifyAssignment(generator, generator.finalDestination(dst), local, m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));
        }
        
        if (generator.leftHandSideNeedsCopy(m_rightHasAssignments, m_right->isPure(generator))) {
            RefPtr<RegisterID> result = generator.newTemporary();
            generator.emitMove(result.get(), local);
            emitReadModifyAssignment(generator, result.get(), result.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));
            generator.emitMove(local, result.get());
            return generator.moveToDestinationIfNeeded(dst, result.get());
        }
        
        RegisterID* result = emitReadModifyAssignment(generator, local, local, m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));
        return generator.moveToDestinationIfNeeded(dst, result);
    }

    int index = 0;
    size_t depth = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    if (generator.findScopedProperty(m_ident, index, depth, true, requiresDynamicChecks, globalObject) && index != missingSymbolMarker() && !requiresDynamicChecks) {
        RefPtr<RegisterID> src1 = generator.emitGetScopedVar(generator.tempDestination(dst), depth, index, globalObject);
        RegisterID* result = emitReadModifyAssignment(generator, generator.finalDestination(dst, src1.get()), src1.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));
        generator.emitPutScopedVar(depth, index, result, globalObject);
        return result;
    }

    RefPtr<RegisterID> src1 = generator.tempDestination(dst);
    generator.emitExpressionInfo(divot() - startOffset() + m_ident.length(), m_ident.length(), 0);
    RefPtr<RegisterID> base = generator.emitResolveWithBase(generator.newTemporary(), src1.get(), m_ident);
    RegisterID* result = emitReadModifyAssignment(generator, generator.finalDestination(dst, src1.get()), src1.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()), this);
    return generator.emitPutById(base.get(), m_ident, result);
}

// ------------------------------ AssignResolveNode -----------------------------------

RegisterID* AssignResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (RegisterID* local = generator.registerFor(m_ident)) {
        if (generator.isLocalConstant(m_ident))
            return generator.emitNode(dst, m_right);
        
        RegisterID* result = generator.emitNode(local, m_right);
        return generator.moveToDestinationIfNeeded(dst, result);
    }

    int index = 0;
    size_t depth = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    if (generator.findScopedProperty(m_ident, index, depth, true, requiresDynamicChecks, globalObject) && index != missingSymbolMarker() && !requiresDynamicChecks) {
        if (dst == generator.ignoredResult())
            dst = 0;
        RegisterID* value = generator.emitNode(dst, m_right);
        generator.emitPutScopedVar(depth, index, value, globalObject);
        return value;
    }

    RefPtr<RegisterID> base = generator.emitResolveBaseForPut(generator.newTemporary(), m_ident);
    if (dst == generator.ignoredResult())
        dst = 0;
    RegisterID* value = generator.emitNode(dst, m_right);
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    return generator.emitPutById(base.get(), m_ident, value);
}

// ------------------------------ AssignDotNode -----------------------------------

RegisterID* AssignDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_rightHasAssignments, m_right->isPure(generator));
    RefPtr<RegisterID> value = generator.destinationForAssignResult(dst);
    RegisterID* result = generator.emitNode(value.get(), m_right);
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitPutById(base.get(), m_ident, result);
    return generator.moveToDestinationIfNeeded(dst, result);
}

// ------------------------------ ReadModifyDotNode -----------------------------------

RegisterID* ReadModifyDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_rightHasAssignments, m_right->isPure(generator));

    generator.emitExpressionInfo(divot() - m_subexpressionDivotOffset, startOffset() - m_subexpressionDivotOffset, m_subexpressionEndOffset);
    RefPtr<RegisterID> value = generator.emitGetById(generator.tempDestination(dst), base.get(), m_ident);
    RegisterID* updatedValue = emitReadModifyAssignment(generator, generator.finalDestination(dst, value.get()), value.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    return generator.emitPutById(base.get(), m_ident, updatedValue);
}

// ------------------------------ AssignErrorNode -----------------------------------

RegisterID* AssignErrorNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    return emitThrowReferenceError(generator, "Left side of assignment is not a reference.");
}

// ------------------------------ AssignBracketNode -----------------------------------

RegisterID* AssignBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_subscriptHasAssignments || m_rightHasAssignments, m_subscript->isPure(generator) && m_right->isPure(generator));
    RefPtr<RegisterID> property = generator.emitNodeForLeftHandSide(m_subscript, m_rightHasAssignments, m_right->isPure(generator));
    RefPtr<RegisterID> value = generator.destinationForAssignResult(dst);
    RegisterID* result = generator.emitNode(value.get(), m_right);

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitPutByVal(base.get(), property.get(), result);
    return generator.moveToDestinationIfNeeded(dst, result);
}

// ------------------------------ ReadModifyBracketNode -----------------------------------

RegisterID* ReadModifyBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_subscriptHasAssignments || m_rightHasAssignments, m_subscript->isPure(generator) && m_right->isPure(generator));
    RefPtr<RegisterID> property = generator.emitNodeForLeftHandSide(m_subscript, m_rightHasAssignments, m_right->isPure(generator));

    generator.emitExpressionInfo(divot() - m_subexpressionDivotOffset, startOffset() - m_subexpressionDivotOffset, m_subexpressionEndOffset);
    RefPtr<RegisterID> value = generator.emitGetByVal(generator.tempDestination(dst), base.get(), property.get());
    RegisterID* updatedValue = emitReadModifyAssignment(generator, generator.finalDestination(dst, value.get()), value.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));

    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitPutByVal(base.get(), property.get(), updatedValue);

    return updatedValue;
}

// ------------------------------ CommaNode ------------------------------------

RegisterID* CommaNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ASSERT(m_expressions.size() > 1);
    for (size_t i = 0; i < m_expressions.size() - 1; i++)
        generator.emitNode(generator.ignoredResult(), m_expressions[i]);
    return generator.emitNode(dst, m_expressions.last());
}

// ------------------------------ ConstDeclNode ------------------------------------

RegisterID* ConstDeclNode::emitCodeSingle(BytecodeGenerator& generator)
{
    if (RegisterID* local = generator.constRegisterFor(m_ident)) {
        if (!m_init)
            return local;

        return generator.emitNode(local, m_init);
    }

    if (generator.codeType() != EvalCode) {
        if (m_init)
            return generator.emitNode(m_init);
        else
            return generator.emitResolve(generator.newTemporary(), m_ident);
    }
    // FIXME: While this code should only be hit in eval code, it will potentially
    // assign to the wrong base if m_ident exists in an intervening dynamic scope.
    RefPtr<RegisterID> base = generator.emitResolveBase(generator.newTemporary(), m_ident);
    RegisterID* value = m_init ? generator.emitNode(m_init) : generator.emitLoad(0, jsUndefined());
    return generator.emitPutById(base.get(), m_ident, value);
}

RegisterID* ConstDeclNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    RegisterID* result = 0;
    for (ConstDeclNode* n = this; n; n = n->m_next)
        result = n->emitCodeSingle(generator);

    return result;
}

// ------------------------------ ConstStatementNode -----------------------------

RegisterID* ConstStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    return generator.emitNode(m_next);
}

// ------------------------------ SourceElements -------------------------------


inline StatementNode* SourceElements::lastStatement() const
{
    size_t size = m_statements.size();
    return size ? m_statements[size - 1] : 0;
}

inline void SourceElements::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    size_t size = m_statements.size();
    for (size_t i = 0; i < size; ++i)
        generator.emitNode(dst, m_statements[i]);
}

// ------------------------------ BlockNode ------------------------------------

inline StatementNode* BlockNode::lastStatement() const
{
    return m_statements ? m_statements->lastStatement() : 0;
}

inline StatementNode* BlockNode::singleStatement() const
{
    return m_statements ? m_statements->singleStatement() : 0;
}

RegisterID* BlockNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_statements)
        m_statements->emitBytecode(generator, dst);
    return 0;
}

// ------------------------------ EmptyStatementNode ---------------------------

RegisterID* EmptyStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    return dst;
}

// ------------------------------ DebuggerStatementNode ---------------------------

RegisterID* DebuggerStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(DidReachBreakpoint, firstLine(), lastLine());
    return dst;
}

// ------------------------------ ExprStatementNode ----------------------------

RegisterID* ExprStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ASSERT(m_expr);
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine()); 
    return generator.emitNode(dst, m_expr);
}

// ------------------------------ VarStatementNode ----------------------------

RegisterID* VarStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    ASSERT(m_expr);
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    return generator.emitNode(m_expr);
}

// ------------------------------ IfNode ---------------------------------------

RegisterID* IfNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    
    RefPtr<Label> afterThen = generator.newLabel();

    if (m_condition->hasConditionContextCodegen()) {
        RefPtr<Label> beforeThen = generator.newLabel();
        generator.emitNodeInConditionContext(m_condition, beforeThen.get(), afterThen.get(), true);
        generator.emitLabel(beforeThen.get());
    } else {
        RegisterID* cond = generator.emitNode(m_condition);
        generator.emitJumpIfFalse(cond, afterThen.get());
    }

    generator.emitNode(dst, m_ifBlock);
    generator.emitLabel(afterThen.get());

    // FIXME: This should return the last statement executed so that it can be returned as a Completion.
    return 0;
}

// ------------------------------ IfElseNode ---------------------------------------

RegisterID* IfElseNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    
    RefPtr<Label> beforeElse = generator.newLabel();
    RefPtr<Label> afterElse = generator.newLabel();

    if (m_condition->hasConditionContextCodegen()) {
        RefPtr<Label> beforeThen = generator.newLabel();
        generator.emitNodeInConditionContext(m_condition, beforeThen.get(), beforeElse.get(), true);
        generator.emitLabel(beforeThen.get());
    } else {
        RegisterID* cond = generator.emitNode(m_condition);
        generator.emitJumpIfFalse(cond, beforeElse.get());
    }

    generator.emitNode(dst, m_ifBlock);
    generator.emitJump(afterElse.get());

    generator.emitLabel(beforeElse.get());

    generator.emitNode(dst, m_elseBlock);

    generator.emitLabel(afterElse.get());

    // FIXME: This should return the last statement executed so that it can be returned as a Completion.
    return 0;
}

// ------------------------------ DoWhileNode ----------------------------------

RegisterID* DoWhileNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<LabelScope> scope = generator.newLabelScope(LabelScope::Loop);

    RefPtr<Label> topOfLoop = generator.newLabel();
    generator.emitLabel(topOfLoop.get());

    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
   
    RefPtr<RegisterID> result = generator.emitNode(dst, m_statement);

    generator.emitLabel(scope->continueTarget());
    generator.emitDebugHook(WillExecuteStatement, m_expr->lineNo(), m_expr->lineNo());
    if (m_expr->hasConditionContextCodegen())
        generator.emitNodeInConditionContext(m_expr, topOfLoop.get(), scope->breakTarget(), false);
    else {
        RegisterID* cond = generator.emitNode(m_expr);
        generator.emitJumpIfTrue(cond, topOfLoop.get());
    }

    generator.emitLabel(scope->breakTarget());
    return result.get();
}

// ------------------------------ WhileNode ------------------------------------

RegisterID* WhileNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<LabelScope> scope = generator.newLabelScope(LabelScope::Loop);

    generator.emitJump(scope->continueTarget());

    RefPtr<Label> topOfLoop = generator.newLabel();
    generator.emitLabel(topOfLoop.get());
    
    generator.emitNode(dst, m_statement);

    generator.emitLabel(scope->continueTarget());
    generator.emitDebugHook(WillExecuteStatement, m_expr->lineNo(), m_expr->lineNo());

    if (m_expr->hasConditionContextCodegen())
        generator.emitNodeInConditionContext(m_expr, topOfLoop.get(), scope->breakTarget(), false);
    else {
        RegisterID* cond = generator.emitNode(m_expr);
        generator.emitJumpIfTrue(cond, topOfLoop.get());
    }

    generator.emitLabel(scope->breakTarget());
    
    // FIXME: This should return the last statement executed so that it can be returned as a Completion
    return 0;
}

// ------------------------------ ForNode --------------------------------------

RegisterID* ForNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<LabelScope> scope = generator.newLabelScope(LabelScope::Loop);

    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());

    if (m_expr1)
        generator.emitNode(generator.ignoredResult(), m_expr1);

    RefPtr<Label> condition = generator.newLabel();
    generator.emitJump(condition.get());

    RefPtr<Label> topOfLoop = generator.newLabel();
    generator.emitLabel(topOfLoop.get());

    RefPtr<RegisterID> result = generator.emitNode(dst, m_statement);

    generator.emitLabel(scope->continueTarget());
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    if (m_expr3)
        generator.emitNode(generator.ignoredResult(), m_expr3);

    generator.emitLabel(condition.get());
    if (m_expr2) {
        if (m_expr2->hasConditionContextCodegen())
            generator.emitNodeInConditionContext(m_expr2, topOfLoop.get(), scope->breakTarget(), false);
        else {
            RegisterID* cond = generator.emitNode(m_expr2);
            generator.emitJumpIfTrue(cond, topOfLoop.get());
        }
    } else
        generator.emitJump(topOfLoop.get());

    generator.emitLabel(scope->breakTarget());
    return result.get();
}

// ------------------------------ ForInNode ------------------------------------

RegisterID* ForInNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<LabelScope> scope = generator.newLabelScope(LabelScope::Loop);

    if (!m_lexpr->isLocation())
        return emitThrowReferenceError(generator, "Left side of for-in statement is not a reference.");

    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());

    if (m_init)
        generator.emitNode(generator.ignoredResult(), m_init);

    RefPtr<RegisterID> base = generator.newTemporary();
    generator.emitNode(base.get(), m_expr);
    RefPtr<RegisterID> i = generator.newTemporary();
    RefPtr<RegisterID> size = generator.newTemporary();
    RefPtr<RegisterID> expectedSubscript;
    RefPtr<RegisterID> iter = generator.emitGetPropertyNames(generator.newTemporary(), base.get(), i.get(), size.get(), scope->breakTarget());
    generator.emitJump(scope->continueTarget());

    RefPtr<Label> loopStart = generator.newLabel();
    generator.emitLabel(loopStart.get());

    RegisterID* propertyName;
    bool optimizedForinAccess = false;
    if (m_lexpr->isResolveNode()) {
        const Identifier& ident = static_cast<ResolveNode*>(m_lexpr)->identifier();
        propertyName = generator.registerFor(ident);
        if (!propertyName) {
            propertyName = generator.newTemporary();
            RefPtr<RegisterID> protect = propertyName;
            RegisterID* base = generator.emitResolveBaseForPut(generator.newTemporary(), ident);

            generator.emitExpressionInfo(divot(), startOffset(), endOffset());
            generator.emitPutById(base, ident, propertyName);
        } else {
            expectedSubscript = generator.emitMove(generator.newTemporary(), propertyName);
            generator.pushOptimisedForIn(expectedSubscript.get(), iter.get(), i.get(), propertyName);
            optimizedForinAccess = true;
        }
    } else if (m_lexpr->isDotAccessorNode()) {
        DotAccessorNode* assignNode = static_cast<DotAccessorNode*>(m_lexpr);
        const Identifier& ident = assignNode->identifier();
        propertyName = generator.newTemporary();
        RefPtr<RegisterID> protect = propertyName;
        RegisterID* base = generator.emitNode(assignNode->base());

        generator.emitExpressionInfo(assignNode->divot(), assignNode->startOffset(), assignNode->endOffset());
        generator.emitPutById(base, ident, propertyName);
    } else {
        ASSERT(m_lexpr->isBracketAccessorNode());
        BracketAccessorNode* assignNode = static_cast<BracketAccessorNode*>(m_lexpr);
        propertyName = generator.newTemporary();
        RefPtr<RegisterID> protect = propertyName;
        RefPtr<RegisterID> base = generator.emitNode(assignNode->base());
        RegisterID* subscript = generator.emitNode(assignNode->subscript());
        
        generator.emitExpressionInfo(assignNode->divot(), assignNode->startOffset(), assignNode->endOffset());
        generator.emitPutByVal(base.get(), subscript, propertyName);
    }   

    generator.emitNode(dst, m_statement);

    if (optimizedForinAccess)
        generator.popOptimisedForIn();

    generator.emitLabel(scope->continueTarget());
    generator.emitNextPropertyName(propertyName, base.get(), i.get(), size.get(), iter.get(), loopStart.get());
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    generator.emitLabel(scope->breakTarget());
    return dst;
}

// ------------------------------ ContinueNode ---------------------------------

// ECMA 12.7
RegisterID* ContinueNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    
    LabelScope* scope = generator.continueTarget(m_ident);
    ASSERT(scope);

    generator.emitJumpScopes(scope->continueTarget(), scope->scopeDepth());
    return dst;
}

// ------------------------------ BreakNode ------------------------------------

// ECMA 12.8
RegisterID* BreakNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    
    LabelScope* scope = generator.breakTarget(m_ident);
    ASSERT(scope);

    generator.emitJumpScopes(scope->breakTarget(), scope->scopeDepth());
    return dst;
}

// ------------------------------ ReturnNode -----------------------------------

RegisterID* ReturnNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    ASSERT(generator.codeType() == FunctionCode);

    if (dst == generator.ignoredResult())
        dst = 0;
    RegisterID* r0 = m_value ? generator.emitNode(dst, m_value) : generator.emitLoad(dst, jsUndefined());
    RefPtr<RegisterID> returnRegister;
    if (generator.scopeDepth()) {
        RefPtr<Label> l0 = generator.newLabel();
        if (generator.hasFinaliser() && !r0->isTemporary()) {
            returnRegister = generator.emitMove(generator.newTemporary(), r0);
            r0 = returnRegister.get();
        }
        generator.emitJumpScopes(l0.get(), 0);
        generator.emitLabel(l0.get());
    }
    generator.emitDebugHook(WillLeaveCallFrame, firstLine(), lastLine());
    return generator.emitReturn(r0);
}

// ------------------------------ WithNode -------------------------------------

RegisterID* WithNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    
    RefPtr<RegisterID> scope = generator.newTemporary();
    generator.emitNode(scope.get(), m_expr); // scope must be protected until popped
    generator.emitExpressionInfo(m_divot, m_expressionLength, 0);
    generator.emitPushScope(scope.get());
    RegisterID* result = generator.emitNode(dst, m_statement);
    generator.emitPopScope();
    return result;
}

// ------------------------------ CaseClauseNode --------------------------------

inline void CaseClauseNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_statements)
        m_statements->emitBytecode(generator, dst);
}

// ------------------------------ CaseBlockNode --------------------------------

enum SwitchKind { 
    SwitchUnset = 0,
    SwitchNumber = 1, 
    SwitchString = 2, 
    SwitchNeither = 3 
};

static void processClauseList(ClauseListNode* list, Vector<ExpressionNode*, 8>& literalVector, SwitchKind& typeForTable, bool& singleCharacterSwitch, int32_t& min_num, int32_t& max_num)
{
    for (; list; list = list->getNext()) {
        ExpressionNode* clauseExpression = list->getClause()->expr();
        literalVector.append(clauseExpression);
        if (clauseExpression->isNumber()) {
            double value = static_cast<NumberNode*>(clauseExpression)->value();
            int32_t intVal = static_cast<int32_t>(value);
            if ((typeForTable & ~SwitchNumber) || (intVal != value)) {
                typeForTable = SwitchNeither;
                break;
            }
            if (intVal < min_num)
                min_num = intVal;
            if (intVal > max_num)
                max_num = intVal;
            typeForTable = SwitchNumber;
            continue;
        }
        if (clauseExpression->isString()) {
            if (typeForTable & ~SwitchString) {
                typeForTable = SwitchNeither;
                break;
            }
            const UString& value = static_cast<StringNode*>(clauseExpression)->value().ustring();
            if (singleCharacterSwitch &= value.length() == 1) {
                int32_t intVal = value.impl()->characters()[0];
                if (intVal < min_num)
                    min_num = intVal;
                if (intVal > max_num)
                    max_num = intVal;
            }
            typeForTable = SwitchString;
            continue;
        }
        typeForTable = SwitchNeither;
        break;        
    }
}
    
SwitchInfo::SwitchType CaseBlockNode::tryOptimizedSwitch(Vector<ExpressionNode*, 8>& literalVector, int32_t& min_num, int32_t& max_num)
{
    SwitchKind typeForTable = SwitchUnset;
    bool singleCharacterSwitch = true;
    
    processClauseList(m_list1, literalVector, typeForTable, singleCharacterSwitch, min_num, max_num);
    processClauseList(m_list2, literalVector, typeForTable, singleCharacterSwitch, min_num, max_num);
    
    if (typeForTable == SwitchUnset || typeForTable == SwitchNeither)
        return SwitchInfo::SwitchNone;
    
    if (typeForTable == SwitchNumber) {
        int32_t range = max_num - min_num;
        if (min_num <= max_num && range <= 1000 && (range / literalVector.size()) < 10)
            return SwitchInfo::SwitchImmediate;
        return SwitchInfo::SwitchNone;
    } 
    
    ASSERT(typeForTable == SwitchString);
    
    if (singleCharacterSwitch) {
        int32_t range = max_num - min_num;
        if (min_num <= max_num && range <= 1000 && (range / literalVector.size()) < 10)
            return SwitchInfo::SwitchCharacter;
    }

    return SwitchInfo::SwitchString;
}

RegisterID* CaseBlockNode::emitBytecodeForBlock(BytecodeGenerator& generator, RegisterID* switchExpression, RegisterID* dst)
{
    RefPtr<Label> defaultLabel;
    Vector<RefPtr<Label>, 8> labelVector;
    Vector<ExpressionNode*, 8> literalVector;
    int32_t min_num = std::numeric_limits<int32_t>::max();
    int32_t max_num = std::numeric_limits<int32_t>::min();
    SwitchInfo::SwitchType switchType = tryOptimizedSwitch(literalVector, min_num, max_num);

    if (switchType != SwitchInfo::SwitchNone) {
        // Prepare the various labels
        for (uint32_t i = 0; i < literalVector.size(); i++)
            labelVector.append(generator.newLabel());
        defaultLabel = generator.newLabel();
        generator.beginSwitch(switchExpression, switchType);
    } else {
        // Setup jumps
        for (ClauseListNode* list = m_list1; list; list = list->getNext()) {
            RefPtr<RegisterID> clauseVal = generator.newTemporary();
            generator.emitNode(clauseVal.get(), list->getClause()->expr());
            generator.emitBinaryOp(op_stricteq, clauseVal.get(), clauseVal.get(), switchExpression, OperandTypes());
            labelVector.append(generator.newLabel());
            generator.emitJumpIfTrue(clauseVal.get(), labelVector[labelVector.size() - 1].get());
        }
        
        for (ClauseListNode* list = m_list2; list; list = list->getNext()) {
            RefPtr<RegisterID> clauseVal = generator.newTemporary();
            generator.emitNode(clauseVal.get(), list->getClause()->expr());
            generator.emitBinaryOp(op_stricteq, clauseVal.get(), clauseVal.get(), switchExpression, OperandTypes());
            labelVector.append(generator.newLabel());
            generator.emitJumpIfTrue(clauseVal.get(), labelVector[labelVector.size() - 1].get());
        }
        defaultLabel = generator.newLabel();
        generator.emitJump(defaultLabel.get());
    }

    RegisterID* result = 0;

    size_t i = 0;
    for (ClauseListNode* list = m_list1; list; list = list->getNext()) {
        generator.emitLabel(labelVector[i++].get());
        list->getClause()->emitBytecode(generator, dst);
    }

    if (m_defaultClause) {
        generator.emitLabel(defaultLabel.get());
        m_defaultClause->emitBytecode(generator, dst);
    }

    for (ClauseListNode* list = m_list2; list; list = list->getNext()) {
        generator.emitLabel(labelVector[i++].get());
        list->getClause()->emitBytecode(generator, dst);
    }
    if (!m_defaultClause)
        generator.emitLabel(defaultLabel.get());

    ASSERT(i == labelVector.size());
    if (switchType != SwitchInfo::SwitchNone) {
        ASSERT(labelVector.size() == literalVector.size());
        generator.endSwitch(labelVector.size(), labelVector.data(), literalVector.data(), defaultLabel.get(), min_num, max_num);
    }
    return result;
}

// ------------------------------ SwitchNode -----------------------------------

RegisterID* SwitchNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());
    
    RefPtr<LabelScope> scope = generator.newLabelScope(LabelScope::Switch);

    RefPtr<RegisterID> r0 = generator.emitNode(m_expr);
    RegisterID* r1 = m_block->emitBytecodeForBlock(generator, r0.get(), dst);

    generator.emitLabel(scope->breakTarget());
    return r1;
}

// ------------------------------ LabelNode ------------------------------------

RegisterID* LabelNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());

    ASSERT(!generator.breakTarget(m_name));

    RefPtr<LabelScope> scope = generator.newLabelScope(LabelScope::NamedLabel, &m_name);
    RegisterID* r0 = generator.emitNode(dst, m_statement);

    generator.emitLabel(scope->breakTarget());
    return r0;
}

// ------------------------------ ThrowNode ------------------------------------

RegisterID* ThrowNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());

    if (dst == generator.ignoredResult())
        dst = 0;
    RefPtr<RegisterID> expr = generator.emitNode(m_expr);
    generator.emitExpressionInfo(divot(), startOffset(), endOffset());
    generator.emitThrow(expr.get());
    return 0;
}

// ------------------------------ TryNode --------------------------------------

RegisterID* TryNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    // NOTE: The catch and finally blocks must be labeled explicitly, so the
    // optimizer knows they may be jumped to from anywhere.

    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine());

    RefPtr<Label> tryStartLabel = generator.newLabel();
    RefPtr<Label> finallyStart;
    RefPtr<RegisterID> finallyReturnAddr;
    if (m_finallyBlock) {
        finallyStart = generator.newLabel();
        finallyReturnAddr = generator.newTemporary();
        generator.pushFinallyContext(finallyStart.get(), finallyReturnAddr.get());
    }

    generator.emitLabel(tryStartLabel.get());
    generator.emitNode(dst, m_tryBlock);

    if (m_catchBlock) {
        RefPtr<Label> catchEndLabel = generator.newLabel();
        
        // Normal path: jump over the catch block.
        generator.emitJump(catchEndLabel.get());

        // Uncaught exception path: the catch block.
        RefPtr<Label> here = generator.emitLabel(generator.newLabel().get());
        RefPtr<RegisterID> exceptionRegister = generator.emitCatch(generator.newTemporary(), tryStartLabel.get(), here.get());
        if (m_catchHasEval) {
            RefPtr<RegisterID> dynamicScopeObject = generator.emitNewObject(generator.newTemporary());
            generator.emitPutById(dynamicScopeObject.get(), m_exceptionIdent, exceptionRegister.get());
            generator.emitMove(exceptionRegister.get(), dynamicScopeObject.get());
            generator.emitPushScope(exceptionRegister.get());
        } else
            generator.emitPushNewScope(exceptionRegister.get(), m_exceptionIdent, exceptionRegister.get());
        generator.emitNode(dst, m_catchBlock);
        generator.emitPopScope();
        generator.emitLabel(catchEndLabel.get());
    }

    if (m_finallyBlock) {
        generator.popFinallyContext();
        // there may be important registers live at the time we jump
        // to a finally block (such as for a return or throw) so we
        // ref the highest register ever used as a conservative
        // approach to not clobbering anything important
        RefPtr<RegisterID> highestUsedRegister = generator.highestUsedRegister();
        RefPtr<Label> finallyEndLabel = generator.newLabel();

        // Normal path: invoke the finally block, then jump over it.
        generator.emitJumpSubroutine(finallyReturnAddr.get(), finallyStart.get());
        generator.emitJump(finallyEndLabel.get());

        // Uncaught exception path: invoke the finally block, then re-throw the exception.
        RefPtr<Label> here = generator.emitLabel(generator.newLabel().get());
        RefPtr<RegisterID> tempExceptionRegister = generator.emitCatch(generator.newTemporary(), tryStartLabel.get(), here.get());
        generator.emitJumpSubroutine(finallyReturnAddr.get(), finallyStart.get());
        generator.emitThrow(tempExceptionRegister.get());

        // The finally block.
        generator.emitLabel(finallyStart.get());
        generator.emitNode(dst, m_finallyBlock);
        generator.emitSubroutineReturn(finallyReturnAddr.get());

        generator.emitLabel(finallyEndLabel.get());
    }

    return dst;
}

// ------------------------------ ScopeNode -----------------------------

inline void ScopeNode::emitStatementsBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_data->m_statements)
        m_data->m_statements->emitBytecode(generator, dst);
}

// ------------------------------ ProgramNode -----------------------------

RegisterID* ProgramNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteProgram, firstLine(), lastLine());

    RefPtr<RegisterID> dstRegister = generator.newTemporary();
    generator.emitLoad(dstRegister.get(), jsUndefined());
    emitStatementsBytecode(generator, dstRegister.get());

    generator.emitDebugHook(DidExecuteProgram, firstLine(), lastLine());
    generator.emitEnd(dstRegister.get());
    return 0;
}

// ------------------------------ EvalNode -----------------------------

RegisterID* EvalNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteProgram, firstLine(), lastLine());

    RefPtr<RegisterID> dstRegister = generator.newTemporary();
    generator.emitLoad(dstRegister.get(), jsUndefined());
    emitStatementsBytecode(generator, dstRegister.get());

    generator.emitDebugHook(DidExecuteProgram, firstLine(), lastLine());
    generator.emitEnd(dstRegister.get());
    return 0;
}

// ------------------------------ FunctionBodyNode -----------------------------

RegisterID* FunctionBodyNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(DidEnterCallFrame, firstLine(), lastLine());
    emitStatementsBytecode(generator, generator.ignoredResult());

    StatementNode* singleStatement = this->singleStatement();
    ReturnNode* returnNode = 0;

    // Check for a return statement at the end of a function composed of a single block.
    if (singleStatement && singleStatement->isBlock()) {
        StatementNode* lastStatementInBlock = static_cast<BlockNode*>(singleStatement)->lastStatement();
        if (lastStatementInBlock && lastStatementInBlock->isReturnNode())
            returnNode = static_cast<ReturnNode*>(lastStatementInBlock);
    }

    // If there is no return we must automatically insert one.
    if (!returnNode) {
        RegisterID* r0 = generator.isConstructor() ? generator.thisRegister() : generator.emitLoad(0, jsUndefined());
        generator.emitDebugHook(WillLeaveCallFrame, firstLine(), lastLine());
        generator.emitReturn(r0);
        return 0;
    }

    // If there is a return statment, and it is the only statement in the function, check if this is a numeric compare.
    if (static_cast<BlockNode*>(singleStatement)->singleStatement()) {
        ExpressionNode* returnValueExpression = returnNode->value();
        if (returnValueExpression && returnValueExpression->isSubtract()) {
            ExpressionNode* lhsExpression = static_cast<SubNode*>(returnValueExpression)->lhs();
            ExpressionNode* rhsExpression = static_cast<SubNode*>(returnValueExpression)->rhs();
            if (lhsExpression->isResolveNode() && rhsExpression->isResolveNode()) {
                generator.setIsNumericCompareFunction(generator.argumentNumberFor(static_cast<ResolveNode*>(lhsExpression)->identifier()) == 1
                    && generator.argumentNumberFor(static_cast<ResolveNode*>(rhsExpression)->identifier()) == 2);
            }
        }
    }

    return 0;
}

// ------------------------------ FuncDeclNode ---------------------------------

RegisterID* FuncDeclNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        dst = 0;
    return dst;
}

// ------------------------------ FuncExprNode ---------------------------------

RegisterID* FuncExprNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    return generator.emitNewFunctionExpression(generator.finalDestination(dst), this);
}

} // namespace JSC
