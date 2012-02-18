/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef JSArray_h
#define JSArray_h

#include "JSObject.h"

#define CHECK_ARRAY_CONSISTENCY 0

namespace JSC {

    typedef HashMap<unsigned, WriteBarrier<Unknown> > SparseArrayValueMap;

    // This struct holds the actual data values of an array.  A JSArray object points to it's contained ArrayStorage
    // struct by pointing to m_vector.  To access the contained ArrayStorage struct, use the getStorage() and 
    // setStorage() methods.  It is important to note that there may be space before the ArrayStorage that 
    // is used to quick unshift / shift operation.  The actual allocated pointer is available by using:
    //     getStorage() - m_indexBias * sizeof(JSValue)
    struct ArrayStorage {
        unsigned m_length; // The "length" property on the array
        unsigned m_numValuesInVector;
        SparseArrayValueMap* m_sparseValueMap;
        void* subclassData; // A JSArray subclass can use this to fill the vector lazily.
        void* m_allocBase; // Pointer to base address returned by malloc().  Keeping this pointer does eliminate false positives from the leak detector.
        size_t reportedMapCapacity;
#if CHECK_ARRAY_CONSISTENCY
        bool m_inCompactInitialization;
#endif
        WriteBarrier<Unknown> m_vector[1];
    };

    // The CreateCompact creation mode is used for fast construction of arrays
    // whose size and contents are known at time of creation.
    //
    // There are two obligations when using this mode:
    //
    //   - uncheckedSetIndex() must be used when initializing the array.
    //   - setLength() must be called after initialization.

    enum ArrayCreationMode { CreateCompact, CreateInitialized };

    class JSArray : public JSNonFinalObject {
        friend class Walker;

    public:
        JSArray(VPtrStealingHackType);

        explicit JSArray(JSGlobalData&, Structure*);
        JSArray(JSGlobalData&, Structure*, unsigned initialLength, ArrayCreationMode);
        JSArray(JSGlobalData&, Structure*, const ArgList& initialValues);
        virtual ~JSArray();

        virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
        virtual void put(ExecState*, unsigned propertyName, JSValue); // FIXME: Make protected and add setItem.

        static JS_EXPORTDATA const ClassInfo s_info;
        
        unsigned length() const { return m_storage->m_length; }
        void setLength(unsigned); // OK to use on new arrays, but not if it might be a RegExpMatchArray.

        void sort(ExecState*);
        void sort(ExecState*, JSValue compareFunction, CallType, const CallData&);
        void sortNumeric(ExecState*, JSValue compareFunction, CallType, const CallData&);

        void push(ExecState*, JSValue);
        JSValue pop();

        void shiftCount(ExecState*, int count);
        void unshiftCount(ExecState*, int count);

        bool canGetIndex(unsigned i) { return i < m_vectorLength && m_storage->m_vector[i]; }
        JSValue getIndex(unsigned i)
        {
            ASSERT(canGetIndex(i));
            return m_storage->m_vector[i].get();
        }

        bool canSetIndex(unsigned i) { return i < m_vectorLength; }
        void setIndex(JSGlobalData& globalData, unsigned i, JSValue v)
        {
            ASSERT(canSetIndex(i));
            
            WriteBarrier<Unknown>& x = m_storage->m_vector[i];
            if (!x) {
                ArrayStorage *storage = m_storage;
                ++storage->m_numValuesInVector;
                if (i >= storage->m_length)
                    storage->m_length = i + 1;
            }
            x.set(globalData, this, v);
        }
        
        void uncheckedSetIndex(JSGlobalData& globalData, unsigned i, JSValue v)
        {
            ASSERT(canSetIndex(i));
            ArrayStorage *storage = m_storage;
#if CHECK_ARRAY_CONSISTENCY
            ASSERT(storage->m_inCompactInitialization);
#endif
            storage->m_vector[i].set(globalData, this, v);
        }

        void fillArgList(ExecState*, MarkedArgumentBuffer&);
        void copyToRegisters(ExecState*, Register*, uint32_t);

        static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }
        
        inline void visitChildrenDirect(SlotVisitor&);

        static ptrdiff_t storageOffset()
        {
            return OBJECT_OFFSETOF(JSArray, m_storage);
        }

        static ptrdiff_t vectorLengthOffset()
        {
            return OBJECT_OFFSETOF(JSArray, m_vectorLength);
        }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | OverridesGetPropertyNames | JSObject::StructureFlags;
        virtual void put(ExecState*, const Identifier& propertyName, JSValue, PutPropertySlot&);
        virtual bool deleteProperty(ExecState*, const Identifier& propertyName);
        virtual bool deleteProperty(ExecState*, unsigned propertyName);
        virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);
        virtual void visitChildren(SlotVisitor&);

        void* subclassData() const;
        void setSubclassData(void*);

    private:
        bool getOwnPropertySlotSlowCase(ExecState*, unsigned propertyName, PropertySlot&);
        void putSlowCase(ExecState*, unsigned propertyName, JSValue);

        unsigned getNewVectorLength(unsigned desiredLength);
        bool increaseVectorLength(unsigned newLength);
        bool increaseVectorPrefixLength(unsigned newLength);
        
        unsigned compactForSorting();

        enum ConsistencyCheckType { NormalConsistencyCheck, DestructorConsistencyCheck, SortConsistencyCheck };
        void checkConsistency(ConsistencyCheckType = NormalConsistencyCheck);

        unsigned m_vectorLength; // The valid length of m_vector
        int m_indexBias; // The number of JSValue sized blocks before ArrayStorage.
        ArrayStorage *m_storage;
    };

    JSArray* asArray(JSValue);

    inline JSArray* asArray(JSCell* cell)
    {
        ASSERT(cell->inherits(&JSArray::s_info));
        return static_cast<JSArray*>(cell);
    }

    inline JSArray* asArray(JSValue value)
    {
        return asArray(value.asCell());
    }

    inline bool isJSArray(JSGlobalData* globalData, JSCell* cell) { return cell->vptr() == globalData->jsArrayVPtr; }
    inline bool isJSArray(JSGlobalData* globalData, JSValue v) { return v.isCell() && isJSArray(globalData, v.asCell()); }

    inline void JSArray::visitChildrenDirect(SlotVisitor& visitor)
    {
        JSObject::visitChildrenDirect(visitor);
        
        ArrayStorage* storage = m_storage;

        unsigned usedVectorLength = std::min(storage->m_length, m_vectorLength);
        visitor.appendValues(storage->m_vector, usedVectorLength, MayContainNullValues);

        if (SparseArrayValueMap* map = storage->m_sparseValueMap) {
            SparseArrayValueMap::iterator end = map->end();
            for (SparseArrayValueMap::iterator it = map->begin(); it != end; ++it)
                visitor.append(&it->second);
        }
    }

    // Rule from ECMA 15.2 about what an array index is.
    // Must exactly match string form of an unsigned integer, and be less than 2^32 - 1.
    inline unsigned Identifier::toArrayIndex(bool& ok) const
    {
        unsigned i = toUInt32(ok);
        if (ok && i >= 0xFFFFFFFFU)
            ok = false;
        return i;
    }

} // namespace JSC

#endif // JSArray_h
