//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#include "common/angleutils.h"

#include "compiler/BaseTypes.h"
#include "compiler/Common.h"
#include "compiler/debug.h"

struct TPublicType;
class TType;

class TField
{
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator);
    TField(TType* type, TString* name) : mType(type), mName(name) {}

    // TODO(alokp): We should only return const type.
    // Fix it by tweaking grammar.
    TType* type() { return mType; }
    const TType* type() const { return mType; }

    const TString& name() const { return *mName; }

private:
    DISALLOW_COPY_AND_ASSIGN(TField);
    TType* mType;
    TString* mName;
};

typedef TVector<TField*> TFieldList;
inline TFieldList* NewPoolTFieldList()
{
    void* memory = GlobalPoolAllocator.allocate(sizeof(TFieldList));
    return new(memory) TFieldList;
}

class TStructure
{
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator);
    TStructure(TString* name, TFieldList* fields)
        : mName(name),
          mFields(fields),
          mObjectSize(0),
          mDeepestNesting(0) {
    }

    const TString& name() const { return *mName; }
    const TFieldList& fields() const { return *mFields; }

    const TString& mangledName() const {
        if (mMangledName.empty())
            mMangledName = buildMangledName();
        return mMangledName;
    }
    size_t objectSize() const {
        if (mObjectSize == 0)
            mObjectSize = calculateObjectSize();
        return mObjectSize;
    };
    int deepestNesting() const {
        if (mDeepestNesting == 0)
            mDeepestNesting = calculateDeepestNesting();
        return mDeepestNesting;
    }
    bool containsArrays() const;

private:
    DISALLOW_COPY_AND_ASSIGN(TStructure);
    TString buildMangledName() const;
    size_t calculateObjectSize() const;
    int calculateDeepestNesting() const;

    TString* mName;
    TFieldList* mFields;

    mutable TString mMangledName;
    mutable size_t mObjectSize;
    mutable int mDeepestNesting;
};

//
// Base class for things that have a type.
//
class TType
{
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)
    TType() {}
    TType(TBasicType t, TPrecision p, TQualifier q = EvqTemporary, int s = 1, bool m = false, bool a = false) :
            type(t), precision(p), qualifier(q), size(s), matrix(m), array(a), arraySize(0), structure(0)
    {
    }
    explicit TType(const TPublicType &p);
    TType(TStructure* userDef, TPrecision p = EbpUndefined) :
            type(EbtStruct), precision(p), qualifier(EvqTemporary), size(1), matrix(false), array(false), arraySize(0), structure(userDef)
    {
    }

    TBasicType getBasicType() const { return type; }
    void setBasicType(TBasicType t) { type = t; }

    TPrecision getPrecision() const { return precision; }
    void setPrecision(TPrecision p) { precision = p; }

    TQualifier getQualifier() const { return qualifier; }
    void setQualifier(TQualifier q) { qualifier = q; }

    // One-dimensional size of single instance type
    int getNominalSize() const { return size; }
    void setNominalSize(int s) { size = s; }
    // Full size of single instance of type
    size_t getObjectSize() const;

    int elementRegisterCount() const
    {
        if (structure)
        {
            const TFieldList &fields = getStruct()->fields();
            int registerCount = 0;

            for (size_t i = 0; i < fields.size(); i++)
            {
                registerCount += fields[i]->type()->totalRegisterCount();
            }

            return registerCount;
        }
        else if (isMatrix())
        {
            return getNominalSize();
        }
        else
        {
            return 1;
        }
    }

    int totalRegisterCount() const
    {
        if (array)
        {
            return arraySize * elementRegisterCount();
        }
        else
        {
            return elementRegisterCount();
        }
    }

    bool isMatrix() const { return matrix ? true : false; }
    void setMatrix(bool m) { matrix = m; }

    bool isArray() const  { return array ? true : false; }
    int getArraySize() const { return arraySize; }
    void setArraySize(int s) { array = true; arraySize = s; }
    void clearArrayness() { array = false; arraySize = 0; }

    bool isVector() const { return size > 1 && !matrix; }
    bool isScalar() const { return size == 1 && !matrix && !structure; }

    TStructure* getStruct() const { return structure; }
    void setStruct(TStructure* s) { structure = s; }

    const TString& getMangledName() const {
        if (mangled.empty()) {
            mangled = buildMangledName();
            mangled += ';';
        }
        return mangled;
    }

    bool sameElementType(const TType& right) const {
        return      type == right.type   &&
                    size == right.size   &&
                  matrix == right.matrix &&
               structure == right.structure;
    }
    bool operator==(const TType& right) const {
        return      type == right.type   &&
                    size == right.size   &&
                  matrix == right.matrix &&
                   array == right.array  && (!array || arraySize == right.arraySize) &&
               structure == right.structure;
        // don't check the qualifier, it's not ever what's being sought after
    }
    bool operator!=(const TType& right) const {
        return !operator==(right);
    }
    bool operator<(const TType& right) const {
        if (type != right.type) return type < right.type;
        if (size != right.size) return size < right.size;
        if (matrix != right.matrix) return matrix < right.matrix;
        if (array != right.array) return array < right.array;
        if (arraySize != right.arraySize) return arraySize < right.arraySize;
        if (structure != right.structure) return structure < right.structure;

        return false;
    }

    const char* getBasicString() const { return ::getBasicString(type); }
    const char* getPrecisionString() const { return ::getPrecisionString(precision); }
    const char* getQualifierString() const { return ::getQualifierString(qualifier); }
    TString getCompleteString() const;

    // If this type is a struct, returns the deepest struct nesting of
    // any field in the struct. For example:
    //   struct nesting1 {
    //     vec4 position;
    //   };
    //   struct nesting2 {
    //     nesting1 field1;
    //     vec4 field2;
    //   };
    // For type "nesting2", this method would return 2 -- the number
    // of structures through which indirection must occur to reach the
    // deepest field (nesting2.field1.position).
    int getDeepestStructNesting() const {
        return structure ? structure->deepestNesting() : 0;
    }

    bool isStructureContainingArrays() const {
        return structure ? structure->containsArrays() : false;
    }

private:
    TString buildMangledName() const;

#ifdef __GNUC__
    TBasicType type;
    TPrecision precision;
    TQualifier qualifier;
#else
    TBasicType type      : 6;
    TPrecision precision;
    TQualifier qualifier : 7;
#endif
    int size             : 8; // size of vector or matrix, not size of array
    unsigned int matrix  : 1;
    unsigned int array   : 1;
    int arraySize;

    TStructure* structure;      // 0 unless this is a struct

    mutable TString mangled;
};

//
// This is a workaround for a problem with the yacc stack,  It can't have
// types that it thinks have non-trivial constructors.  It should
// just be used while recognizing the grammar, not anything else.  Pointers
// could be used, but also trying to avoid lots of memory management overhead.
//
// Not as bad as it looks, there is no actual assumption that the fields
// match up or are name the same or anything like that.
//
struct TPublicType
{
    TBasicType type;
    TQualifier qualifier;
    TPrecision precision;
    int size;          // size of vector or matrix, not size of array
    bool matrix;
    bool array;
    int arraySize;
    TType* userDef;
    TSourceLoc line;

    void setBasic(TBasicType bt, TQualifier q, const TSourceLoc& ln)
    {
        type = bt;
        qualifier = q;
        precision = EbpUndefined;
        size = 1;
        matrix = false;
        array = false;
        arraySize = 0;
        userDef = 0;
        line = ln;
    }

    void setAggregate(int s, bool m = false)
    {
        size = s;
        matrix = m;
    }

    void setArray(bool a, int s = 0)
    {
        array = a;
        arraySize = s;
    }

    bool isStructureContainingArrays() const
    {
        if (!userDef)
        {
            return false;
        }

        return userDef->isStructureContainingArrays();
    }
};

#endif // _TYPES_INCLUDED_
