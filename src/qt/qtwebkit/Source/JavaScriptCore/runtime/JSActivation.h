/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef JSActivation_h
#define JSActivation_h

#include "CodeBlock.h"
#include "CopiedSpaceInlines.h"
#include "JSVariableObject.h"
#include "Nodes.h"
#include "SymbolTable.h"

namespace JSC {

    class Register;
    
    class JSActivation : public JSVariableObject {
    private:
        JSActivation(VM& vm, CallFrame*, SharedSymbolTable*);
    
    public:
        typedef JSVariableObject Base;

        static JSActivation* create(VM& vm, CallFrame* callFrame, CodeBlock* codeBlock)
        {
            SharedSymbolTable* symbolTable = codeBlock->symbolTable();
            JSActivation* activation = new (
                NotNull,
                allocateCell<JSActivation>(
                    vm.heap,
                    allocationSize(symbolTable)
                )
            ) JSActivation(vm, callFrame, symbolTable);
            activation->finishCreation(vm);
            return activation;
        }

        static void visitChildren(JSCell*, SlotVisitor&);

        bool isDynamicScope(bool& requiresDynamicChecks) const;

        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        static void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        JS_EXPORT_PRIVATE static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);

        static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);

        static void putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned attributes);
        static bool deleteProperty(JSCell*, ExecState*, PropertyName);

        static JSObject* toThisObject(JSCell*, ExecState*);

        void tearOff(VM&);
        
        static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto) { return Structure::create(vm, globalObject, proto, TypeInfo(ActivationObjectType, StructureFlags), &s_info); }

        WriteBarrierBase<Unknown>& registerAt(int) const;
        bool isValidIndex(int) const;
        bool isValid(const SymbolTableEntry&) const;
        bool isTornOff();
        int registersOffset();
        static int registersOffset(SharedSymbolTable*);

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | OverridesGetPropertyNames | Base::StructureFlags;

    private:
        bool symbolTableGet(PropertyName, PropertySlot&);
        bool symbolTableGet(PropertyName, PropertyDescriptor&);
        bool symbolTableGet(PropertyName, PropertySlot&, bool& slotIsWriteable);
        bool symbolTablePut(ExecState*, PropertyName, JSValue, bool shouldThrow);
        bool symbolTablePutWithAttributes(VM&, PropertyName, JSValue, unsigned attributes);

        static JSValue argumentsGetter(ExecState*, JSValue, PropertyName);
        NEVER_INLINE PropertySlot::GetValueFunc getArgumentsGetter();

        static size_t allocationSize(SharedSymbolTable*);
        static size_t storageOffset();

        WriteBarrier<Unknown>* storage(); // captureCount() number of registers.
    };

    extern int activationCount;
    extern int allTheThingsCount;

    inline JSActivation::JSActivation(VM& vm, CallFrame* callFrame, SharedSymbolTable* symbolTable)
        : Base(
            vm,
            callFrame->lexicalGlobalObject()->activationStructure(),
            callFrame->registers(),
            callFrame->scope(),
            symbolTable
        )
    {
        WriteBarrier<Unknown>* storage = this->storage();
        size_t captureCount = symbolTable->captureCount();
        for (size_t i = 0; i < captureCount; ++i)
            new(&storage[i]) WriteBarrier<Unknown>;
    }

    JSActivation* asActivation(JSValue);

    inline JSActivation* asActivation(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&JSActivation::s_info));
        return jsCast<JSActivation*>(asObject(value));
    }
    
    ALWAYS_INLINE JSActivation* Register::activation() const
    {
        return asActivation(jsValue());
    }

    inline bool JSActivation::isDynamicScope(bool& requiresDynamicChecks) const
    {
        requiresDynamicChecks = symbolTable()->usesNonStrictEval();
        return false;
    }

    inline int JSActivation::registersOffset(SharedSymbolTable* symbolTable)
    {
        return storageOffset() - (symbolTable->captureStart() * sizeof(WriteBarrier<Unknown>));
    }

    inline void JSActivation::tearOff(VM& vm)
    {
        ASSERT(!isTornOff());

        WriteBarrierBase<Unknown>* dst = reinterpret_cast_ptr<WriteBarrierBase<Unknown>*>(
            reinterpret_cast<char*>(this) + registersOffset(symbolTable()));
        WriteBarrierBase<Unknown>* src = m_registers;

        int captureEnd = symbolTable()->captureEnd();
        for (int i = symbolTable()->captureStart(); i < captureEnd; ++i)
            dst[i].set(vm, this, src[i].get());

        m_registers = dst;
        ASSERT(isTornOff());
    }

    inline bool JSActivation::isTornOff()
    {
        return m_registers == reinterpret_cast_ptr<WriteBarrierBase<Unknown>*>(
            reinterpret_cast<char*>(this) + registersOffset(symbolTable()));
    }

    inline size_t JSActivation::storageOffset()
    {
        return WTF::roundUpToMultipleOf<sizeof(WriteBarrier<Unknown>)>(sizeof(JSActivation));
    }

    inline WriteBarrier<Unknown>* JSActivation::storage()
    {
        return reinterpret_cast_ptr<WriteBarrier<Unknown>*>(
            reinterpret_cast<char*>(this) + storageOffset());
    }

    inline size_t JSActivation::allocationSize(SharedSymbolTable* symbolTable)
    {
        size_t objectSizeInBytes = WTF::roundUpToMultipleOf<sizeof(WriteBarrier<Unknown>)>(sizeof(JSActivation));
        size_t storageSizeInBytes = symbolTable->captureCount() * sizeof(WriteBarrier<Unknown>);
        return objectSizeInBytes + storageSizeInBytes;
    }

    inline bool JSActivation::isValidIndex(int index) const
    {
        if (index < symbolTable()->captureStart())
            return false;
        if (index >= symbolTable()->captureEnd())
            return false;
        return true;
    }

    inline bool JSActivation::isValid(const SymbolTableEntry& entry) const
    {
        return isValidIndex(entry.getIndex());
    }

    inline WriteBarrierBase<Unknown>& JSActivation::registerAt(int index) const
    {
        ASSERT(isValidIndex(index));
        return Base::registerAt(index);
    }

} // namespace JSC

#endif // JSActivation_h
