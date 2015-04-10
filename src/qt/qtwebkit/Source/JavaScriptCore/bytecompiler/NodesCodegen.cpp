/*
*  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
*  Copyright (C) 2001 Peter Kelly (pmk@post.com)
*  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012, 2013 Apple Inc. All rights reserved.
*  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
*  Copyright (C) 2007 Maks Orlovich
*  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2012 Igalia, S.L.
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
#include "JSNameScope.h"
#include "LabelScope.h"
#include "Lexer.h"
#include "Operations.h"
#include "Parser.h"
#include "PropertyNameArray.h"
#include "RegExpCache.h"
#include "RegExpObject.h"
#include "SamplingTool.h"
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

void ExpressionNode::emitBytecodeInConditionContext(BytecodeGenerator& generator, Label* trueTarget, Label* falseTarget, FallThroughMode fallThroughMode)
{
    RegisterID* result = generator.emitNode(this);
    if (fallThroughMode == FallThroughMeansTrue)
        generator.emitJumpIfFalse(result, falseTarget);
    else
        generator.emitJumpIfTrue(result, trueTarget);
}

// ------------------------------ ThrowableExpressionData --------------------------------

RegisterID* ThrowableExpressionData::emitThrowReferenceError(BytecodeGenerator& generator, const String& message)
{
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    generator.emitThrowReferenceError(message);
    return generator.newTemporary();
}

// ------------------------------ ConstantNode ----------------------------------

void ConstantNode::emitBytecodeInConditionContext(BytecodeGenerator& generator, Label* trueTarget, Label* falseTarget, FallThroughMode fallThroughMode)
{
    TriState value = jsValue(generator).pureToBoolean();
    if (value == MixedTriState)
        ExpressionNode::emitBytecodeInConditionContext(generator, trueTarget, falseTarget, fallThroughMode);
    else if (value == TrueTriState && fallThroughMode == FallThroughMeansFalse)
        generator.emitJump(trueTarget);
    else if (value == FalseTriState && fallThroughMode == FallThroughMeansTrue)
        generator.emitJump(falseTarget);

    // All other cases are unconditional fall-throughs, like "if (true)".
}

RegisterID* ConstantNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return 0;
    return generator.emitLoad(dst, jsValue(generator));
}

JSValue StringNode::jsValue(BytecodeGenerator& generator) const
{
    return generator.addStringConstant(m_value);
}

// ------------------------------ RegExpNode -----------------------------------

RegisterID* RegExpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return 0;
    return generator.emitNewRegExp(generator.finalDestination(dst), RegExp::create(*generator.vm(), m_pattern.string(), regExpFlags(m_flags.string())));
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
    return generator.resolve(m_ident).isStatic();
}

RegisterID* ResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ResolveResult resolveResult = generator.resolve(m_ident);
    if (RegisterID* local = resolveResult.local()) {
        if (dst == generator.ignoredResult())
            return 0;
        return generator.moveToDestinationIfNeeded(dst, local);
    }

    JSTextPosition divot = m_start + m_ident.length();
    generator.emitExpressionInfo(divot, m_start, divot);
    return generator.emitResolve(generator.finalDestination(dst), resolveResult, m_ident);
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
        return generator.emitNewArray(generator.finalDestination(dst), m_element, length);

    RefPtr<RegisterID> array = generator.emitNewArray(generator.tempDestination(dst), m_element, length);

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

ArgumentListNode* ArrayNode::toArgumentList(VM* vm, int lineNumber, int startPosition) const
{
    ASSERT(!m_elision && !m_optional);
    ElementNode* ptr = m_element;
    if (!ptr)
        return 0;
    JSTokenLocation location;
    location.line = lineNumber;
    location.startOffset = startPosition;
    ArgumentListNode* head = new (vm) ArgumentListNode(location, ptr->value());
    ArgumentListNode* tail = head;
    ptr = ptr->next();
    for (; ptr; ptr = ptr->next()) {
        ASSERT(!ptr->elision());
        tail = new (vm) ArgumentListNode(location, tail, ptr->value());
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
    
    // Fast case: this loop just handles regular value properties.
    PropertyListNode* p = this;
    for (; p && p->m_node->m_type == PropertyNode::Constant; p = p->m_next)
        generator.emitDirectPutById(newObj.get(), p->m_node->name(), generator.emitNode(p->m_node->m_assign));

    // Were there any get/set properties?
    if (p) {
        typedef std::pair<PropertyNode*, PropertyNode*> GetterSetterPair;
        typedef HashMap<StringImpl*, GetterSetterPair> GetterSetterMap;
        GetterSetterMap map;

        // Build a map, pairing get/set values together.
        for (PropertyListNode* q = p; q; q = q->m_next) {
            PropertyNode* node = q->m_node;
            if (node->m_type == PropertyNode::Constant)
                continue;

            GetterSetterPair pair(node, static_cast<PropertyNode*>(0));
            GetterSetterMap::AddResult result = map.add(node->name().impl(), pair);
            if (!result.isNewEntry)
                result.iterator->value.second = node;
        }

        // Iterate over the remaining properties in the list.
        for (; p; p = p->m_next) {
            PropertyNode* node = p->m_node;
            RegisterID* value = generator.emitNode(node->m_assign);

            // Handle regular values.
            if (node->m_type == PropertyNode::Constant) {
                generator.emitDirectPutById(newObj.get(), node->name(), value);
                continue;
            }

            // This is a get/set property, find its entry in the map.
            ASSERT(node->m_type == PropertyNode::Getter || node->m_type == PropertyNode::Setter);
            GetterSetterMap::iterator it = map.find(node->name().impl());
            ASSERT(it != map.end());
            GetterSetterPair& pair = it->value;

            // Was this already generated as a part of its partner?
            if (pair.second == node)
                continue;
    
            // Generate the paired node now.
            RefPtr<RegisterID> getterReg;
            RefPtr<RegisterID> setterReg;

            if (node->m_type == PropertyNode::Getter) {
                getterReg = value;
                if (pair.second) {
                    ASSERT(pair.second->m_type == PropertyNode::Setter);
                    setterReg = generator.emitNode(pair.second->m_assign);
                } else {
                    setterReg = generator.newTemporary();
                    generator.emitLoad(setterReg.get(), jsUndefined());
                }
            } else {
                ASSERT(node->m_type == PropertyNode::Setter);
                setterReg = value;
                if (pair.second) {
                    ASSERT(pair.second->m_type == PropertyNode::Getter);
                    getterReg = generator.emitNode(pair.second->m_assign);
                } else {
                    getterReg = generator.newTemporary();
                    generator.emitLoad(getterReg.get(), jsUndefined());
                }
            }

            generator.emitPutGetterSetter(newObj.get(), node->name(), getterReg.get(), setterReg.get());
        }
    }

    return generator.moveToDestinationIfNeeded(dst, newObj.get());
}

// ------------------------------ BracketAccessorNode --------------------------------

RegisterID* BracketAccessorNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_base->isResolveNode() 
        && generator.willResolveToArguments(static_cast<ResolveNode*>(m_base)->identifier())
        && !generator.symbolTable().slowArguments()) {
        RegisterID* property = generator.emitNode(m_subscript);
        generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
        return generator.emitGetArgumentByVal(generator.finalDestination(dst), generator.uncheckedRegisterForArguments(), property);
    }

    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_subscriptHasAssignments, m_subscript->isPure(generator));
    RegisterID* property = generator.emitNode(m_subscript);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
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
        generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
        return generator.emitGetArgumentsLength(generator.finalDestination(dst), generator.uncheckedRegisterForArguments());
    }

nonArgumentsPath:
    RegisterID* base = generator.emitNode(m_base);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
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
    ExpectedFunction expectedFunction;
    if (m_expr->isResolveNode())
        expectedFunction = generator.expectedFunctionForIdentifier(static_cast<ResolveNode*>(m_expr)->identifier());
    else
        expectedFunction = NoExpectedFunction;
    RefPtr<RegisterID> func = generator.emitNode(m_expr);
    CallArguments callArguments(generator, m_args);
    return generator.emitConstruct(generator.finalDestinationOrIgnored(dst), func.get(), expectedFunction, callArguments, divot(), divotStart(), divotEnd());
}

inline CallArguments::CallArguments(BytecodeGenerator& generator, ArgumentsNode* argumentsNode)
    : m_argumentsNode(argumentsNode)
{
    if (generator.shouldEmitProfileHooks())
        m_profileHookRegister = generator.newTemporary();

    size_t argumentCountIncludingThis = 1; // 'this' register.
    if (argumentsNode) {
        for (ArgumentListNode* node = argumentsNode->m_listNode; node; node = node->m_next)
            ++argumentCountIncludingThis;
    }

    m_argv.grow(argumentCountIncludingThis);
    for (int i = argumentCountIncludingThis - 1; i >= 0; --i) {
        m_argv[i] = generator.newTemporary();
        ASSERT(static_cast<size_t>(i) == m_argv.size() - 1 || m_argv[i]->index() == m_argv[i + 1]->index() + 1);
    }
}

inline void CallArguments::newArgument(BytecodeGenerator& generator)
{
    RefPtr<RegisterID> tmp = generator.newTemporary();
    ASSERT(m_argv.isEmpty() || tmp->index() == m_argv.last()->index() + 1); // Calling convention assumes that all arguments are contiguous.
    m_argv.append(tmp.release());
}

// ------------------------------ EvalFunctionCallNode ----------------------------------

RegisterID* EvalFunctionCallNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> func = generator.tempDestination(dst);
    CallArguments callArguments(generator, m_args);
    JSTextPosition newDivot = divotStart() + 4;
    generator.emitExpressionInfo(newDivot, divotStart(), newDivot);
    generator.emitResolveWithThis(callArguments.thisRegister(), func.get(), generator.resolve(generator.propertyNames().eval), generator.propertyNames().eval);
    return generator.emitCallEval(generator.finalDestination(dst, func.get()), func.get(), callArguments, divot(), divotStart(), divotEnd());
}

// ------------------------------ FunctionCallValueNode ----------------------------------

RegisterID* FunctionCallValueNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> func = generator.emitNode(m_expr);
    CallArguments callArguments(generator, m_args);
    generator.emitLoad(callArguments.thisRegister(), jsUndefined());
    return generator.emitCall(generator.finalDestinationOrIgnored(dst, func.get()), func.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
}

// ------------------------------ FunctionCallResolveNode ----------------------------------

RegisterID* FunctionCallResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ExpectedFunction expectedFunction = generator.expectedFunctionForIdentifier(m_ident);
    ResolveResult resolveResult = generator.resolve(m_ident);

    if (RegisterID* local = resolveResult.local()) {
        RefPtr<RegisterID> func = generator.emitMove(generator.tempDestination(dst), local);
        CallArguments callArguments(generator, m_args);
        generator.emitLoad(callArguments.thisRegister(), jsUndefined());
        // This passes NoExpectedFunction because we expect that if the function is in a
        // local variable, then it's not one of our built-in constructors.
        return generator.emitCall(generator.finalDestinationOrIgnored(dst, callArguments.thisRegister()), func.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
    }

    if (resolveResult.isStatic()) {
        RefPtr<RegisterID> func = generator.newTemporary();
        CallArguments callArguments(generator, m_args);
        generator.emitGetStaticVar(func.get(), resolveResult, m_ident);
        generator.emitLoad(callArguments.thisRegister(), jsUndefined());
        return generator.emitCall(generator.finalDestinationOrIgnored(dst, func.get()), func.get(), expectedFunction, callArguments, divot(), divotStart(), divotEnd());
    }

    RefPtr<RegisterID> func = generator.newTemporary();
    CallArguments callArguments(generator, m_args);

    JSTextPosition newDivot = divotStart() + m_ident.length();
    generator.emitExpressionInfo(newDivot, divotStart(), newDivot);
    generator.emitResolveWithThis(callArguments.thisRegister(), func.get(), resolveResult, m_ident);
    return generator.emitCall(generator.finalDestinationOrIgnored(dst, func.get()), func.get(), expectedFunction, callArguments, divot(), divotStart(), divotEnd());
}

// ------------------------------ FunctionCallBracketNode ----------------------------------

RegisterID* FunctionCallBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNode(m_base);
    RegisterID* property = generator.emitNode(m_subscript);
    generator.emitExpressionInfo(subexpressionDivot(), subexpressionStart(), subexpressionEnd());
    RefPtr<RegisterID> function = generator.emitGetByVal(generator.tempDestination(dst), base.get(), property);
    CallArguments callArguments(generator, m_args);
    generator.emitMove(callArguments.thisRegister(), base.get());
    return generator.emitCall(generator.finalDestinationOrIgnored(dst, function.get()), function.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
}

// ------------------------------ FunctionCallDotNode ----------------------------------

RegisterID* FunctionCallDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> function = generator.tempDestination(dst);
    CallArguments callArguments(generator, m_args);
    generator.emitNode(callArguments.thisRegister(), m_base);
    generator.emitExpressionInfo(subexpressionDivot(), subexpressionStart(), subexpressionEnd());
    generator.emitGetById(function.get(), callArguments.thisRegister(), m_ident);
    return generator.emitCall(generator.finalDestinationOrIgnored(dst, function.get()), function.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
}

RegisterID* CallFunctionCallDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<Label> realCall = generator.newLabel();
    RefPtr<Label> end = generator.newLabel();
    RefPtr<RegisterID> base = generator.emitNode(m_base);
    generator.emitExpressionInfo(subexpressionDivot(), subexpressionStart(), subexpressionEnd());
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
            generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
            generator.emitJump(end.get());

            m_args->m_listNode = oldList;
        } else {
            RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
            CallArguments callArguments(generator, m_args);
            generator.emitLoad(callArguments.thisRegister(), jsUndefined());
            generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
            generator.emitJump(end.get());
        }
    }
    generator.emitLabel(realCall.get());
    {
        CallArguments callArguments(generator, m_args);
        generator.emitMove(callArguments.thisRegister(), base.get());
        generator.emitCall(finalDestinationOrIgnored.get(), function.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
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
    generator.emitExpressionInfo(subexpressionDivot(), subexpressionStart(), subexpressionEnd());
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
                    m_args->m_listNode = static_cast<ArrayNode*>(m_args->m_listNode->m_next->m_expr)->toArgumentList(generator.vm(), 0, 0);
                    RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
                    CallArguments callArguments(generator, m_args);
                    generator.emitNode(callArguments.thisRegister(), oldList->m_expr);
                    generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
                } else {
                    m_args->m_listNode = m_args->m_listNode->m_next;
                    RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
                    CallArguments callArguments(generator, m_args);
                    generator.emitNode(callArguments.thisRegister(), oldList->m_expr);
                    generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
                }
                m_args->m_listNode = oldList;
            } else {
                RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
                CallArguments callArguments(generator, m_args);
                generator.emitLoad(callArguments.thisRegister(), jsUndefined());
                generator.emitCall(finalDestinationOrIgnored.get(), realFunction.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
            }
        } else {
            ASSERT(m_args->m_listNode && m_args->m_listNode->m_next);
            RefPtr<RegisterID> profileHookRegister;
            if (generator.shouldEmitProfileHooks())
                profileHookRegister = generator.newTemporary();
            RefPtr<RegisterID> realFunction = generator.emitMove(generator.tempDestination(dst), base.get());
            RefPtr<RegisterID> thisRegister = generator.emitNode(m_args->m_listNode->m_expr);
            RefPtr<RegisterID> argsRegister;
            ArgumentListNode* args = m_args->m_listNode->m_next;
            if (args->m_expr->isResolveNode() && generator.willResolveToArguments(static_cast<ResolveNode*>(args->m_expr)->identifier()))
                argsRegister = generator.uncheckedRegisterForArguments();
            else
                argsRegister = generator.emitNode(args->m_expr);

            // Function.prototype.apply ignores extra arguments, but we still
            // need to evaluate them for side effects.
            while ((args = args->m_next))
                generator.emitNode(args->m_expr);

            generator.emitCallVarargs(finalDestinationOrIgnored.get(), realFunction.get(), thisRegister.get(), argsRegister.get(), generator.newTemporary(), profileHookRegister.get(), divot(), divotStart(), divotEnd());
        }
        generator.emitJump(end.get());
    }
    generator.emitLabel(realCall.get());
    {
        CallArguments callArguments(generator, m_args);
        generator.emitMove(callArguments.thisRegister(), base.get());
        generator.emitCall(finalDestinationOrIgnored.get(), function.get(), NoExpectedFunction, callArguments, divot(), divotStart(), divotEnd());
    }
    generator.emitLabel(end.get());
    return finalDestinationOrIgnored.get();
}

// ------------------------------ PostfixNode ----------------------------------

static RegisterID* emitIncOrDec(BytecodeGenerator& generator, RegisterID* srcDst, Operator oper)
{
    return (oper == OpPlusPlus) ? generator.emitInc(srcDst) : generator.emitDec(srcDst);
}

static RegisterID* emitPostIncOrDec(BytecodeGenerator& generator, RegisterID* dst, RegisterID* srcDst, Operator oper)
{
    if (dst == srcDst)
        return generator.emitToNumber(generator.finalDestination(dst), srcDst);
    RefPtr<RegisterID> tmp = generator.emitToNumber(generator.tempDestination(dst), srcDst);
    emitIncOrDec(generator, srcDst, oper);
    return generator.moveToDestinationIfNeeded(dst, tmp.get());
}

RegisterID* PostfixNode::emitResolve(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return PrefixNode::emitResolve(generator, dst);

    ASSERT(m_expr->isResolveNode());
    ResolveNode* resolve = static_cast<ResolveNode*>(m_expr);
    const Identifier& ident = resolve->identifier();

    ResolveResult resolveResult = generator.resolve(ident);

    if (RefPtr<RegisterID> local = resolveResult.local()) {
        if (resolveResult.isReadOnly()) {
            generator.emitReadOnlyExceptionIfNeeded();
            local = generator.emitMove(generator.tempDestination(dst), local.get());
        }
        return emitPostIncOrDec(generator, generator.finalDestination(dst), local.get(), m_operator);
    }

    if (resolveResult.isStatic() && !resolveResult.isReadOnly()) {
        RefPtr<RegisterID> value = generator.emitGetStaticVar(generator.newTemporary(), resolveResult, ident);
        RefPtr<RegisterID> oldValue = emitPostIncOrDec(generator, generator.finalDestination(dst), value.get(), m_operator);
        generator.emitPutStaticVar(resolveResult, ident, value.get());
        return oldValue.get();
    }

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    RefPtr<RegisterID> value = generator.newTemporary();
    NonlocalResolveInfo resolveInfo;
    RefPtr<RegisterID> base = generator.emitResolveWithBaseForPut(generator.newTemporary(), value.get(), resolveResult, ident, resolveInfo);
    RefPtr<RegisterID> oldValue = emitPostIncOrDec(generator, generator.finalDestination(dst), value.get(), m_operator);
    generator.emitPutToBase(base.get(), ident, value.get(), resolveInfo);
    return oldValue.get();
}

RegisterID* PostfixNode::emitBracket(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return PrefixNode::emitBracket(generator, dst);

    ASSERT(m_expr->isBracketAccessorNode());
    BracketAccessorNode* bracketAccessor = static_cast<BracketAccessorNode*>(m_expr);
    ExpressionNode* baseNode = bracketAccessor->base();
    ExpressionNode* subscript = bracketAccessor->subscript();

    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(baseNode, bracketAccessor->subscriptHasAssignments(), subscript->isPure(generator));
    RefPtr<RegisterID> property = generator.emitNode(subscript);

    generator.emitExpressionInfo(bracketAccessor->divot(), bracketAccessor->divotStart(), bracketAccessor->divotEnd());
    RefPtr<RegisterID> value = generator.emitGetByVal(generator.newTemporary(), base.get(), property.get());
    RegisterID* oldValue = emitPostIncOrDec(generator, generator.tempDestination(dst), value.get(), m_operator);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    generator.emitPutByVal(base.get(), property.get(), value.get());
    return generator.moveToDestinationIfNeeded(dst, oldValue);
}

RegisterID* PostfixNode::emitDot(BytecodeGenerator& generator, RegisterID* dst)
{
    if (dst == generator.ignoredResult())
        return PrefixNode::emitDot(generator, dst);

    ASSERT(m_expr->isDotAccessorNode());
    DotAccessorNode* dotAccessor = static_cast<DotAccessorNode*>(m_expr);
    ExpressionNode* baseNode = dotAccessor->base();
    const Identifier& ident = dotAccessor->identifier();

    RefPtr<RegisterID> base = generator.emitNode(baseNode);

    generator.emitExpressionInfo(dotAccessor->divot(), dotAccessor->divotStart(), dotAccessor->divotEnd());
    RefPtr<RegisterID> value = generator.emitGetById(generator.newTemporary(), base.get(), ident);
    RegisterID* oldValue = emitPostIncOrDec(generator, generator.tempDestination(dst), value.get(), m_operator);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    generator.emitPutById(base.get(), ident, value.get());
    return generator.moveToDestinationIfNeeded(dst, oldValue);
}

RegisterID* PostfixNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_expr->isResolveNode())
        return emitResolve(generator, dst);

    if (m_expr->isBracketAccessorNode())
        return emitBracket(generator, dst);

    if (m_expr->isDotAccessorNode())
        return emitDot(generator, dst);

    return emitThrowReferenceError(generator, m_operator == OpPlusPlus
        ? "Postfix ++ operator applied to value that is not a reference."
        : "Postfix -- operator applied to value that is not a reference.");
}

// ------------------------------ DeleteResolveNode -----------------------------------

RegisterID* DeleteResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ResolveResult resolveResult = generator.resolve(m_ident);
    if (resolveResult.isRegister())
        return generator.emitLoad(generator.finalDestination(dst), false);

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    RegisterID* base = generator.emitResolveBase(generator.tempDestination(dst), resolveResult, m_ident);
    return generator.emitDeleteById(generator.finalDestination(dst, base), base, m_ident);
}

// ------------------------------ DeleteBracketNode -----------------------------------

RegisterID* DeleteBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> r0 = generator.emitNode(m_base);
    RegisterID* r1 = generator.emitNode(m_subscript);

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    return generator.emitDeleteByVal(generator.finalDestination(dst), r0.get(), r1);
}

// ------------------------------ DeleteDotNode -----------------------------------

RegisterID* DeleteDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RegisterID* r0 = generator.emitNode(m_base);

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
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
    ResolveResult resolveResult = generator.resolve(m_ident);
    if (RegisterID* local = resolveResult.local()) {
        if (dst == generator.ignoredResult())
            return 0;
        return generator.emitTypeOf(generator.finalDestination(dst), local);
    }

    if (resolveResult.isStatic()) {
        RefPtr<RegisterID> scratch = generator.emitGetStaticVar(generator.tempDestination(dst), resolveResult, m_ident);
        return generator.emitTypeOf(generator.finalDestination(dst, scratch.get()), scratch.get());
    }

    RefPtr<RegisterID> scratch = generator.emitResolveBase(generator.tempDestination(dst), resolveResult, m_ident);
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

// ------------------------------ PrefixNode ----------------------------------

RegisterID* PrefixNode::emitResolve(BytecodeGenerator& generator, RegisterID* dst)
{
    ASSERT(m_expr->isResolveNode());
    ResolveNode* resolve = static_cast<ResolveNode*>(m_expr);
    const Identifier& ident = resolve->identifier();

    ResolveResult resolveResult = generator.resolve(ident);
    if (RefPtr<RegisterID> local = resolveResult.local()) {
        if (resolveResult.isReadOnly()) {
            generator.emitReadOnlyExceptionIfNeeded();
            local = generator.emitMove(generator.tempDestination(dst), local.get());
        }
        emitIncOrDec(generator, local.get(), m_operator);
        return generator.moveToDestinationIfNeeded(dst, local.get());
    }

    if (resolveResult.isStatic() && !resolveResult.isReadOnly()) {
        RefPtr<RegisterID> propDst = generator.emitGetStaticVar(generator.tempDestination(dst), resolveResult, ident);
        emitIncOrDec(generator, propDst.get(), m_operator);
        generator.emitPutStaticVar(resolveResult, ident, propDst.get());
        return generator.moveToDestinationIfNeeded(dst, propDst.get());
    }

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    RefPtr<RegisterID> propDst = generator.tempDestination(dst);
    NonlocalResolveInfo resolveVerifier;
    RefPtr<RegisterID> base = generator.emitResolveWithBaseForPut(generator.newTemporary(), propDst.get(), resolveResult, ident, resolveVerifier);
    emitIncOrDec(generator, propDst.get(), m_operator);
    generator.emitPutToBase(base.get(), ident, propDst.get(), resolveVerifier);
    return generator.moveToDestinationIfNeeded(dst, propDst.get());
}

RegisterID* PrefixNode::emitBracket(BytecodeGenerator& generator, RegisterID* dst)
{
    ASSERT(m_expr->isBracketAccessorNode());
    BracketAccessorNode* bracketAccessor = static_cast<BracketAccessorNode*>(m_expr);
    ExpressionNode* baseNode = bracketAccessor->base();
    ExpressionNode* subscript = bracketAccessor->subscript();

    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(baseNode, bracketAccessor->subscriptHasAssignments(), subscript->isPure(generator));
    RefPtr<RegisterID> property = generator.emitNode(subscript);
    RefPtr<RegisterID> propDst = generator.tempDestination(dst);

    generator.emitExpressionInfo(bracketAccessor->divot(), bracketAccessor->divotStart(), bracketAccessor->divotEnd());
    RegisterID* value = generator.emitGetByVal(propDst.get(), base.get(), property.get());
    emitIncOrDec(generator, value, m_operator);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    generator.emitPutByVal(base.get(), property.get(), value);
    return generator.moveToDestinationIfNeeded(dst, propDst.get());
}

RegisterID* PrefixNode::emitDot(BytecodeGenerator& generator, RegisterID* dst)
{
    ASSERT(m_expr->isDotAccessorNode());
    DotAccessorNode* dotAccessor = static_cast<DotAccessorNode*>(m_expr);
    ExpressionNode* baseNode = dotAccessor->base();
    const Identifier& ident = dotAccessor->identifier();

    RefPtr<RegisterID> base = generator.emitNode(baseNode);
    RefPtr<RegisterID> propDst = generator.tempDestination(dst);

    generator.emitExpressionInfo(dotAccessor->divot(), dotAccessor->divotStart(), dotAccessor->divotEnd());
    RegisterID* value = generator.emitGetById(propDst.get(), base.get(), ident);
    emitIncOrDec(generator, value, m_operator);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    generator.emitPutById(base.get(), ident, value);
    return generator.moveToDestinationIfNeeded(dst, propDst.get());
}

RegisterID* PrefixNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_expr->isResolveNode())
        return emitResolve(generator, dst);

    if (m_expr->isBracketAccessorNode())
        return emitBracket(generator, dst);

    if (m_expr->isDotAccessorNode())
        return emitDot(generator, dst);

    return emitThrowReferenceError(generator, m_operator == OpPlusPlus
        ? "Prefix ++ operator applied to value that is not a reference."
        : "Prefix -- operator applied to value that is not a reference.");
}

// ------------------------------ Unary Operation Nodes -----------------------------------

RegisterID* UnaryOpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RegisterID* src = generator.emitNode(m_expr);
    generator.emitExpressionInfo(position(), position(), position());
    return generator.emitUnaryOp(opcodeID(), generator.finalDestination(dst), src);
}

// ------------------------------ BitwiseNotNode -----------------------------------
 
RegisterID* BitwiseNotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> src2 = generator.emitLoad(generator.newTemporary(), jsNumber(-1));
    RegisterID* src1 = generator.emitNode(m_expr);
    return generator.emitBinaryOp(op_bitxor, generator.finalDestination(dst, src1), src1, src2.get(), OperandTypes(m_expr->resultDescriptor(), ResultType::numberTypeIsInt32()));
}
 
// ------------------------------ LogicalNotNode -----------------------------------

void LogicalNotNode::emitBytecodeInConditionContext(BytecodeGenerator& generator, Label* trueTarget, Label* falseTarget, FallThroughMode fallThroughMode)
{
    // reverse the true and false targets
    generator.emitNodeInConditionContext(expr(), falseTarget, trueTarget, invert(fallThroughMode));
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
        generator.emitExpressionInfo(emitExpressionInfoForMe->divot(), emitExpressionInfoForMe->divotStart(), emitExpressionInfoForMe->divotEnd());
    // If there is an assignment convert the lhs now.  This will also copy lhs to
    // the temporary register we allocated for it.
    if (lhs)
        generator.emitToPrimitive(temporaryRegisters[0].get(), lhs);

    return generator.emitStrcat(generator.finalDestination(dst, temporaryRegisters[0].get()), temporaryRegisters[0].get(), temporaryRegisters.size());
}

void BinaryOpNode::emitBytecodeInConditionContext(BytecodeGenerator& generator, Label* trueTarget, Label* falseTarget, FallThroughMode fallThroughMode)
{
    TriState branchCondition;
    ExpressionNode* branchExpression;
    tryFoldToBranch(generator, branchCondition, branchExpression);

    if (branchCondition == MixedTriState)
        ExpressionNode::emitBytecodeInConditionContext(generator, trueTarget, falseTarget, fallThroughMode);
    else if (branchCondition == TrueTriState)
        generator.emitNodeInConditionContext(branchExpression, trueTarget, falseTarget, fallThroughMode);
    else
        generator.emitNodeInConditionContext(branchExpression, falseTarget, trueTarget, invert(fallThroughMode));
}

static inline bool canFoldToBranch(OpcodeID opcodeID, ExpressionNode* branchExpression, JSValue constant)
{
    ResultType expressionType = branchExpression->resultDescriptor();

    if (expressionType.definitelyIsBoolean() && constant.isBoolean())
        return true;
    else if (expressionType.definitelyIsBoolean() && constant.isInt32() && (constant.asInt32() == 0 || constant.asInt32() == 1))
        return opcodeID == op_eq || opcodeID == op_neq; // Strict equality is false in the case of type mismatch.
    else if (expressionType.isInt32() && constant.isInt32() && constant.asInt32() == 0)
        return true;

    return false;
}

void BinaryOpNode::tryFoldToBranch(BytecodeGenerator& generator, TriState& branchCondition, ExpressionNode*& branchExpression)
{
    branchCondition = MixedTriState;
    branchExpression = 0;

    ConstantNode* constant = 0;
    if (m_expr1->isConstant()) {
        constant = static_cast<ConstantNode*>(m_expr1);
        branchExpression = m_expr2;
    } else if (m_expr2->isConstant()) {
        constant = static_cast<ConstantNode*>(m_expr2);
        branchExpression = m_expr1;
    }

    if (!constant)
        return;
    ASSERT(branchExpression);

    OpcodeID opcodeID = this->opcodeID();
    JSValue value = constant->jsValue(generator);
    bool canFoldToBranch = JSC::canFoldToBranch(opcodeID, branchExpression, value);
    if (!canFoldToBranch)
        return;

    if (opcodeID == op_eq || opcodeID == op_stricteq)
        branchCondition = triState(value.pureToBoolean());
    else if (opcodeID == op_neq || opcodeID == op_nstricteq)
        branchCondition = triState(!value.pureToBoolean());
}

RegisterID* BinaryOpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    OpcodeID opcodeID = this->opcodeID();

    if (opcodeID == op_add && m_expr1->isAdd() && m_expr1->resultDescriptor().definitelyIsString()) {
        generator.emitExpressionInfo(position(), position(), position());
        return emitStrcat(generator, dst);
    }

    if (opcodeID == op_neq) {
        if (m_expr1->isNull() || m_expr2->isNull()) {
            RefPtr<RegisterID> src = generator.tempDestination(dst);
            generator.emitNode(src.get(), m_expr1->isNull() ? m_expr2 : m_expr1);
            return generator.emitUnaryOp(op_neq_null, generator.finalDestination(dst, src.get()), src.get());
        }
    }

    ExpressionNode* left = m_expr1;
    ExpressionNode* right = m_expr2;
    if (opcodeID == op_neq || opcodeID == op_nstricteq) {
        if (left->isString())
            std::swap(left, right);
    }

    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(left, m_rightHasAssignments, right->isPure(generator));
    bool wasTypeof = generator.m_lastOpcodeID == op_typeof;
    RegisterID* src2 = generator.emitNode(right);
    generator.emitExpressionInfo(position(), position(), position());
    if (wasTypeof && (opcodeID == op_neq || opcodeID == op_nstricteq)) {
        RefPtr<RegisterID> tmp = generator.tempDestination(dst);
        if (opcodeID == op_neq)
            generator.emitEqualityOp(op_eq, generator.finalDestination(tmp.get(), src1.get()), src1.get(), src2);
        else if (opcodeID == op_nstricteq)
            generator.emitEqualityOp(op_stricteq, generator.finalDestination(tmp.get(), src1.get()), src1.get(), src2);
        else
            RELEASE_ASSERT_NOT_REACHED();
        return generator.emitUnaryOp(op_not, generator.finalDestination(dst, tmp.get()), tmp.get());
    }
    return generator.emitBinaryOp(opcodeID, generator.finalDestination(dst, src1.get()), src1.get(), src2, OperandTypes(left->resultDescriptor(), right->resultDescriptor()));
}

RegisterID* EqualNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (m_expr1->isNull() || m_expr2->isNull()) {
        RefPtr<RegisterID> src = generator.tempDestination(dst);
        generator.emitNode(src.get(), m_expr1->isNull() ? m_expr2 : m_expr1);
        return generator.emitUnaryOp(op_eq_null, generator.finalDestination(dst, src.get()), src.get());
    }

    ExpressionNode* left = m_expr1;
    ExpressionNode* right = m_expr2;
    if (left->isString())
        std::swap(left, right);

    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(left, m_rightHasAssignments, m_expr2->isPure(generator));
    RegisterID* src2 = generator.emitNode(right);
    return generator.emitEqualityOp(op_eq, generator.finalDestination(dst, src1.get()), src1.get(), src2);
}

RegisterID* StrictEqualNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ExpressionNode* left = m_expr1;
    ExpressionNode* right = m_expr2;
    if (left->isString())
        std::swap(left, right);

    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(left, m_rightHasAssignments, m_expr2->isPure(generator));
    RegisterID* src2 = generator.emitNode(right);
    return generator.emitEqualityOp(op_stricteq, generator.finalDestination(dst, src1.get()), src1.get(), src2);
}

RegisterID* ThrowableBinaryOpNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(m_expr1, m_rightHasAssignments, m_expr2->isPure(generator));
    RegisterID* src2 = generator.emitNode(m_expr2);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    return generator.emitBinaryOp(opcodeID(), generator.finalDestination(dst, src1.get()), src1.get(), src2, OperandTypes(m_expr1->resultDescriptor(), m_expr2->resultDescriptor()));
}

RegisterID* InstanceOfNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> src1 = generator.emitNodeForLeftHandSide(m_expr1, m_rightHasAssignments, m_expr2->isPure(generator));
    RefPtr<RegisterID> src2 = generator.emitNode(m_expr2);
    RefPtr<RegisterID> prototype = generator.newTemporary();
    RefPtr<RegisterID> dstReg = generator.finalDestination(dst, src1.get());
    RefPtr<Label> target = generator.newLabel();

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    generator.emitCheckHasInstance(dstReg.get(), src1.get(), src2.get(), target.get());

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    generator.emitGetById(prototype.get(), src2.get(), generator.vm()->propertyNames->prototype);

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    RegisterID* result = generator.emitInstanceOf(dstReg.get(), src1.get(), prototype.get());
    generator.emitLabel(target.get());
    return result;
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

void LogicalOpNode::emitBytecodeInConditionContext(BytecodeGenerator& generator, Label* trueTarget, Label* falseTarget, FallThroughMode fallThroughMode)
{
    RefPtr<Label> afterExpr1 = generator.newLabel();
    if (m_operator == OpLogicalAnd)
        generator.emitNodeInConditionContext(m_expr1, afterExpr1.get(), falseTarget, FallThroughMeansTrue);
    else 
        generator.emitNodeInConditionContext(m_expr1, trueTarget, afterExpr1.get(), FallThroughMeansFalse);
    generator.emitLabel(afterExpr1.get());

    generator.emitNodeInConditionContext(m_expr2, trueTarget, falseTarget, fallThroughMode);
}

// ------------------------------ ConditionalNode ------------------------------

RegisterID* ConditionalNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> newDst = generator.finalDestination(dst);
    RefPtr<Label> beforeElse = generator.newLabel();
    RefPtr<Label> afterElse = generator.newLabel();

    RefPtr<Label> beforeThen = generator.newLabel();
    generator.emitNodeInConditionContext(m_logical, beforeThen.get(), beforeElse.get(), FallThroughMeansTrue);
    generator.emitLabel(beforeThen.get());

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
            RELEASE_ASSERT_NOT_REACHED();
            return dst;
    }

    RegisterID* src2 = generator.emitNode(m_right);

    // Certain read-modify nodes require expression info to be emitted *after* m_right has been generated.
    // If this is required the node is passed as 'emitExpressionInfoForMe'; do so now.
    if (emitExpressionInfoForMe)
        generator.emitExpressionInfo(emitExpressionInfoForMe->divot(), emitExpressionInfoForMe->divotStart(), emitExpressionInfoForMe->divotEnd());
    return generator.emitBinaryOp(opcodeID, dst, src1, src2, types);
}

RegisterID* ReadModifyResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ResolveResult resolveResult = generator.resolve(m_ident);

    if (RegisterID *local = resolveResult.local()) {
        if (resolveResult.isReadOnly()) {
            generator.emitReadOnlyExceptionIfNeeded();
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

    if (resolveResult.isStatic() && !resolveResult.isReadOnly()) {
        RefPtr<RegisterID> src1 = generator.emitGetStaticVar(generator.tempDestination(dst), resolveResult, m_ident);
        RegisterID* result = emitReadModifyAssignment(generator, generator.finalDestination(dst, src1.get()), src1.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));
        generator.emitPutStaticVar(resolveResult, m_ident, result);
        return result;
    }

    RefPtr<RegisterID> src1 = generator.tempDestination(dst);
    JSTextPosition newDivot = divotStart() + m_ident.length();
    generator.emitExpressionInfo(newDivot, divotStart(), newDivot);
    NonlocalResolveInfo resolveVerifier;
    RefPtr<RegisterID> base = generator.emitResolveWithBaseForPut(generator.newTemporary(), src1.get(), resolveResult, m_ident, resolveVerifier);
    RegisterID* result = emitReadModifyAssignment(generator, generator.finalDestination(dst, src1.get()), src1.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()), this);
    return generator.emitPutToBase(base.get(), m_ident, result, resolveVerifier);
}

// ------------------------------ AssignResolveNode -----------------------------------

RegisterID* AssignResolveNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ResolveResult resolveResult = generator.resolve(m_ident);

    if (RegisterID* local = resolveResult.local()) {
        if (resolveResult.isReadOnly()) {
            generator.emitReadOnlyExceptionIfNeeded();
            return generator.emitNode(dst, m_right);
        }
        RegisterID* result = generator.emitNode(local, m_right);
        return generator.moveToDestinationIfNeeded(dst, result);
    }

    if (resolveResult.isStatic() && !resolveResult.isReadOnly()) {
        if (dst == generator.ignoredResult())
            dst = 0;
        RegisterID* value = generator.emitNode(dst, m_right);
        generator.emitPutStaticVar(resolveResult, m_ident, value);
        return value;
    }

    NonlocalResolveInfo resolveVerifier;
    if (generator.isStrictMode())
        generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    RefPtr<RegisterID> base = generator.emitResolveBaseForPut(generator.newTemporary(), resolveResult, m_ident, resolveVerifier);
    if (dst == generator.ignoredResult())
        dst = 0;
    RegisterID* value = generator.emitNode(dst, m_right);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    return generator.emitPutToBase(base.get(), m_ident, value, resolveVerifier);
}

// ------------------------------ AssignDotNode -----------------------------------

RegisterID* AssignDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_rightHasAssignments, m_right->isPure(generator));
    RefPtr<RegisterID> value = generator.destinationForAssignResult(dst);
    RegisterID* result = generator.emitNode(value.get(), m_right);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    RegisterID* forwardResult = (dst == generator.ignoredResult()) ? result : generator.moveToDestinationIfNeeded(generator.tempDestination(result), result);
    generator.emitPutById(base.get(), m_ident, forwardResult);
    return generator.moveToDestinationIfNeeded(dst, forwardResult);
}

// ------------------------------ ReadModifyDotNode -----------------------------------

RegisterID* ReadModifyDotNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_rightHasAssignments, m_right->isPure(generator));

    generator.emitExpressionInfo(subexpressionDivot(), subexpressionStart(), subexpressionEnd());
    RefPtr<RegisterID> value = generator.emitGetById(generator.tempDestination(dst), base.get(), m_ident);
    RegisterID* updatedValue = emitReadModifyAssignment(generator, generator.finalDestination(dst, value.get()), value.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
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

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    RegisterID* forwardResult = (dst == generator.ignoredResult()) ? result : generator.moveToDestinationIfNeeded(generator.tempDestination(result), result);
    generator.emitPutByVal(base.get(), property.get(), forwardResult);
    return generator.moveToDestinationIfNeeded(dst, forwardResult);
}

// ------------------------------ ReadModifyBracketNode -----------------------------------

RegisterID* ReadModifyBracketNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    RefPtr<RegisterID> base = generator.emitNodeForLeftHandSide(m_base, m_subscriptHasAssignments || m_rightHasAssignments, m_subscript->isPure(generator) && m_right->isPure(generator));
    RefPtr<RegisterID> property = generator.emitNodeForLeftHandSide(m_subscript, m_rightHasAssignments, m_right->isPure(generator));

    generator.emitExpressionInfo(subexpressionDivot(), subexpressionStart(), subexpressionEnd());
    RefPtr<RegisterID> value = generator.emitGetByVal(generator.tempDestination(dst), base.get(), property.get());
    RegisterID* updatedValue = emitReadModifyAssignment(generator, generator.finalDestination(dst, value.get()), value.get(), m_right, m_operator, OperandTypes(ResultType::unknownType(), m_right->resultDescriptor()));

    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
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
    ResolveResult resolveResult = generator.resolveConstDecl(m_ident);

    // FIXME: This code does not match the behavior of const in Firefox.
    if (RegisterID* local = resolveResult.local()) {
        if (!m_init)
            return local;

        return generator.emitNode(local, m_init);
    }

    RefPtr<RegisterID> value = m_init ? generator.emitNode(m_init) : generator.emitLoad(0, jsUndefined());

    if (generator.codeType() == GlobalCode)
        return generator.emitInitGlobalConst(m_ident, value.get());

    if (generator.codeType() != EvalCode)
        return value.get();

    // FIXME: This will result in incorrect assignment if m_ident exists in an intervening with scope.
    RefPtr<RegisterID> base = generator.emitResolveBase(generator.newTemporary(), resolveResult, m_ident);
    return generator.emitPutById(base.get(), m_ident, value.get());
}

RegisterID* ConstDeclNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    RegisterID* result = 0;
    for (ConstDeclNode* n = this; n; n = n->m_next)
        result = n->emitCodeSingle(generator);

    return result;
}

// ------------------------------ ConstStatementNode -----------------------------

void ConstStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    generator.emitNode(m_next);
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

void BlockNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (!m_statements)
        return;
    m_statements->emitBytecode(generator, dst);
}

// ------------------------------ EmptyStatementNode ---------------------------

void EmptyStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
}

// ------------------------------ DebuggerStatementNode ---------------------------

void DebuggerStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(DidReachBreakpoint, firstLine(), lastLine(), startOffset(), lineStartOffset());
}

// ------------------------------ ExprStatementNode ----------------------------

void ExprStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    ASSERT(m_expr);
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    generator.emitNode(dst, m_expr);
}

// ------------------------------ VarStatementNode ----------------------------

void VarStatementNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    ASSERT(m_expr);
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    generator.emitNode(m_expr);
}

// ------------------------------ IfElseNode ---------------------------------------

static inline StatementNode* singleStatement(StatementNode* statementNode)
{
    if (statementNode->isBlock())
        return static_cast<BlockNode*>(statementNode)->singleStatement();
    return statementNode;
}

bool IfElseNode::tryFoldBreakAndContinue(BytecodeGenerator& generator, StatementNode* ifBlock,
    Label*& trueTarget, FallThroughMode& fallThroughMode)
{
    StatementNode* singleStatement = JSC::singleStatement(ifBlock);
    if (!singleStatement)
        return false;

    if (singleStatement->isBreak()) {
        BreakNode* breakNode = static_cast<BreakNode*>(singleStatement);
        Label* target = breakNode->trivialTarget(generator);
        if (!target)
            return false;
        trueTarget = target;
        fallThroughMode = FallThroughMeansFalse;
        return true;
    }

    if (singleStatement->isContinue()) {
        ContinueNode* continueNode = static_cast<ContinueNode*>(singleStatement);
        Label* target = continueNode->trivialTarget(generator);
        if (!target)
            return false;
        trueTarget = target;
        fallThroughMode = FallThroughMeansFalse;
        return true;
    }

    return false;
}

void IfElseNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    
    RefPtr<Label> beforeThen = generator.newLabel();
    RefPtr<Label> beforeElse = generator.newLabel();
    RefPtr<Label> afterElse = generator.newLabel();

    Label* trueTarget = beforeThen.get();
    Label* falseTarget = beforeElse.get();
    FallThroughMode fallThroughMode = FallThroughMeansTrue;
    bool didFoldIfBlock = tryFoldBreakAndContinue(generator, m_ifBlock, trueTarget, fallThroughMode);

    generator.emitNodeInConditionContext(m_condition, trueTarget, falseTarget, fallThroughMode);
    generator.emitLabel(beforeThen.get());

    if (!didFoldIfBlock) {
        generator.emitNode(dst, m_ifBlock);
        if (m_elseBlock)
            generator.emitJump(afterElse.get());
    }

    generator.emitLabel(beforeElse.get());

    if (m_elseBlock)
        generator.emitNode(dst, m_elseBlock);

    generator.emitLabel(afterElse.get());
}

// ------------------------------ DoWhileNode ----------------------------------

void DoWhileNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    LabelScopePtr scope = generator.newLabelScope(LabelScope::Loop);

    RefPtr<Label> topOfLoop = generator.newLabel();
    generator.emitLabel(topOfLoop.get());
    generator.emitLoopHint();
    generator.emitDebugHook(WillExecuteStatement, lastLine(), lastLine(), startOffset(), lineStartOffset());

    generator.emitNode(dst, m_statement);

    generator.emitLabel(scope->continueTarget());
    generator.emitDebugHook(WillExecuteStatement, lastLine(), lastLine(), startOffset(), lineStartOffset());
    generator.emitNodeInConditionContext(m_expr, topOfLoop.get(), scope->breakTarget(), FallThroughMeansFalse);

    generator.emitLabel(scope->breakTarget());
}

// ------------------------------ WhileNode ------------------------------------

void WhileNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    LabelScopePtr scope = generator.newLabelScope(LabelScope::Loop);
    RefPtr<Label> topOfLoop = generator.newLabel();

    generator.emitDebugHook(WillExecuteStatement, m_expr->lineNo(), m_expr->lineNo(), m_expr->startOffset(), m_expr->lineStartOffset());
    generator.emitNodeInConditionContext(m_expr, topOfLoop.get(), scope->breakTarget(), FallThroughMeansTrue);

    generator.emitLabel(topOfLoop.get());
    generator.emitLoopHint();
    
    generator.emitNode(dst, m_statement);

    generator.emitLabel(scope->continueTarget());
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());

    generator.emitNodeInConditionContext(m_expr, topOfLoop.get(), scope->breakTarget(), FallThroughMeansFalse);

    generator.emitLabel(scope->breakTarget());
}

// ------------------------------ ForNode --------------------------------------

void ForNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    LabelScopePtr scope = generator.newLabelScope(LabelScope::Loop);

    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());

    if (m_expr1)
        generator.emitNode(generator.ignoredResult(), m_expr1);
    
    RefPtr<Label> topOfLoop = generator.newLabel();
    if (m_expr2)
        generator.emitNodeInConditionContext(m_expr2, topOfLoop.get(), scope->breakTarget(), FallThroughMeansTrue);

    generator.emitLabel(topOfLoop.get());
    generator.emitLoopHint();

    generator.emitNode(dst, m_statement);

    generator.emitLabel(scope->continueTarget());
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    if (m_expr3)
        generator.emitNode(generator.ignoredResult(), m_expr3);

    if (m_expr2)
        generator.emitNodeInConditionContext(m_expr2, topOfLoop.get(), scope->breakTarget(), FallThroughMeansFalse);
    else
        generator.emitJump(topOfLoop.get());

    generator.emitLabel(scope->breakTarget());
}

// ------------------------------ ForInNode ------------------------------------

void ForInNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    LabelScopePtr scope = generator.newLabelScope(LabelScope::Loop);

    if (!m_lexpr->isLocation()) {
        emitThrowReferenceError(generator, "Left side of for-in statement is not a reference.");
        return;
    }

    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());

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
    generator.emitLoopHint();

    RegisterID* propertyName;
    bool optimizedForinAccess = false;
    if (m_lexpr->isResolveNode()) {
        const Identifier& ident = static_cast<ResolveNode*>(m_lexpr)->identifier();
        ResolveResult resolveResult = generator.resolve(ident);
        propertyName = resolveResult.local();
        if (!propertyName) {
            propertyName = generator.newTemporary();
            RefPtr<RegisterID> protect = propertyName;
            NonlocalResolveInfo resolveVerifier;
            if (generator.isStrictMode())
                generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
            RegisterID* base = generator.emitResolveBaseForPut(generator.newTemporary(), resolveResult, ident, resolveVerifier);

            generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
            generator.emitPutToBase(base, ident, propertyName, resolveVerifier);
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

        generator.emitExpressionInfo(assignNode->divot(), assignNode->divotStart(), assignNode->divotEnd());
        generator.emitPutById(base, ident, propertyName);
    } else {
        ASSERT(m_lexpr->isBracketAccessorNode());
        BracketAccessorNode* assignNode = static_cast<BracketAccessorNode*>(m_lexpr);
        propertyName = generator.newTemporary();
        RefPtr<RegisterID> protect = propertyName;
        RefPtr<RegisterID> base = generator.emitNode(assignNode->base());
        RegisterID* subscript = generator.emitNode(assignNode->subscript());
        
        generator.emitExpressionInfo(assignNode->divot(), assignNode->divotStart(), assignNode->divotEnd());
        generator.emitPutByVal(base.get(), subscript, propertyName);
    }   

    generator.emitNode(dst, m_statement);

    if (optimizedForinAccess)
        generator.popOptimisedForIn();

    generator.emitLabel(scope->continueTarget());
    generator.emitNextPropertyName(propertyName, base.get(), i.get(), size.get(), iter.get(), loopStart.get());
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    generator.emitLabel(scope->breakTarget());
}

// ------------------------------ ContinueNode ---------------------------------

Label* ContinueNode::trivialTarget(BytecodeGenerator& generator)
{
    if (generator.shouldEmitDebugHooks())
        return 0;

    LabelScope* scope = generator.continueTarget(m_ident);
    ASSERT(scope);

    if (generator.scopeDepth() != scope->scopeDepth())
        return 0;

    return scope->continueTarget();
}

void ContinueNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    
    LabelScope* scope = generator.continueTarget(m_ident);
    ASSERT(scope);

    generator.emitPopScopes(scope->scopeDepth());
    generator.emitJump(scope->continueTarget());
}

// ------------------------------ BreakNode ------------------------------------

Label* BreakNode::trivialTarget(BytecodeGenerator& generator)
{
    if (generator.shouldEmitDebugHooks())
        return 0;

    LabelScope* scope = generator.breakTarget(m_ident);
    ASSERT(scope);

    if (generator.scopeDepth() != scope->scopeDepth())
        return 0;

    return scope->breakTarget();
}

void BreakNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    
    LabelScope* scope = generator.breakTarget(m_ident);
    ASSERT(scope);

    generator.emitPopScopes(scope->scopeDepth());
    generator.emitJump(scope->breakTarget());
}

// ------------------------------ ReturnNode -----------------------------------

void ReturnNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    ASSERT(generator.codeType() == FunctionCode);

    if (dst == generator.ignoredResult())
        dst = 0;

    RefPtr<RegisterID> returnRegister = m_value ? generator.emitNode(dst, m_value) : generator.emitLoad(dst, jsUndefined());
    if (generator.scopeDepth()) {
        returnRegister = generator.emitMove(generator.newTemporary(), returnRegister.get());
        generator.emitPopScopes(0);
    }

    generator.emitDebugHook(WillLeaveCallFrame, firstLine(), lastLine(), startOffset(), lineStartOffset());
    generator.emitReturn(returnRegister.get());
}

// ------------------------------ WithNode -------------------------------------

void WithNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());

    RefPtr<RegisterID> scope = generator.emitNode(m_expr);
    generator.emitExpressionInfo(m_divot, m_divot - m_expressionLength, m_divot);
    generator.emitPushWithScope(scope.get());
    generator.emitNode(dst, m_statement);
    generator.emitPopScope();
}

// ------------------------------ CaseClauseNode --------------------------------

inline void CaseClauseNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (!m_statements)
        return;
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
            const String& value = static_cast<StringNode*>(clauseExpression)->value().string();
            if (singleCharacterSwitch &= value.length() == 1) {
                int32_t intVal = value[0];
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

static inline size_t length(ClauseListNode* list1, ClauseListNode* list2)
{
    size_t length = 0;
    for (ClauseListNode* node = list1; node; node = node->getNext())
        ++length;
    for (ClauseListNode* node = list2; node; node = node->getNext())
        ++length;
    return length;
}

SwitchInfo::SwitchType CaseBlockNode::tryTableSwitch(Vector<ExpressionNode*, 8>& literalVector, int32_t& min_num, int32_t& max_num)
{
    if (length(m_list1, m_list2) < s_tableSwitchMinimum)
        return SwitchInfo::SwitchNone;

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

void CaseBlockNode::emitBytecodeForBlock(BytecodeGenerator& generator, RegisterID* switchExpression, RegisterID* dst)
{
    RefPtr<Label> defaultLabel;
    Vector<RefPtr<Label>, 8> labelVector;
    Vector<ExpressionNode*, 8> literalVector;
    int32_t min_num = std::numeric_limits<int32_t>::max();
    int32_t max_num = std::numeric_limits<int32_t>::min();
    SwitchInfo::SwitchType switchType = tryTableSwitch(literalVector, min_num, max_num);

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
}

// ------------------------------ SwitchNode -----------------------------------

void SwitchNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());
    
    LabelScopePtr scope = generator.newLabelScope(LabelScope::Switch);

    RefPtr<RegisterID> r0 = generator.emitNode(m_expr);
    m_block->emitBytecodeForBlock(generator, r0.get(), dst);

    generator.emitLabel(scope->breakTarget());
}

// ------------------------------ LabelNode ------------------------------------

void LabelNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());

    ASSERT(!generator.breakTarget(m_name));

    LabelScopePtr scope = generator.newLabelScope(LabelScope::NamedLabel, &m_name);
    generator.emitNode(dst, m_statement);

    generator.emitLabel(scope->breakTarget());
}

// ------------------------------ ThrowNode ------------------------------------

void ThrowNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());

    if (dst == generator.ignoredResult())
        dst = 0;
    RefPtr<RegisterID> expr = generator.emitNode(m_expr);
    generator.emitExpressionInfo(divot(), divotStart(), divotEnd());
    generator.emitThrow(expr.get());
}

// ------------------------------ TryNode --------------------------------------

void TryNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    // NOTE: The catch and finally blocks must be labeled explicitly, so the
    // optimizer knows they may be jumped to from anywhere.

    generator.emitDebugHook(WillExecuteStatement, firstLine(), lastLine(), startOffset(), lineStartOffset());

    ASSERT(m_catchBlock || m_finallyBlock);

    RefPtr<Label> tryStartLabel = generator.newLabel();
    generator.emitLabel(tryStartLabel.get());
    
    if (m_finallyBlock)
        generator.pushFinallyContext(m_finallyBlock);
    TryData* tryData = generator.pushTry(tryStartLabel.get());

    generator.emitNode(dst, m_tryBlock);

    if (m_catchBlock) {
        RefPtr<Label> catchEndLabel = generator.newLabel();
        
        // Normal path: jump over the catch block.
        generator.emitJump(catchEndLabel.get());

        // Uncaught exception path: the catch block.
        RefPtr<Label> here = generator.emitLabel(generator.newLabel().get());
        RefPtr<RegisterID> exceptionRegister = generator.popTryAndEmitCatch(tryData, generator.newTemporary(), here.get());
        
        if (m_finallyBlock) {
            // If the catch block throws an exception and we have a finally block, then the finally
            // block should "catch" that exception.
            tryData = generator.pushTry(here.get());
        }
        
        generator.emitPushNameScope(m_exceptionIdent, exceptionRegister.get(), DontDelete);
        generator.emitNode(dst, m_catchBlock);
        generator.emitPopScope();
        generator.emitLabel(catchEndLabel.get());
    }

    if (m_finallyBlock) {
        RefPtr<Label> preFinallyLabel = generator.emitLabel(generator.newLabel().get());
        
        generator.popFinallyContext();

        RefPtr<Label> finallyEndLabel = generator.newLabel();

        // Normal path: run the finally code, and jump to the end.
        generator.emitNode(dst, m_finallyBlock);
        generator.emitJump(finallyEndLabel.get());

        // Uncaught exception path: invoke the finally block, then re-throw the exception.
        RefPtr<RegisterID> tempExceptionRegister = generator.popTryAndEmitCatch(tryData, generator.newTemporary(), preFinallyLabel.get());
        generator.emitNode(dst, m_finallyBlock);
        generator.emitThrow(tempExceptionRegister.get());

        generator.emitLabel(finallyEndLabel.get());
    }
}

// ------------------------------ ScopeNode -----------------------------

inline void ScopeNode::emitStatementsBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    if (!m_statements)
        return;
    m_statements->emitBytecode(generator, dst);
}

// ------------------------------ ProgramNode -----------------------------

void ProgramNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteProgram, startLine(), startLine(), startStartOffset(), startLineStartOffset());

    RefPtr<RegisterID> dstRegister = generator.newTemporary();
    generator.emitLoad(dstRegister.get(), jsUndefined());
    emitStatementsBytecode(generator, dstRegister.get());

    generator.emitDebugHook(DidExecuteProgram, lastLine(), lastLine(), startOffset(), lineStartOffset());
    generator.emitEnd(dstRegister.get());
}

// ------------------------------ EvalNode -----------------------------

void EvalNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(WillExecuteProgram, startLine(), startLine(), startStartOffset(), startLineStartOffset());

    RefPtr<RegisterID> dstRegister = generator.newTemporary();
    generator.emitLoad(dstRegister.get(), jsUndefined());
    emitStatementsBytecode(generator, dstRegister.get());

    generator.emitDebugHook(DidExecuteProgram, lastLine(), lastLine(), startOffset(), lineStartOffset());
    generator.emitEnd(dstRegister.get());
}

// ------------------------------ FunctionBodyNode -----------------------------

void FunctionBodyNode::emitBytecode(BytecodeGenerator& generator, RegisterID*)
{
    generator.emitDebugHook(DidEnterCallFrame, startLine(), startLine(), startStartOffset(), startLineStartOffset());
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
        ASSERT((startOffset() -  1) >= lineStartOffset());
        generator.emitDebugHook(WillLeaveCallFrame, lastLine(), lastLine(), startOffset() - 1, lineStartOffset());
        generator.emitReturn(r0);
        return;
    }

    // If there is a return statment, and it is the only statement in the function, check if this is a numeric compare.
    if (static_cast<BlockNode*>(singleStatement)->singleStatement()) {
        ExpressionNode* returnValueExpression = returnNode->value();
        if (returnValueExpression && returnValueExpression->isSubtract()) {
            ExpressionNode* lhsExpression = static_cast<SubNode*>(returnValueExpression)->lhs();
            ExpressionNode* rhsExpression = static_cast<SubNode*>(returnValueExpression)->rhs();
            if (lhsExpression->isResolveNode()
                && rhsExpression->isResolveNode()
                && generator.isArgumentNumber(static_cast<ResolveNode*>(lhsExpression)->identifier(), 0)
                && generator.isArgumentNumber(static_cast<ResolveNode*>(rhsExpression)->identifier(), 1)) {
                
                generator.setIsNumericCompareFunction(true);
            }
        }
    }
}

// ------------------------------ FuncDeclNode ---------------------------------

void FuncDeclNode::emitBytecode(BytecodeGenerator&, RegisterID*)
{
}

// ------------------------------ FuncExprNode ---------------------------------

RegisterID* FuncExprNode::emitBytecode(BytecodeGenerator& generator, RegisterID* dst)
{
    return generator.emitNewFunctionExpression(generator.finalDestination(dst), this);
}

} // namespace JSC
