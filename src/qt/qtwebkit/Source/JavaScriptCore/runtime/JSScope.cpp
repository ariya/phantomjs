/*
 * Copyright (C) 2012 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSScope.h"

#include "JSActivation.h"
#include "JSGlobalObject.h"
#include "JSNameScope.h"
#include "JSWithScope.h"
#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSScope);

void JSScope::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSScope* thisObject = jsCast<JSScope*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_next);
}

bool JSScope::isDynamicScope(bool& requiresDynamicChecks) const
{
    switch (structure()->typeInfo().type()) {
    case GlobalObjectType:
        return static_cast<const JSGlobalObject*>(this)->isDynamicScope(requiresDynamicChecks);
    case ActivationObjectType:
        return static_cast<const JSActivation*>(this)->isDynamicScope(requiresDynamicChecks);
    case NameScopeObjectType:
        return static_cast<const JSNameScope*>(this)->isDynamicScope(requiresDynamicChecks);
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    return false;
}

JSObject* JSScope::objectAtScope(JSScope* scope)
{
    JSObject* object = scope;
    if (object->structure()->typeInfo().type() == WithScopeType)
        return jsCast<JSWithScope*>(object)->object();

    return object;
}

int JSScope::localDepth()
{
    int scopeDepth = 0;
    ScopeChainIterator iter = this->begin();
    ScopeChainIterator end = this->end();
    while (!iter->inherits(&JSActivation::s_info)) {
        ++iter;
        if (iter == end)
            break;
        ++scopeDepth;
    }
    return scopeDepth;
}

struct LookupResult {
    JSValue base() const { return m_base; }
    JSValue value() const { return m_value; }
    void setBase(JSValue base) { ASSERT(base); m_base = base; }
    void setValue(JSValue value) { ASSERT(value); m_value = value; }

private:
    JSValue m_base;
    JSValue m_value;
};


static void setPutPropertyAccessOffset(PutToBaseOperation* operation, PropertyOffset offset)
{
    ASSERT(isOutOfLineOffset(offset));
    operation->m_offset = offset;
    operation->m_offsetInButterfly = offsetInButterfly(offset);
}

static bool executeResolveOperations(CallFrame* callFrame, JSScope* scope, const Identifier& propertyName, ResolveOperation* pc, LookupResult& result)
{
    while (true) {
        switch (pc->m_operation) {
        case ResolveOperation::Fail:
            return false;
        case ResolveOperation::CheckForDynamicEntriesBeforeGlobalScope: {
            while (JSScope* nextScope = scope->next()) {
                if (scope->isActivationObject() && scope->structure() != scope->globalObject()->activationStructure())
                    return false;
                ASSERT(scope->isNameScopeObject() || scope->isVariableObject() || scope->isGlobalObject());
                scope = nextScope;
            }
            pc++;
            break;
        }
        case ResolveOperation::SetBaseToUndefined:
            result.setBase(jsUndefined());
            pc++;
            continue;
        case ResolveOperation::SetBaseToScope:
            result.setBase(scope);
            pc++;
            continue;
        case ResolveOperation::ReturnScopeAsBase:
            result.setBase(scope);
            return true;
        case ResolveOperation::SetBaseToGlobal:
            result.setBase(scope->globalObject());
            pc++;
            continue;
        case ResolveOperation::SkipScopes: {
            int count = pc->m_scopesToSkip;
            while (count--)
                scope = scope->next();
            ASSERT(scope);
            pc++;
            continue;
        }
        case ResolveOperation::SkipTopScopeNode:
            if (callFrame->r(pc->m_activationRegister).jsValue())
                scope = scope->next();
            ASSERT(scope);
            pc++;
            continue;
        case ResolveOperation::GetAndReturnScopedVar:
            ASSERT(jsCast<JSVariableObject*>(scope)->registerAt(pc->m_offset).get());
            result.setValue(jsCast<JSVariableObject*>(scope)->registerAt(pc->m_offset).get());
            return true;
        case ResolveOperation::GetAndReturnGlobalVar:
            result.setValue(pc->m_registerAddress->get());
            return true;
        case ResolveOperation::GetAndReturnGlobalVarWatchable:
            result.setValue(pc->m_registerAddress->get());
            return true;
        case ResolveOperation::ReturnGlobalObjectAsBase:
            result.setBase(callFrame->lexicalGlobalObject());
            return true;
        case ResolveOperation::GetAndReturnGlobalProperty: {
            JSGlobalObject* globalObject = scope->globalObject();
            if (globalObject->structure() == pc->m_structure.get()) {
                result.setValue(globalObject->getDirect(pc->m_offset));
                return true;
            }

            PropertySlot slot(globalObject);
            if (!globalObject->getPropertySlot(callFrame, propertyName, slot))
                return false;

            JSValue value = slot.getValue(callFrame, propertyName);
            if (callFrame->hadException())
                return false;

            Structure* structure = globalObject->structure();

            // Don't try to cache prototype lookups
            if (globalObject != slot.slotBase() || !slot.isCacheableValue() || !structure->propertyAccessesAreCacheable()) {
                result.setValue(value);
                return true;
            }

            pc->m_structure.set(callFrame->vm(), callFrame->codeBlock()->ownerExecutable(), structure);
            pc->m_offset = slot.cachedOffset();
            result.setValue(value);
            return true;
        }
        }
    }
}

template <JSScope::LookupMode mode, JSScope::ReturnValues returnValues> JSObject* JSScope::resolveContainingScopeInternal(CallFrame* callFrame, const Identifier& identifier, PropertySlot& slot, Vector<ResolveOperation>* operations, PutToBaseOperation* putToBaseOperation, bool )
{
    JSScope* scope = callFrame->scope();
    ASSERT(scope);
    int scopeCount = 0;
    bool seenGenericObjectScope = false;
    bool requiresDynamicChecks = false;
    bool skipTopScopeNode = false;
    int activationRegister = 0;
    CodeBlock* codeBlock = callFrame->codeBlock();
    if (mode == UnknownResolve) {
        ASSERT(operations->isEmpty());
        if (codeBlock->codeType() == FunctionCode && codeBlock->needsActivation()) {
            activationRegister = codeBlock->activationRegister();
            JSValue activation = callFrame->r(activationRegister).jsValue();
            
            // If the activation register doesn't match our actual scope, a dynamic
            // scope has been inserted so we shouldn't skip the top scope node.
            if (activation == scope) {
                jsCast<JSActivation*>(activation.asCell())->isDynamicScope(requiresDynamicChecks);
                if (!requiresDynamicChecks) {
                    ASSERT(jsCast<JSActivation*>(activation.asCell())->symbolTable()->get(identifier.impl()).isNull());
                    scope = scope->next();
                    ASSERT(scope);
                    skipTopScopeNode = true;
                }
            } else if (!activation)
                skipTopScopeNode = true;
        }
    } else
        ASSERT(operations->size());

    if (codeBlock->codeType() == EvalCode && scope->next())
        requiresDynamicChecks = true;

    if (mode == UnknownResolve && putToBaseOperation)
        putToBaseOperation->m_kind = PutToBaseOperation::Generic;

    do {
        JSObject* object = JSScope::objectAtScope(scope);
        slot = PropertySlot(object);

        bool currentScopeNeedsDynamicChecks = false;
        if (!(scope->isVariableObject() || scope->isNameScopeObject()) || (scope->next() && scope->isDynamicScope(currentScopeNeedsDynamicChecks)))
            seenGenericObjectScope = true;

        requiresDynamicChecks = requiresDynamicChecks || currentScopeNeedsDynamicChecks;

        if (object->getPropertySlot(callFrame, identifier, slot)) {
            if (mode == UnknownResolve) {
                if (seenGenericObjectScope)
                    goto fail;
                if (putToBaseOperation)
                    putToBaseOperation->m_isDynamic = requiresDynamicChecks;
                if (!scope->next()) {
                    // Global lookup of some kind
                    JSGlobalObject* globalObject = jsCast<JSGlobalObject*>(scope);
                    SymbolTableEntry entry = globalObject->symbolTable()->get(identifier.impl());
                    if (!entry.isNull()) {
                        if (requiresDynamicChecks)
                            operations->append(ResolveOperation::checkForDynamicEntriesBeforeGlobalScope());

                        if (putToBaseOperation) {
                            putToBaseOperation->m_isDynamic = requiresDynamicChecks;
                            if (entry.isReadOnly())
                                putToBaseOperation->m_kind = PutToBaseOperation::Readonly;
                            else if (entry.couldBeWatched()) {
                                putToBaseOperation->m_kind = PutToBaseOperation::GlobalVariablePutChecked;
                                putToBaseOperation->m_predicatePointer = entry.addressOfIsWatched();
                            } else
                                putToBaseOperation->m_kind = PutToBaseOperation::GlobalVariablePut;
                            putToBaseOperation->m_registerAddress = &globalObject->registerAt(entry.getIndex());
                        }
                        // Override custom accessor behaviour that the DOM introduces for some
                        // event handlers declared on function declarations.
                        if (!requiresDynamicChecks)
                            slot.setValue(globalObject, globalObject->registerAt(entry.getIndex()).get());
                        switch (returnValues) {
                        case ReturnValue:
                            ASSERT(!putToBaseOperation);
                            operations->append(ResolveOperation::getAndReturnGlobalVar(&globalObject->registerAt(entry.getIndex()), entry.couldBeWatched()));
                            break;
                        case ReturnBase:
                            ASSERT(putToBaseOperation);
                            operations->append(ResolveOperation::returnGlobalObjectAsBase());
                            break;
                        case ReturnBaseAndValue:
                            ASSERT(putToBaseOperation);
                            operations->append(ResolveOperation::setBaseToGlobal());
                            operations->append(ResolveOperation::getAndReturnGlobalVar(&globalObject->registerAt(entry.getIndex()), entry.couldBeWatched()));
                            break;
                        case ReturnThisAndValue:
                            ASSERT(!putToBaseOperation);
                            operations->append(ResolveOperation::setBaseToUndefined());
                            operations->append(ResolveOperation::getAndReturnGlobalVar(&globalObject->registerAt(entry.getIndex()), entry.couldBeWatched()));
                            break;
                        }
                    } else {
                        if (!slot.isCacheableValue() || slot.slotBase() != globalObject)
                            goto fail;

                        if (requiresDynamicChecks)
                            operations->append(ResolveOperation::checkForDynamicEntriesBeforeGlobalScope());

                        if (putToBaseOperation) {
                            putToBaseOperation->m_isDynamic = requiresDynamicChecks;
                            putToBaseOperation->m_kind = PutToBaseOperation::GlobalPropertyPut;
                            putToBaseOperation->m_structure.set(callFrame->vm(), callFrame->codeBlock()->ownerExecutable(), globalObject->structure());
                            setPutPropertyAccessOffset(putToBaseOperation, slot.cachedOffset());
                        }
                        switch (returnValues) {
                        case ReturnValue:
                            ASSERT(!putToBaseOperation);
                            operations->append(ResolveOperation::getAndReturnGlobalProperty());
                            break;
                        case ReturnBase:
                            ASSERT(putToBaseOperation);
                            operations->append(ResolveOperation::returnGlobalObjectAsBase());
                            break;
                        case ReturnBaseAndValue:
                            ASSERT(putToBaseOperation);
                            operations->append(ResolveOperation::setBaseToGlobal());
                            operations->append(ResolveOperation::getAndReturnGlobalProperty());
                            break;
                        case ReturnThisAndValue:
                            ASSERT(!putToBaseOperation);
                            operations->append(ResolveOperation::setBaseToUndefined());
                            operations->append(ResolveOperation::getAndReturnGlobalProperty());
                            break;
                        }
                    }
                    return object;
                }
                if (!requiresDynamicChecks) {
                    // Normal lexical lookup
                    JSVariableObject* variableObject = jsCast<JSVariableObject*>(scope);
                    ASSERT(variableObject);
                    ASSERT(variableObject->symbolTable());
                    SymbolTableEntry entry = variableObject->symbolTable()->get(identifier.impl());
                    // Defend against the variable being actually inserted by eval.
                    if (entry.isNull()) {
                        ASSERT(!jsDynamicCast<JSNameScope*>(variableObject));
                        goto fail;
                    }
                    // If we're getting the 'arguments' then give up on life.
                    if (identifier == callFrame->propertyNames().arguments)
                        goto fail;

                    if (putToBaseOperation) {
                        putToBaseOperation->m_kind = entry.isReadOnly() ? PutToBaseOperation::Readonly : PutToBaseOperation::VariablePut;
                        putToBaseOperation->m_structure.set(callFrame->vm(), callFrame->codeBlock()->ownerExecutable(), callFrame->lexicalGlobalObject()->activationStructure());
                        putToBaseOperation->m_offset = entry.getIndex();
                        putToBaseOperation->m_scopeDepth = (skipTopScopeNode ? 1 : 0) + scopeCount;
                    }

                    if (skipTopScopeNode)
                        operations->append(ResolveOperation::skipTopScopeNode(activationRegister));

                    operations->append(ResolveOperation::skipScopes(scopeCount));
                    switch (returnValues) {
                    case ReturnBaseAndValue:
                        operations->append(ResolveOperation::setBaseToScope());
                        operations->append(ResolveOperation::getAndReturnScopedVar(entry.getIndex()));
                        break;

                    case ReturnBase:
                        operations->append(ResolveOperation::returnScopeAsBase());
                        break;

                    case ReturnThisAndValue:
                        operations->append(ResolveOperation::setBaseToUndefined());
                        // fallthrough
                    case ReturnValue:
                        operations->append(ResolveOperation::getAndReturnScopedVar(entry.getIndex()));
                        break;
                    }
                    return object;
                }
            fail:
                if (!operations->size())
                    operations->append(ResolveOperation::resolveFail());
            }
            return object;
        }
        scopeCount++;
    } while ((scope = scope->next()));
    
    if (mode == UnknownResolve) {
        ASSERT(operations->isEmpty());
        if (seenGenericObjectScope) {
            operations->append(ResolveOperation::resolveFail());
            return 0;
        }
        if (putToBaseOperation) {
            putToBaseOperation->m_isDynamic = requiresDynamicChecks;
            putToBaseOperation->m_kind = PutToBaseOperation::GlobalPropertyPut;
            putToBaseOperation->m_structure.clear();
            putToBaseOperation->m_offset = -1;
        }
        if (requiresDynamicChecks)
            operations->append(ResolveOperation::checkForDynamicEntriesBeforeGlobalScope());
        switch (returnValues) {
        case ReturnValue:
            ASSERT(!putToBaseOperation);
            operations->append(ResolveOperation::getAndReturnGlobalProperty());
            break;
        case ReturnBase:
            ASSERT(putToBaseOperation);
            operations->append(ResolveOperation::returnGlobalObjectAsBase());
            break;
        case ReturnBaseAndValue:
            ASSERT(putToBaseOperation);
            operations->append(ResolveOperation::setBaseToGlobal());
            operations->append(ResolveOperation::getAndReturnGlobalProperty());
            break;
        case ReturnThisAndValue:
            ASSERT(!putToBaseOperation);
            operations->append(ResolveOperation::setBaseToUndefined());
            operations->append(ResolveOperation::getAndReturnGlobalProperty());
            break;
        }
    }
    return 0;
}

template <JSScope::ReturnValues returnValues> JSObject* JSScope::resolveContainingScope(CallFrame* callFrame, const Identifier& identifier, PropertySlot& slot, Vector<ResolveOperation>* operations, PutToBaseOperation* putToBaseOperation, bool isStrict)
{
    if (operations->size())
        return resolveContainingScopeInternal<KnownResolve, returnValues>(callFrame, identifier, slot, operations, putToBaseOperation, isStrict);
    JSObject* result = resolveContainingScopeInternal<UnknownResolve, returnValues>(callFrame, identifier, slot, operations, putToBaseOperation, isStrict);
    operations->shrinkToFit();
    return result;
}

JSValue JSScope::resolve(CallFrame* callFrame, const Identifier& identifier, ResolveOperations* operations)
{
    ASSERT(operations);
    LookupResult fastResult;
    if (operations->size() && executeResolveOperations(callFrame, callFrame->scope(), identifier, operations->data(), fastResult)) {
        ASSERT(fastResult.value());
        ASSERT(!callFrame->hadException());
        return fastResult.value();
    }

    if (callFrame->hadException())
        return JSValue();

    PropertySlot slot;
    if (JSScope::resolveContainingScope<ReturnValue>(callFrame, identifier, slot, operations, 0, false)) {
        ASSERT(operations->size());
        return slot.getValue(callFrame, identifier);
    }
    ASSERT(operations->size());

    return throwError(callFrame, createUndefinedVariableError(callFrame, identifier));
}

JSValue JSScope::resolveBase(CallFrame* callFrame, const Identifier& identifier, bool isStrict, ResolveOperations* operations, PutToBaseOperation* putToBaseOperations)
{
    ASSERT(operations);
    ASSERT_UNUSED(putToBaseOperations, putToBaseOperations);
    LookupResult fastResult;
    if (operations->size() && executeResolveOperations(callFrame, callFrame->scope(), identifier, operations->data(), fastResult)) {
        ASSERT(fastResult.base());
        ASSERT(!callFrame->hadException());
        return fastResult.base();
    }

    if (callFrame->hadException())
        return JSValue();

    PropertySlot slot;
    if (JSObject* base = JSScope::resolveContainingScope<ReturnBase>(callFrame, identifier, slot, operations, putToBaseOperations, isStrict)) {
        ASSERT(operations->size());
        return base;
    }

    if (!isStrict)
        return callFrame->lexicalGlobalObject();

    return throwError(callFrame, createErrorForInvalidGlobalAssignment(callFrame, identifier.string()));
}

JSValue JSScope::resolveWithBase(CallFrame* callFrame, const Identifier& identifier, Register* base, ResolveOperations* operations, PutToBaseOperation* putToBaseOperations)
{
    ASSERT(operations);
    ASSERT_UNUSED(putToBaseOperations, putToBaseOperations);
    LookupResult fastResult;
    if (operations->size() && executeResolveOperations(callFrame, callFrame->scope(), identifier, operations->data(), fastResult)) {
        ASSERT(fastResult.base());
        ASSERT(fastResult.value());
        ASSERT(!callFrame->hadException());
        *base = fastResult.base();
        return fastResult.value();
    }

    if (callFrame->hadException())
        return JSValue();

    PropertySlot slot;
    if (JSObject* propertyBase = JSScope::resolveContainingScope<ReturnBaseAndValue>(callFrame, identifier, slot, operations, putToBaseOperations, false)) {
        ASSERT(operations->size());
        JSValue value = slot.getValue(callFrame, identifier);
        if (callFrame->vm().exception)
            return JSValue();

        *base = propertyBase;
        return value;
    }
    ASSERT(operations->size());

    return throwError(callFrame, createUndefinedVariableError(callFrame, identifier));
}

JSValue JSScope::resolveWithThis(CallFrame* callFrame, const Identifier& identifier, Register* base, ResolveOperations* operations)
{
    ASSERT(operations);
    LookupResult fastResult;
    if (operations->size() && executeResolveOperations(callFrame, callFrame->scope(), identifier, operations->data(), fastResult)) {
        ASSERT(fastResult.base());
        ASSERT(fastResult.value());
        ASSERT(!callFrame->hadException());
        *base = fastResult.base();
        return fastResult.value();
    }

    if (callFrame->hadException())
        return JSValue();

    PropertySlot slot;
    if (JSObject* propertyBase = JSScope::resolveContainingScope<ReturnThisAndValue>(callFrame, identifier, slot, operations, 0, false)) {
        ASSERT(operations->size());
        JSValue value = slot.getValue(callFrame, identifier);
        if (callFrame->vm().exception)
            return JSValue();
        ASSERT(value);
        *base = propertyBase->structure()->typeInfo().isEnvironmentRecord() ? jsUndefined() : JSValue(propertyBase);
        return value;
    }
    ASSERT(operations->size());

    return throwError(callFrame, createUndefinedVariableError(callFrame, identifier));
}

void JSScope::resolvePut(CallFrame* callFrame, JSValue base, const Identifier& property, JSValue value, PutToBaseOperation* operation)
{
    ASSERT_UNUSED(operation, operation);
    ASSERT(base);
    ASSERT(value);
    switch (operation->m_kind) {
    case PutToBaseOperation::Uninitialised:
        CRASH();

    case PutToBaseOperation::Readonly:
        return;

    case PutToBaseOperation::GlobalVariablePutChecked:
        if (*operation->m_predicatePointer)
            goto genericHandler;
    case PutToBaseOperation::GlobalVariablePut:
        if (operation->m_isDynamic) {
            JSObject* baseObject = jsCast<JSObject*>(base);
            if (baseObject != callFrame->lexicalGlobalObject()) {
                if (baseObject->isGlobalObject())
                    ASSERT(!jsCast<JSGlobalObject*>(baseObject)->assertRegisterIsInThisObject(operation->m_registerAddress));
                goto genericHandler;
            }
        }
        operation->m_registerAddress->set(callFrame->vm(), base.asCell(), value);
        return;

    case PutToBaseOperation::VariablePut: {
        if (operation->m_isDynamic) {
            JSObject* baseObject = jsCast<JSObject*>(base);
            if (baseObject->structure() != operation->m_structure.get())
                goto genericHandler;
        }
        JSVariableObject* variableObject = jsCast<JSVariableObject*>(base);
        variableObject->registerAt(operation->m_offset).set(callFrame->vm(), variableObject, value);
        return;
    }

    case PutToBaseOperation::GlobalPropertyPut: {
        JSObject* object = jsCast<JSObject*>(base);
        if (operation->m_structure.get() != object->structure())
            break;
        object->putDirect(callFrame->vm(), operation->m_offset, value);
        return;
    }

    genericHandler:
    case PutToBaseOperation::Generic:
        PutPropertySlot slot(operation->m_isStrict);
        base.put(callFrame, property, value, slot);
        return;
    }
    ASSERT(operation->m_kind == PutToBaseOperation::GlobalPropertyPut);
    PutPropertySlot slot(operation->m_isStrict);
    base.put(callFrame, property, value, slot);
    if (!slot.isCacheable())
        return;
    if (callFrame->hadException())
        return;
    JSObject* baseObject = jsCast<JSObject*>(base);
    if (!baseObject->structure()->propertyAccessesAreCacheable())
        return;
    if (slot.base() != callFrame->lexicalGlobalObject())
        return;
    if (slot.base() != baseObject)
        return;
    ASSERT(!baseObject->hasInlineStorage());
    operation->m_structure.set(callFrame->vm(), callFrame->codeBlock()->ownerExecutable(), baseObject->structure());
    setPutPropertyAccessOffset(operation, slot.cachedOffset());
    return;
}

JSValue JSScope::resolveGlobal(CallFrame* callFrame, const Identifier& identifier, JSGlobalObject* globalObject, ResolveOperation* resolveOperation)
{
    ASSERT(resolveOperation);
    ASSERT(resolveOperation->m_operation == ResolveOperation::GetAndReturnGlobalProperty);
    ASSERT_UNUSED(globalObject, callFrame->lexicalGlobalObject() == globalObject);

    LookupResult fastResult;
    if (executeResolveOperations(callFrame, callFrame->scope(), identifier, resolveOperation, fastResult)) {
        ASSERT(fastResult.value());
        ASSERT(!callFrame->hadException());
        return fastResult.value();
    }

    if (callFrame->hadException())
        return JSValue();

    return throwError(callFrame, createUndefinedVariableError(callFrame, identifier));
}


} // namespace JSC
