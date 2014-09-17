/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef JSCell_h
#define JSCell_h

#include "CallData.h"
#include "CallFrame.h"
#include "ConstructData.h"
#include "Heap.h"
#include "JSLock.h"
#include "JSValueInlineMethods.h"
#include "MarkStack.h"
#include <wtf/Noncopyable.h>

namespace JSC {

    class JSGlobalObject;
    class Structure;

#if COMPILER(MSVC)
    // If WTF_MAKE_NONCOPYABLE is applied to JSCell we end up with a bunch of
    // undefined references to the JSCell copy constructor and assignment operator
    // when linking JavaScriptCore.
    class MSVCBugWorkaround {
        WTF_MAKE_NONCOPYABLE(MSVCBugWorkaround);

    protected:
        MSVCBugWorkaround() { }
        ~MSVCBugWorkaround() { }
    };

    class JSCell : MSVCBugWorkaround {
#else
    class JSCell {
        WTF_MAKE_NONCOPYABLE(JSCell);
#endif

        friend class ExecutableBase;
        friend class GetterSetter;
        friend class Heap;
        friend class JSObject;
        friend class JSPropertyNameIterator;
        friend class JSString;
        friend class JSValue;
        friend class JSAPIValueWrapper;
        friend class JSZombie;
        friend class JSGlobalData;
        friend class MarkedSpace;
        friend class MarkedBlock;
        friend class ScopeChainNode;
        friend class Structure;
        friend class StructureChain;

    protected:
        enum VPtrStealingHackType { VPtrStealingHack };

    private:
        explicit JSCell(VPtrStealingHackType) { }
        JSCell(JSGlobalData&, Structure*);
        virtual ~JSCell();
        static const ClassInfo s_dummyCellInfo;

    public:
        static Structure* createDummyStructure(JSGlobalData&);

        // Querying the type.
        bool isString() const;
        bool isObject() const;
        virtual bool isGetterSetter() const;
        bool inherits(const ClassInfo*) const;
        virtual bool isAPIValueWrapper() const { return false; }
        virtual bool isPropertyNameIterator() const { return false; }

        Structure* structure() const;

        // Extracting the value.
        bool getString(ExecState* exec, UString&) const;
        UString getString(ExecState* exec) const; // null string if not a string
        JSObject* getObject(); // NULL if not an object
        const JSObject* getObject() const; // NULL if not an object
        
        virtual CallType getCallData(CallData&);
        virtual ConstructType getConstructData(ConstructData&);

        // Extracting integer values.
        // FIXME: remove these methods, can check isNumberCell in JSValue && then call asNumberCell::*.
        virtual bool getUInt32(uint32_t&) const;

        // Basic conversions.
        virtual JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(ExecState*, double& number, JSValue&);
        virtual bool toBoolean(ExecState*) const;
        virtual double toNumber(ExecState*) const;
        virtual UString toString(ExecState*) const;
        virtual JSObject* toObject(ExecState*, JSGlobalObject*) const;

        // Garbage collection.
        void* operator new(size_t, ExecState*);
        void* operator new(size_t, JSGlobalData*);
        void* operator new(size_t, void* placementNewDestination) { return placementNewDestination; }

        virtual void visitChildren(SlotVisitor&);
#if ENABLE(JSC_ZOMBIES)
        virtual bool isZombie() const { return false; }
#endif

        // Object operations, with the toObject operation included.
        const ClassInfo* classInfo() const;
        virtual void put(ExecState*, const Identifier& propertyName, JSValue, PutPropertySlot&);
        virtual void put(ExecState*, unsigned propertyName, JSValue);
        virtual bool deleteProperty(ExecState*, const Identifier& propertyName);
        virtual bool deleteProperty(ExecState*, unsigned propertyName);

        virtual JSObject* toThisObject(ExecState*) const;
        virtual JSValue getJSNumber();
        void* vptr() { return *reinterpret_cast<void**>(this); }
        void setVPtr(void* vptr) { *reinterpret_cast<void**>(this) = vptr; }

        // FIXME: Rename getOwnPropertySlot to virtualGetOwnPropertySlot, and
        // fastGetOwnPropertySlot to getOwnPropertySlot. Callers should always
        // call this function, not its slower virtual counterpart. (For integer
        // property names, we want a similar interface with appropriate optimizations.)
        bool fastGetOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);

        static ptrdiff_t structureOffset()
        {
            return OBJECT_OFFSETOF(JSCell, m_structure);
        }

    protected:
        static const unsigned AnonymousSlotCount = 0;

    private:
        // Base implementation; for non-object classes implements getPropertySlot.
        virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
        
        WriteBarrier<Structure> m_structure;
    };

    inline JSCell::JSCell(JSGlobalData& globalData, Structure* structure)
        : m_structure(globalData, this, structure)
    {
        // Very first set of allocations won't have a real structure.
        ASSERT(m_structure || !globalData.dummyMarkableCellStructure);
    }

    inline JSCell::~JSCell()
    {
    }

    inline Structure* JSCell::structure() const
    {
        return m_structure.get();
    }

    inline void JSCell::visitChildren(SlotVisitor& visitor)
    {
        visitor.append(&m_structure);
    }

    // --- JSValue inlines ----------------------------

    inline bool JSValue::isString() const
    {
        return isCell() && asCell()->isString();
    }

    inline bool JSValue::isGetterSetter() const
    {
        return isCell() && asCell()->isGetterSetter();
    }

    inline bool JSValue::isObject() const
    {
        return isCell() && asCell()->isObject();
    }

    inline bool JSValue::getString(ExecState* exec, UString& s) const
    {
        return isCell() && asCell()->getString(exec, s);
    }

    inline UString JSValue::getString(ExecState* exec) const
    {
        return isCell() ? asCell()->getString(exec) : UString();
    }

    template <typename Base> UString HandleConverter<Base, Unknown>::getString(ExecState* exec) const
    {
        return jsValue().getString(exec);
    }

    inline JSObject* JSValue::getObject() const
    {
        return isCell() ? asCell()->getObject() : 0;
    }

    inline CallType getCallData(JSValue value, CallData& callData)
    {
        CallType result = value.isCell() ? value.asCell()->getCallData(callData) : CallTypeNone;
        ASSERT(result == CallTypeNone || value.isValidCallee());
        return result;
    }

    inline ConstructType getConstructData(JSValue value, ConstructData& constructData)
    {
        ConstructType result = value.isCell() ? value.asCell()->getConstructData(constructData) : ConstructTypeNone;
        ASSERT(result == ConstructTypeNone || value.isValidCallee());
        return result;
    }

    ALWAYS_INLINE bool JSValue::getUInt32(uint32_t& v) const
    {
        if (isInt32()) {
            int32_t i = asInt32();
            v = static_cast<uint32_t>(i);
            return i >= 0;
        }
        if (isDouble()) {
            double d = asDouble();
            v = static_cast<uint32_t>(d);
            return v == d;
        }
        return false;
    }

    inline JSValue JSValue::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
    {
        return isCell() ? asCell()->toPrimitive(exec, preferredType) : asValue();
    }

    inline bool JSValue::getPrimitiveNumber(ExecState* exec, double& number, JSValue& value)
    {
        if (isInt32()) {
            number = asInt32();
            value = *this;
            return true;
        }
        if (isDouble()) {
            number = asDouble();
            value = *this;
            return true;
        }
        if (isCell())
            return asCell()->getPrimitiveNumber(exec, number, value);
        if (isTrue()) {
            number = 1.0;
            value = *this;
            return true;
        }
        if (isFalse() || isNull()) {
            number = 0.0;
            value = *this;
            return true;
        }
        ASSERT(isUndefined());
        number = nonInlineNaN();
        value = *this;
        return true;
    }

    inline bool JSValue::toBoolean(ExecState* exec) const
    {
        if (isInt32())
            return asInt32() != 0;
        if (isDouble())
            return asDouble() > 0.0 || asDouble() < 0.0; // false for NaN
        if (isCell())
            return asCell()->toBoolean(exec);
        return isTrue(); // false, null, and undefined all convert to false.
    }

    ALWAYS_INLINE double JSValue::toNumber(ExecState* exec) const
    {
        if (isInt32())
            return asInt32();
        if (isDouble())
            return asDouble();
        if (isCell())
            return asCell()->toNumber(exec);
        if (isTrue())
            return 1.0;
        return isUndefined() ? nonInlineNaN() : 0; // null and false both convert to 0.
    }

    inline JSValue JSValue::getJSNumber()
    {
        if (isInt32() || isDouble())
            return *this;
        if (isCell())
            return asCell()->getJSNumber();
        return JSValue();
    }

    inline JSObject* JSValue::toObject(ExecState* exec) const
    {
        return isCell() ? asCell()->toObject(exec, exec->lexicalGlobalObject()) : toObjectSlowCase(exec, exec->lexicalGlobalObject());
    }

    inline JSObject* JSValue::toObject(ExecState* exec, JSGlobalObject* globalObject) const
    {
        return isCell() ? asCell()->toObject(exec, globalObject) : toObjectSlowCase(exec, globalObject);
    }

    inline JSObject* JSValue::toThisObject(ExecState* exec) const
    {
        return isCell() ? asCell()->toThisObject(exec) : toThisObjectSlowCase(exec);
    }

    inline Heap* Heap::heap(JSValue v)
    {
        if (!v.isCell())
            return 0;
        return heap(v.asCell());
    }

    inline Heap* Heap::heap(JSCell* c)
    {
        return MarkedSpace::heap(c);
    }
    
#if ENABLE(JSC_ZOMBIES)
    inline bool JSValue::isZombie() const
    {
        return isCell() && asCell() > (JSCell*)0x1ffffffffL && asCell()->isZombie();
    }
#endif

    inline void* MarkedBlock::allocate()
    {
        while (m_nextAtom < m_endAtom) {
            if (!m_marks.testAndSet(m_nextAtom)) {
                JSCell* cell = reinterpret_cast<JSCell*>(&atoms()[m_nextAtom]);
                m_nextAtom += m_atomsPerCell;
                cell->~JSCell();
                return cell;
            }
            m_nextAtom += m_atomsPerCell;
        }

        return 0;
    }
    
    inline MarkedSpace::SizeClass& MarkedSpace::sizeClassFor(size_t bytes)
    {
        ASSERT(bytes && bytes < maxCellSize);
        if (bytes < preciseCutoff)
            return m_preciseSizeClasses[(bytes - 1) / preciseStep];
        return m_impreciseSizeClasses[(bytes - 1) / impreciseStep];
    }

    inline void* MarkedSpace::allocate(size_t bytes)
    {
        SizeClass& sizeClass = sizeClassFor(bytes);
        return allocateFromSizeClass(sizeClass);
    }
    
    inline void* Heap::allocate(size_t bytes)
    {
        ASSERT(globalData()->identifierTable == wtfThreadData().currentIdentifierTable());
        ASSERT(JSLock::lockCount() > 0);
        ASSERT(JSLock::currentThreadIsHoldingLock());
        ASSERT(bytes <= MarkedSpace::maxCellSize);
        ASSERT(m_operationInProgress == NoOperation);

        m_operationInProgress = Allocation;
        void* result = m_markedSpace.allocate(bytes);
        m_operationInProgress = NoOperation;
        if (result)
            return result;

        return allocateSlowCase(bytes);
    }

    inline void* JSCell::operator new(size_t size, JSGlobalData* globalData)
    {
        return globalData->heap.allocate(size);
    }

    inline void* JSCell::operator new(size_t size, ExecState* exec)
    {
        return exec->heap()->allocate(size);
    }

} // namespace JSC

#endif // JSCell_h
