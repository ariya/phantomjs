//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _CONSTANT_UNION_INCLUDED_
#define _CONSTANT_UNION_INCLUDED_

#include <assert.h>

class ConstantUnion {
public:
    ConstantUnion()
    {
        iConst = 0;
        type = EbtVoid;
    }

    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)        
    void setIConst(int i) {iConst = i; type = EbtInt; }
    void setFConst(float f) {fConst = f; type = EbtFloat; }
    void setBConst(bool b) {bConst = b; type = EbtBool; }

    int getIConst() { return iConst; }
    float getFConst() { return fConst; }
    bool getBConst() { return bConst; }
    int getIConst() const { return iConst; }
    float getFConst() const { return fConst; }
    bool getBConst() const { return bConst; }

    bool operator==(const int i) const
    {
        return i == iConst;
    }

    bool operator==(const float f) const
    {
        return f == fConst;
    }

    bool operator==(const bool b) const
    {
        return b == bConst;
    }

    bool operator==(const ConstantUnion& constant) const
    {
        if (constant.type != type)
            return false;

        switch (type) {
        case EbtInt:
            return constant.iConst == iConst;
        case EbtFloat:
            return constant.fConst == fConst;
        case EbtBool:
            return constant.bConst == bConst;
        default:
            return false;
        }
    }

    bool operator!=(const int i) const
    {
        return !operator==(i);
    }

    bool operator!=(const float f) const
    {
        return !operator==(f);
    }

    bool operator!=(const bool b) const
    {
        return !operator==(b);
    }

    bool operator!=(const ConstantUnion& constant) const
    {
        return !operator==(constant);
    }

    bool operator>(const ConstantUnion& constant) const
    { 
        assert(type == constant.type);
        switch (type) {
        case EbtInt:
            return iConst > constant.iConst;
        case EbtFloat:
            return fConst > constant.fConst;
        default:
            return false;   // Invalid operation, handled at semantic analysis
        }
    }

    bool operator<(const ConstantUnion& constant) const
    { 
        assert(type == constant.type);
        switch (type) {
        case EbtInt:
            return iConst < constant.iConst;
        case EbtFloat:
            return fConst < constant.fConst;
        default:
            return false;   // Invalid operation, handled at semantic analysis
        }
    }

    ConstantUnion operator+(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt: returnValue.setIConst(iConst + constant.iConst); break;
        case EbtFloat: returnValue.setFConst(fConst + constant.fConst); break;
        default: assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator-(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt: returnValue.setIConst(iConst - constant.iConst); break;
        case EbtFloat: returnValue.setFConst(fConst - constant.fConst); break;
        default: assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator*(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt: returnValue.setIConst(iConst * constant.iConst); break;
        case EbtFloat: returnValue.setFConst(fConst * constant.fConst); break; 
        default: assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator%(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt: returnValue.setIConst(iConst % constant.iConst); break;
        default:     assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator>>(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt: returnValue.setIConst(iConst >> constant.iConst); break;
        default:     assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator<<(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt: returnValue.setIConst(iConst << constant.iConst); break;
        default:     assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator&(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt:  returnValue.setIConst(iConst & constant.iConst); break;
        default:     assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator|(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt:  returnValue.setIConst(iConst | constant.iConst); break;
        default:     assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator^(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtInt:  returnValue.setIConst(iConst ^ constant.iConst); break;
        default:     assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator&&(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtBool: returnValue.setBConst(bConst && constant.bConst); break;
        default:     assert(false && "Default missing");
        }

        return returnValue;
    }

    ConstantUnion operator||(const ConstantUnion& constant) const
    { 
        ConstantUnion returnValue;
        assert(type == constant.type);
        switch (type) {
        case EbtBool: returnValue.setBConst(bConst || constant.bConst); break;
        default:     assert(false && "Default missing");
        }

        return returnValue;
    }

    TBasicType getType() const { return type; }
private:

    union  {
        int iConst;  // used for ivec, scalar ints
        bool bConst; // used for bvec, scalar bools
        float fConst;   // used for vec, mat, scalar floats
    } ;

    TBasicType type;
};

#endif // _CONSTANT_UNION_INCLUDED_
