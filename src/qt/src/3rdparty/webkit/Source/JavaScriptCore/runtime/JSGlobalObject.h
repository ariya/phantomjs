/*
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *  Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef JSGlobalObject_h
#define JSGlobalObject_h

#include "JSArray.h"
#include "JSGlobalData.h"
#include "JSVariableObject.h"
#include "JSWeakObjectMapRefInternal.h"
#include "NumberPrototype.h"
#include "StringPrototype.h"
#include "StructureChain.h"
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/RandomNumber.h>

namespace JSC {

    class ArrayPrototype;
    class BooleanPrototype;
    class DatePrototype;
    class Debugger;
    class ErrorConstructor;
    class FunctionPrototype;
    class GlobalCodeBlock;
    class NativeErrorConstructor;
    class ProgramCodeBlock;
    class RegExpConstructor;
    class RegExpPrototype;
    class RegisterFile;

    struct ActivationStackNode;
    struct HashTable;

    typedef Vector<ExecState*, 16> ExecStateStack;
    
    class JSGlobalObject : public JSVariableObject {
    protected:
        typedef HashSet<RefPtr<OpaqueJSWeakObjectMap> > WeakMapSet;

        RefPtr<JSGlobalData> m_globalData;

        size_t m_registerArraySize;
        Register m_globalCallFrame[RegisterFile::CallFrameHeaderSize];

        WriteBarrier<ScopeChainNode> m_globalScopeChain;
        WriteBarrier<JSObject> m_methodCallDummy;

        WriteBarrier<RegExpConstructor> m_regExpConstructor;
        WriteBarrier<ErrorConstructor> m_errorConstructor;
        WriteBarrier<NativeErrorConstructor> m_evalErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_rangeErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_referenceErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_syntaxErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_typeErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_URIErrorConstructor;

        WriteBarrier<JSFunction> m_evalFunction;
        WriteBarrier<JSFunction> m_callFunction;
        WriteBarrier<JSFunction> m_applyFunction;

        WriteBarrier<ObjectPrototype> m_objectPrototype;
        WriteBarrier<FunctionPrototype> m_functionPrototype;
        WriteBarrier<ArrayPrototype> m_arrayPrototype;
        WriteBarrier<BooleanPrototype> m_booleanPrototype;
        WriteBarrier<StringPrototype> m_stringPrototype;
        WriteBarrier<NumberPrototype> m_numberPrototype;
        WriteBarrier<DatePrototype> m_datePrototype;
        WriteBarrier<RegExpPrototype> m_regExpPrototype;

        WriteBarrier<Structure> m_argumentsStructure;
        WriteBarrier<Structure> m_arrayStructure;
        WriteBarrier<Structure> m_booleanObjectStructure;
        WriteBarrier<Structure> m_callbackConstructorStructure;
        WriteBarrier<Structure> m_callbackFunctionStructure;
        WriteBarrier<Structure> m_callbackObjectStructure;
        WriteBarrier<Structure> m_dateStructure;
        WriteBarrier<Structure> m_emptyObjectStructure;
        WriteBarrier<Structure> m_nullPrototypeObjectStructure;
        WriteBarrier<Structure> m_errorStructure;
        WriteBarrier<Structure> m_functionStructure;
        WriteBarrier<Structure> m_numberObjectStructure;
        WriteBarrier<Structure> m_regExpMatchesArrayStructure;
        WriteBarrier<Structure> m_regExpStructure;
        WriteBarrier<Structure> m_stringObjectStructure;
        WriteBarrier<Structure> m_internalFunctionStructure;

        unsigned m_profileGroup;
        Debugger* m_debugger;

        WeakMapSet m_weakMaps;
        WeakRandom m_weakRandom;

        SymbolTable m_symbolTable;

        bool m_isEvalEnabled;

    public:
        void* operator new(size_t, JSGlobalData*);

        explicit JSGlobalObject(JSGlobalData& globalData, Structure* structure)
            : JSVariableObject(globalData, structure, &m_symbolTable, 0)
            , m_registerArraySize(0)
            , m_globalScopeChain()
            , m_weakRandom(static_cast<unsigned>(randomNumber() * (std::numeric_limits<unsigned>::max() + 1.0)))
            , m_isEvalEnabled(true)
        {
            COMPILE_ASSERT(JSGlobalObject::AnonymousSlotCount == 1, JSGlobalObject_has_only_a_single_slot);
            putThisToAnonymousValue(0);
            init(this);
        }

    protected:
        JSGlobalObject(JSGlobalData& globalData, Structure* structure, JSObject* thisValue)
            : JSVariableObject(globalData, structure, &m_symbolTable, 0)
            , m_registerArraySize(0)
            , m_globalScopeChain()
            , m_weakRandom(static_cast<unsigned>(randomNumber() * (std::numeric_limits<unsigned>::max() + 1.0)))
            , m_isEvalEnabled(true)
        {
            COMPILE_ASSERT(JSGlobalObject::AnonymousSlotCount == 1, JSGlobalObject_has_only_a_single_slot);
            putThisToAnonymousValue(0);
            init(thisValue);
        }

    public:
        virtual ~JSGlobalObject();

        virtual void visitChildren(SlotVisitor&);

        virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
        virtual bool hasOwnPropertyForWrite(ExecState*, const Identifier&);
        virtual void put(ExecState*, const Identifier&, JSValue, PutPropertySlot&);
        virtual void putWithAttributes(ExecState*, const Identifier& propertyName, JSValue value, unsigned attributes);

        virtual void defineGetter(ExecState*, const Identifier& propertyName, JSObject* getterFunc, unsigned attributes);
        virtual void defineSetter(ExecState*, const Identifier& propertyName, JSObject* setterFunc, unsigned attributes);

        // We use this in the code generator as we perform symbol table
        // lookups prior to initializing the properties
        bool symbolTableHasProperty(const Identifier& propertyName);

        // The following accessors return pristine values, even if a script 
        // replaces the global object's associated property.

        RegExpConstructor* regExpConstructor() const { return m_regExpConstructor.get(); }

        ErrorConstructor* errorConstructor() const { return m_errorConstructor.get(); }
        NativeErrorConstructor* evalErrorConstructor() const { return m_evalErrorConstructor.get(); }
        NativeErrorConstructor* rangeErrorConstructor() const { return m_rangeErrorConstructor.get(); }
        NativeErrorConstructor* referenceErrorConstructor() const { return m_referenceErrorConstructor.get(); }
        NativeErrorConstructor* syntaxErrorConstructor() const { return m_syntaxErrorConstructor.get(); }
        NativeErrorConstructor* typeErrorConstructor() const { return m_typeErrorConstructor.get(); }
        NativeErrorConstructor* URIErrorConstructor() const { return m_URIErrorConstructor.get(); }

        JSFunction* evalFunction() const { return m_evalFunction.get(); }
        JSFunction* callFunction() const { return m_callFunction.get(); }
        JSFunction* applyFunction() const { return m_applyFunction.get(); }

        ObjectPrototype* objectPrototype() const { return m_objectPrototype.get(); }
        FunctionPrototype* functionPrototype() const { return m_functionPrototype.get(); }
        ArrayPrototype* arrayPrototype() const { return m_arrayPrototype.get(); }
        BooleanPrototype* booleanPrototype() const { return m_booleanPrototype.get(); }
        StringPrototype* stringPrototype() const { return m_stringPrototype.get(); }
        NumberPrototype* numberPrototype() const { return m_numberPrototype.get(); }
        DatePrototype* datePrototype() const { return m_datePrototype.get(); }
        RegExpPrototype* regExpPrototype() const { return m_regExpPrototype.get(); }

        JSObject* methodCallDummy() const { return m_methodCallDummy.get(); }

        Structure* argumentsStructure() const { return m_argumentsStructure.get(); }
        Structure* arrayStructure() const { return m_arrayStructure.get(); }
        Structure* booleanObjectStructure() const { return m_booleanObjectStructure.get(); }
        Structure* callbackConstructorStructure() const { return m_callbackConstructorStructure.get(); }
        Structure* callbackFunctionStructure() const { return m_callbackFunctionStructure.get(); }
        Structure* callbackObjectStructure() const { return m_callbackObjectStructure.get(); }
        Structure* dateStructure() const { return m_dateStructure.get(); }
        Structure* emptyObjectStructure() const { return m_emptyObjectStructure.get(); }
        Structure* nullPrototypeObjectStructure() const { return m_nullPrototypeObjectStructure.get(); }
        Structure* errorStructure() const { return m_errorStructure.get(); }
        Structure* functionStructure() const { return m_functionStructure.get(); }
        Structure* numberObjectStructure() const { return m_numberObjectStructure.get(); }
        Structure* internalFunctionStructure() const { return m_internalFunctionStructure.get(); }
        Structure* regExpMatchesArrayStructure() const { return m_regExpMatchesArrayStructure.get(); }
        Structure* regExpStructure() const { return m_regExpStructure.get(); }
        Structure* stringObjectStructure() const { return m_stringObjectStructure.get(); }

        void setProfileGroup(unsigned value) { m_profileGroup = value; }
        unsigned profileGroup() const { return m_profileGroup; }

        Debugger* debugger() const { return m_debugger; }
        void setDebugger(Debugger* debugger) { m_debugger = debugger; }

        virtual bool supportsProfiling() const { return false; }
        virtual bool supportsRichSourceInfo() const { return true; }

        ScopeChainNode* globalScopeChain() { return m_globalScopeChain.get(); }

        virtual bool isGlobalObject() const { return true; }

        virtual ExecState* globalExec();

        virtual bool shouldInterruptScript() const { return true; }

        virtual bool allowsAccessFrom(const JSGlobalObject*) const { return true; }

        virtual bool isDynamicScope(bool& requiresDynamicChecks) const;

        void disableEval();
        bool isEvalEnabled() { return m_isEvalEnabled; }

        void copyGlobalsFrom(RegisterFile&);
        void copyGlobalsTo(RegisterFile&);
        void resizeRegisters(int oldSize, int newSize);

        void resetPrototype(JSGlobalData&, JSValue prototype);

        JSGlobalData& globalData() const { return *m_globalData.get(); }

        static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

        void registerWeakMap(OpaqueJSWeakObjectMap* map)
        {
            m_weakMaps.add(map);
        }

        void deregisterWeakMap(OpaqueJSWeakObjectMap* map)
        {
            m_weakMaps.remove(map);
        }

        double weakRandomNumber() { return m_weakRandom.get(); }
    protected:

        static const unsigned AnonymousSlotCount = JSVariableObject::AnonymousSlotCount + 1;
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | OverridesGetPropertyNames | JSVariableObject::StructureFlags;

        struct GlobalPropertyInfo {
            GlobalPropertyInfo(const Identifier& i, JSValue v, unsigned a)
                : identifier(i)
                , value(v)
                , attributes(a)
            {
            }

            const Identifier identifier;
            JSValue value;
            unsigned attributes;
        };
        void addStaticGlobals(GlobalPropertyInfo*, int count);

    private:
        // FIXME: Fold reset into init.
        void init(JSObject* thisValue);
        void reset(JSValue prototype);

        void setRegisters(WriteBarrier<Unknown>* registers, PassOwnArrayPtr<WriteBarrier<Unknown> > registerArray, size_t count);

        void* operator new(size_t); // can only be allocated with JSGlobalData
    };

    JSGlobalObject* asGlobalObject(JSValue);

    inline JSGlobalObject* asGlobalObject(JSValue value)
    {
        ASSERT(asObject(value)->isGlobalObject());
        return static_cast<JSGlobalObject*>(asObject(value));
    }

    inline void JSGlobalObject::setRegisters(WriteBarrier<Unknown>* registers, PassOwnArrayPtr<WriteBarrier<Unknown> > registerArray, size_t count)
    {
        JSVariableObject::setRegisters(registers, registerArray);
        m_registerArraySize = count;
    }

    inline void JSGlobalObject::addStaticGlobals(GlobalPropertyInfo* globals, int count)
    {
        size_t oldSize = m_registerArraySize;
        size_t newSize = oldSize + count;
        OwnArrayPtr<WriteBarrier<Unknown> > registerArray = adoptArrayPtr(new WriteBarrier<Unknown>[newSize]);
        if (m_registerArray) {
            // memcpy is safe here as we're copying barriers we already own from the existing array
            memcpy(registerArray.get() + count, m_registerArray.get(), oldSize * sizeof(Register));
        }

        WriteBarrier<Unknown>* registers = registerArray.get() + newSize;
        setRegisters(registers, registerArray.release(), newSize);

        for (int i = 0, index = -static_cast<int>(oldSize) - 1; i < count; ++i, --index) {
            GlobalPropertyInfo& global = globals[i];
            ASSERT(global.attributes & DontDelete);
            SymbolTableEntry newEntry(index, global.attributes);
            symbolTable().add(global.identifier.impl(), newEntry);
            registerAt(index).set(globalData(), this, global.value);
        }
    }

    inline bool JSGlobalObject::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
    {
        if (JSVariableObject::getOwnPropertySlot(exec, propertyName, slot))
            return true;
        return symbolTableGet(propertyName, slot);
    }

    inline bool JSGlobalObject::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
    {
        if (symbolTableGet(propertyName, descriptor))
            return true;
        return JSVariableObject::getOwnPropertyDescriptor(exec, propertyName, descriptor);
    }

    inline bool JSGlobalObject::hasOwnPropertyForWrite(ExecState* exec, const Identifier& propertyName)
    {
        PropertySlot slot;
        if (JSVariableObject::getOwnPropertySlot(exec, propertyName, slot))
            return true;
        bool slotIsWriteable;
        return symbolTableGet(propertyName, slot, slotIsWriteable);
    }

    inline bool JSGlobalObject::symbolTableHasProperty(const Identifier& propertyName)
    {
        SymbolTableEntry entry = symbolTable().inlineGet(propertyName.impl());
        return !entry.isNull();
    }

    inline JSValue Structure::prototypeForLookup(ExecState* exec) const
    {
        if (typeInfo().type() == ObjectType)
            return m_prototype.get();

        ASSERT(typeInfo().type() == StringType);
        return exec->lexicalGlobalObject()->stringPrototype();
    }

    inline StructureChain* Structure::prototypeChain(ExecState* exec) const
    {
        // We cache our prototype chain so our clients can share it.
        if (!isValid(exec, m_cachedPrototypeChain.get())) {
            JSValue prototype = prototypeForLookup(exec);
            m_cachedPrototypeChain.set(exec->globalData(), this, StructureChain::create(exec->globalData(), prototype.isNull() ? 0 : asObject(prototype)->structure()));
        }
        return m_cachedPrototypeChain.get();
    }

    inline bool Structure::isValid(ExecState* exec, StructureChain* cachedPrototypeChain) const
    {
        if (!cachedPrototypeChain)
            return false;

        JSValue prototype = prototypeForLookup(exec);
        WriteBarrier<Structure>* cachedStructure = cachedPrototypeChain->head();
        while(*cachedStructure && !prototype.isNull()) {
            if (asObject(prototype)->structure() != cachedStructure->get())
                return false;
            ++cachedStructure;
            prototype = asObject(prototype)->prototype();
        }
        return prototype.isNull() && !*cachedStructure;
    }

    inline JSGlobalObject* ExecState::dynamicGlobalObject()
    {
        if (this == lexicalGlobalObject()->globalExec())
            return lexicalGlobalObject();

        // For any ExecState that's not a globalExec, the 
        // dynamic global object must be set since code is running
        ASSERT(globalData().dynamicGlobalObject);
        return globalData().dynamicGlobalObject;
    }

    inline JSObject* constructEmptyObject(ExecState* exec, JSGlobalObject* globalObject)
    {
        return constructEmptyObject(exec, globalObject->emptyObjectStructure());
    }

    inline JSObject* constructEmptyObject(ExecState* exec)
    {
        return constructEmptyObject(exec, exec->lexicalGlobalObject());
    }

    inline JSArray* constructEmptyArray(ExecState* exec, JSGlobalObject* globalObject)
    {
        return new (exec) JSArray(exec->globalData(), globalObject->arrayStructure());
    }
    
    inline JSArray* constructEmptyArray(ExecState* exec)
    {
        return constructEmptyArray(exec, exec->lexicalGlobalObject());
    }

    inline JSArray* constructEmptyArray(ExecState* exec, JSGlobalObject* globalObject, unsigned initialLength)
    {
        return new (exec) JSArray(exec->globalData(), globalObject->arrayStructure(), initialLength, CreateInitialized);
    }

    inline JSArray* constructEmptyArray(ExecState* exec, unsigned initialLength)
    {
        return constructEmptyArray(exec, exec->lexicalGlobalObject(), initialLength);
    }

    inline JSArray* constructArray(ExecState* exec, JSGlobalObject* globalObject, JSValue singleItemValue)
    {
        MarkedArgumentBuffer values;
        values.append(singleItemValue);
        return new (exec) JSArray(exec->globalData(), globalObject->arrayStructure(), values);
    }

    inline JSArray* constructArray(ExecState* exec, JSValue singleItemValue)
    {
        return constructArray(exec, exec->lexicalGlobalObject(), singleItemValue);
    }

    inline JSArray* constructArray(ExecState* exec, JSGlobalObject* globalObject, const ArgList& values)
    {
        return new (exec) JSArray(exec->globalData(), globalObject->arrayStructure(), values);
    }

    inline JSArray* constructArray(ExecState* exec, const ArgList& values)
    {
        return constructArray(exec, exec->lexicalGlobalObject(), values);
    }

    class DynamicGlobalObjectScope {
        WTF_MAKE_NONCOPYABLE(DynamicGlobalObjectScope);
    public:
        DynamicGlobalObjectScope(JSGlobalData&, JSGlobalObject*);

        ~DynamicGlobalObjectScope()
        {
            m_dynamicGlobalObjectSlot = m_savedDynamicGlobalObject;
        }

    private:
        JSGlobalObject*& m_dynamicGlobalObjectSlot;
        JSGlobalObject* m_savedDynamicGlobalObject;
    };

} // namespace JSC

#endif // JSGlobalObject_h
