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
#include "ConstructData.h"
#include "Heap.h"
#include "JSLock.h"
#include "SlotVisitor.h"
#include "TypedArrayDescriptor.h"
#include "WriteBarrier.h"
#include <wtf/Noncopyable.h>
#include <wtf/TypeTraits.h>

namespace JSC {

class CopyVisitor;
class ExecState;
class JSDestructibleObject;
class JSGlobalObject;
class LLIntOffsetsExtractor;
class PropertyDescriptor;
class PropertyNameArray;
class Structure;

enum EnumerationMode {
    ExcludeDontEnumProperties,
    IncludeDontEnumProperties
};

template<typename T> void* allocateCell(Heap&);
template<typename T> void* allocateCell(Heap&, size_t);

class JSCell {
    friend class JSValue;
    friend class MarkedBlock;
    template<typename T> friend void* allocateCell(Heap&);
    template<typename T> friend void* allocateCell(Heap&, size_t);

public:
    static const unsigned StructureFlags = 0;

    static const bool needsDestruction = false;
    static const bool hasImmortalStructure = false;

    enum CreatingEarlyCellTag { CreatingEarlyCell };
    JSCell(CreatingEarlyCellTag);

protected:
    JSCell(VM&, Structure*);
    JS_EXPORT_PRIVATE static void destroy(JSCell*);

public:
    // Querying the type.
    bool isString() const;
    bool isObject() const;
    bool isGetterSetter() const;
    bool isProxy() const;
    bool inherits(const ClassInfo*) const;
    bool isAPIValueWrapper() const;

    Structure* structure() const;
    void setStructure(VM&, Structure*);
    void clearStructure() { m_structure.clear(); }

    const char* className();

    // Extracting the value.
    JS_EXPORT_PRIVATE bool getString(ExecState*, String&) const;
    JS_EXPORT_PRIVATE String getString(ExecState*) const; // null string if not a string
    JS_EXPORT_PRIVATE JSObject* getObject(); // NULL if not an object
    const JSObject* getObject() const; // NULL if not an object
        
    JS_EXPORT_PRIVATE static CallType getCallData(JSCell*, CallData&);
    JS_EXPORT_PRIVATE static ConstructType getConstructData(JSCell*, ConstructData&);

    // Basic conversions.
    JS_EXPORT_PRIVATE JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
    bool getPrimitiveNumber(ExecState*, double& number, JSValue&) const;
    bool toBoolean(ExecState*) const;
    TriState pureToBoolean() const;
    JS_EXPORT_PRIVATE double toNumber(ExecState*) const;
    JS_EXPORT_PRIVATE JSObject* toObject(ExecState*, JSGlobalObject*) const;

    static void visitChildren(JSCell*, SlotVisitor&);
    JS_EXPORT_PRIVATE static void copyBackingStore(JSCell*, CopyVisitor&);

    // Object operations, with the toObject operation included.
    const ClassInfo* classInfo() const;
    const MethodTable* methodTable() const;
    const MethodTable* methodTableForDestruction() const;
    static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    static void putByIndex(JSCell*, ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
        
    static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned propertyName);

    static JSObject* toThisObject(JSCell*, ExecState*);

    void zap() { *reinterpret_cast<uintptr_t**>(this) = 0; }
    bool isZapped() const { return !*reinterpret_cast<uintptr_t* const*>(this); }

    // FIXME: Rename getOwnPropertySlot to virtualGetOwnPropertySlot, and
    // fastGetOwnPropertySlot to getOwnPropertySlot. Callers should always
    // call this function, not its slower virtual counterpart. (For integer
    // property names, we want a similar interface with appropriate optimizations.)
    bool fastGetOwnPropertySlot(ExecState*, PropertyName, PropertySlot&);
    JSValue fastGetOwnProperty(ExecState*, const String&);

    static ptrdiff_t structureOffset()
    {
        return OBJECT_OFFSETOF(JSCell, m_structure);
    }

    void* structureAddress()
    {
        return &m_structure;
    }
        
#if ENABLE(GC_VALIDATION)
    Structure* unvalidatedStructure() { return m_structure.unvalidatedGet(); }
#endif
        
    static const TypedArrayType TypedArrayStorageType = TypedArrayNone;
protected:

    void finishCreation(VM&);
    void finishCreation(VM&, Structure*, CreatingEarlyCellTag);

    // Base implementation; for non-object classes implements getPropertySlot.
    static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
    static bool getOwnPropertySlotByIndex(JSCell*, ExecState*, unsigned propertyName, PropertySlot&);

    // Dummy implementations of override-able static functions for classes to put in their MethodTable
    static JSValue defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType);
    static NO_RETURN_DUE_TO_CRASH void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    static NO_RETURN_DUE_TO_CRASH void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    static NO_RETURN_DUE_TO_CRASH void getPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    static String className(const JSObject*);
    JS_EXPORT_PRIVATE static bool customHasInstance(JSObject*, ExecState*, JSValue);
    static NO_RETURN_DUE_TO_CRASH void putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned attributes);
    static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool shouldThrow);
    static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);

private:
    friend class LLIntOffsetsExtractor;
        
    WriteBarrier<Structure> m_structure;
};

template<typename To, typename From>
inline To jsCast(From* from)
{
    ASSERT(!from || from->JSCell::inherits(&WTF::RemovePointer<To>::Type::s_info));
    return static_cast<To>(from);
}
    
template<typename To>
inline To jsCast(JSValue from)
{
    ASSERT(from.isCell() && from.asCell()->JSCell::inherits(&WTF::RemovePointer<To>::Type::s_info));
    return static_cast<To>(from.asCell());
}

template<typename To, typename From>
inline To jsDynamicCast(From* from)
{
    return from->inherits(&WTF::RemovePointer<To>::Type::s_info) ? static_cast<To>(from) : 0;
}

template<typename To>
inline To jsDynamicCast(JSValue from)
{
    return from.isCell() && from.asCell()->inherits(&WTF::RemovePointer<To>::Type::s_info) ? static_cast<To>(from.asCell()) : 0;
}

} // namespace JSC

#endif // JSCell_h
