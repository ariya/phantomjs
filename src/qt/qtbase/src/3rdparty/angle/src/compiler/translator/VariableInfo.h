//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_VARIABLE_INFO_H_
#define COMPILER_VARIABLE_INFO_H_

#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/intermediate.h"

// Provides information about a variable.
// It is currently being used to store info about active attribs and uniforms.
struct TVariableInfo {
    TVariableInfo(ShDataType type, int size);
    TVariableInfo();

    TPersistString name;
    TPersistString mappedName;
    ShDataType type;
    int size;
    bool isArray;
    TPrecision precision;
    bool staticUse;
};
typedef std::vector<TVariableInfo> TVariableInfoList;

// Traverses intermediate tree to collect all attributes, uniforms, varyings.
class CollectVariables : public TIntermTraverser {
public:
    CollectVariables(TVariableInfoList& attribs,
                     TVariableInfoList& uniforms,
                     TVariableInfoList& varyings,
                     ShHashFunction64 hashFunction);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);

private:
    TVariableInfoList& mAttribs;
    TVariableInfoList& mUniforms;
    TVariableInfoList& mVaryings;

    bool mPointCoordAdded;
    bool mFrontFacingAdded;
    bool mFragCoordAdded;

    ShHashFunction64 mHashFunction;
};

#endif  // COMPILER_VARIABLE_INFO_H_
