//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#include "common/angleutils.h"

#include "compiler/translator/BaseTypes.h"
#include "compiler/translator/Common.h"
#include "compiler/translator/compilerdebug.h"

struct TPublicType;
class TType;
class TSymbol;

class TField
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TField(TType *type, TString *name, const TSourceLoc &line)
        : mType(type),
          mName(name),
          mLine(line)
    {
    }

    // TODO(alokp): We should only return const type.
    // Fix it by tweaking grammar.
    TType *type()
    {
        return mType;
    }
    const TType *type() const
    {
        return mType;
    }

    const TString &name() const
    {
        return *mName;
    }
    const TSourceLoc &line() const
    {
        return mLine;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(TField);
    TType *mType;
    TString *mName;
    TSourceLoc mLine;
};

typedef TVector<TField *> TFieldList;
inline TFieldList *NewPoolTFieldList()
{
    void *memory = GetGlobalPoolAllocator()->allocate(sizeof(TFieldList));
    return new(memory) TFieldList;
}

class TFieldListCollection
{
  public:
    const TString &name() const
    {
        return *mName;
    }
    const TFieldList &fields() const
    {
        return *mFields;
    }

    const TString &mangledName() const
    {
        if (mMangledName.empty())
            mMangledName = buildMangledName();
        return mMangledName;
    }
    size_t objectSize() const
    {
        if (mObjectSize == 0)
            mObjectSize = calculateObjectSize();
        return mObjectSize;
    };

  protected:
    TFieldListCollection(const TString *name, TFieldList *fields)
        : mName(name),
          mFields(fields),
          mObjectSize(0)
    {
    }
    TString buildMangledName() const;
    size_t calculateObjectSize() const;
    virtual TString mangledNamePrefix() const = 0;

    const TString *mName;
    TFieldList *mFields;

    mutable TString mMangledName;
    mutable size_t mObjectSize;
};

// May also represent interface blocks
class TStructure : public TFieldListCollection
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TStructure(const TString *name, TFieldList *fields)
        : TFieldListCollection(name, fields),
          mDeepestNesting(0),
          mUniqueId(0)
    {
    }

    int deepestNesting() const
    {
        if (mDeepestNesting == 0)
            mDeepestNesting = calculateDeepestNesting();
        return mDeepestNesting;
    }
    bool containsArrays() const;

    bool equals(const TStructure &other) const;

    void setUniqueId(int uniqueId)
    {
        mUniqueId = uniqueId;
    }

    int uniqueId() const
    {
        ASSERT(mUniqueId != 0);
        return mUniqueId;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(TStructure);

    // TODO(zmo): Find a way to get rid of the const_cast in function
    // setName().  At the moment keep this function private so only
    // friend class RegenerateStructNames may call it.
    friend class RegenerateStructNames;
    void setName(const TString &name)
    {
        TString *mutableName = const_cast<TString *>(mName);
        *mutableName = name;
    }

    virtual TString mangledNamePrefix() const
    {
        return "struct-";
    }
    int calculateDeepestNesting() const;

    mutable int mDeepestNesting;
    int mUniqueId;
};

class TInterfaceBlock : public TFieldListCollection
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TInterfaceBlock(const TString *name, TFieldList *fields, const TString *instanceName,
                    int arraySize, const TLayoutQualifier &layoutQualifier)
        : TFieldListCollection(name, fields),
          mInstanceName(instanceName),
          mArraySize(arraySize),
          mBlockStorage(layoutQualifier.blockStorage),
          mMatrixPacking(layoutQualifier.matrixPacking)
    {
    }

    const TString &instanceName() const
    {
        return *mInstanceName;
    }
    bool hasInstanceName() const
    {
        return mInstanceName != NULL;
    }
    bool isArray() const
    {
        return mArraySize > 0;
    }
    int arraySize() const
    {
        return mArraySize;
    }
    TLayoutBlockStorage blockStorage() const
    {
        return mBlockStorage;
    }
    TLayoutMatrixPacking matrixPacking() const
    {
        return mMatrixPacking;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(TInterfaceBlock);
    virtual TString mangledNamePrefix() const
    {
        return "iblock-";
    }

    const TString *mInstanceName; // for interface block instance names
    int mArraySize; // 0 if not an array
    TLayoutBlockStorage mBlockStorage;
    TLayoutMatrixPacking mMatrixPacking;
};

//
// Base class for things that have a type.
//
class TType
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TType()
    {
    }
    TType(TBasicType t, unsigned char ps = 1, unsigned char ss = 1)
        : type(t), precision(EbpUndefined), qualifier(EvqGlobal),
          layoutQualifier(TLayoutQualifier::create()),
          primarySize(ps), secondarySize(ss), array(false), arraySize(0),
          interfaceBlock(0), structure(0)
    {
    }
    TType(TBasicType t, TPrecision p, TQualifier q = EvqTemporary,
          unsigned char ps = 1, unsigned char ss = 1, bool a = false)
        : type(t), precision(p), qualifier(q),
          layoutQualifier(TLayoutQualifier::create()),
          primarySize(ps), secondarySize(ss), array(a), arraySize(0),
          interfaceBlock(0), structure(0)
    {
    }
    explicit TType(const TPublicType &p);
    TType(TStructure *userDef, TPrecision p = EbpUndefined)
        : type(EbtStruct), precision(p), qualifier(EvqTemporary),
          layoutQualifier(TLayoutQualifier::create()),
          primarySize(1), secondarySize(1), array(false), arraySize(0),
          interfaceBlock(0), structure(userDef)
    {
    }
    TType(TInterfaceBlock *interfaceBlockIn, TQualifier qualifierIn,
          TLayoutQualifier layoutQualifierIn, int arraySizeIn)
        : type(EbtInterfaceBlock), precision(EbpUndefined), qualifier(qualifierIn),
          layoutQualifier(layoutQualifierIn),
          primarySize(1), secondarySize(1), array(arraySizeIn > 0), arraySize(arraySizeIn),
          interfaceBlock(interfaceBlockIn), structure(0)
    {
    }

    TBasicType getBasicType() const
    {
        return type;
    }
    void setBasicType(TBasicType t)
    {
        type = t;
    }

    TPrecision getPrecision() const
    {
        return precision;
    }
    void setPrecision(TPrecision p)
    {
        precision = p;
    }

    TQualifier getQualifier() const
    {
        return qualifier;
    }
    void setQualifier(TQualifier q)
    {
        qualifier = q;
    }

    TLayoutQualifier getLayoutQualifier() const
    {
        return layoutQualifier;
    }
    void setLayoutQualifier(TLayoutQualifier lq)
    {
        layoutQualifier = lq;
    }

    int getNominalSize() const
    {
        return primarySize;
    }
    int getSecondarySize() const
    {
        return secondarySize;
    }
    int getCols() const
    {
        ASSERT(isMatrix());
        return primarySize;
    }
    int getRows() const
    {
        ASSERT(isMatrix());
        return secondarySize;
    }
    void setPrimarySize(unsigned char ps)
    {
        primarySize = ps;
    }
    void setSecondarySize(unsigned char ss)
    {
        secondarySize = ss;
    }

    // Full size of single instance of type
    size_t getObjectSize() const;

    bool isMatrix() const
    {
        return primarySize > 1 && secondarySize > 1;
    }
    bool isArray() const
    {
        return array ? true : false;
    }
    int getArraySize() const
    {
        return arraySize;
    }
    void setArraySize(int s)
    {
        array = true;
        arraySize = s;
    }
    void clearArrayness()
    {
        array = false;
        arraySize = 0;
    }

    TInterfaceBlock *getInterfaceBlock() const
    {
        return interfaceBlock;
    }
    void setInterfaceBlock(TInterfaceBlock *interfaceBlockIn)
    {
        interfaceBlock = interfaceBlockIn;
    }
    bool isInterfaceBlock() const
    {
        return type == EbtInterfaceBlock;
    }

    bool isVector() const
    {
        return primarySize > 1 && secondarySize == 1;
    }
    bool isScalar() const
    {
        return primarySize == 1 && secondarySize == 1 && !structure;
    }
    bool isScalarInt() const
    {
        return isScalar() && (type == EbtInt || type == EbtUInt);
    }

    TStructure *getStruct() const
    {
        return structure;
    }
    void setStruct(TStructure *s)
    {
        structure = s;
    }

    const TString &getMangledName()
    {
        if (mangled.empty())
        {
            mangled = buildMangledName();
            mangled += ';';
        }

        return mangled;
    }

    bool sameElementType(const TType &right) const
    {
        return type == right.type &&
            primarySize == right.primarySize &&
            secondarySize == right.secondarySize &&
            structure == right.structure;
    }
    bool operator==(const TType &right) const
    {
        return type == right.type &&
            primarySize == right.primarySize &&
            secondarySize == right.secondarySize &&
            array == right.array && (!array || arraySize == right.arraySize) &&
            structure == right.structure;
        // don't check the qualifier, it's not ever what's being sought after
    }
    bool operator!=(const TType &right) const
    {
        return !operator==(right);
    }
    bool operator<(const TType &right) const
    {
        if (type != right.type)
            return type < right.type;
        if (primarySize != right.primarySize)
            return primarySize < right.primarySize;
        if (secondarySize != right.secondarySize)
            return secondarySize < right.secondarySize;
        if (array != right.array)
            return array < right.array;
        if (arraySize != right.arraySize)
            return arraySize < right.arraySize;
        if (structure != right.structure)
            return structure < right.structure;

        return false;
    }

    const char *getBasicString() const
    {
        return ::getBasicString(type);
    }
    const char *getPrecisionString() const
    {
        return ::getPrecisionString(precision);
    }
    const char *getQualifierString() const
    {
        return ::getQualifierString(qualifier);
    }
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
    int getDeepestStructNesting() const
    {
        return structure ? structure->deepestNesting() : 0;
    }

    bool isStructureContainingArrays() const
    {
        return structure ? structure->containsArrays() : false;
    }

  protected:
    TString buildMangledName() const;
    size_t getStructSize() const;
    void computeDeepestStructNesting();

    TBasicType type;
    TPrecision precision;
    TQualifier qualifier;
    TLayoutQualifier layoutQualifier;
    unsigned char primarySize; // size of vector or cols matrix
    unsigned char secondarySize; // rows of a matrix
    bool array;
    int arraySize;

    // 0 unless this is an interface block, or interface block member variable
    TInterfaceBlock *interfaceBlock;

    // 0 unless this is a struct
    TStructure *structure;

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
    TLayoutQualifier layoutQualifier;
    TQualifier qualifier;
    TPrecision precision;
    unsigned char primarySize;          // size of vector or cols of matrix
    unsigned char secondarySize;        // rows of matrix
    bool array;
    int arraySize;
    TType *userDef;
    TSourceLoc line;

    void setBasic(TBasicType bt, TQualifier q, const TSourceLoc &ln)
    {
        type = bt;
        layoutQualifier = TLayoutQualifier::create();
        qualifier = q;
        precision = EbpUndefined;
        primarySize = 1;
        secondarySize = 1;
        array = false;
        arraySize = 0;
        userDef = 0;
        line = ln;
    }

    void setAggregate(unsigned char size)
    {
        primarySize = size;
    }

    void setMatrix(unsigned char c, unsigned char r)
    {
        ASSERT(c > 1 && r > 1 && c <= 4 && r <= 4);
        primarySize = c;
        secondarySize = r;
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

    bool isMatrix() const
    {
        return primarySize > 1 && secondarySize > 1;
    }

    bool isVector() const
    {
        return primarySize > 1 && secondarySize == 1;
    }

    int getCols() const
    {
        ASSERT(isMatrix());
        return primarySize;
    }

    int getRows() const
    {
        ASSERT(isMatrix());
        return secondarySize;
    }

    int getNominalSize() const
    {
        return primarySize;
    }

    bool isAggregate() const
    {
        return array || isMatrix() || isVector();
    }
};

#endif // _TYPES_INCLUDED_
