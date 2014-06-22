//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_VARIABLE_INFO_H_
#define COMPILER_VARIABLE_INFO_H_

#include "GLSLANG/ShaderLang.h"
#include "compiler/intermediate.h"

// Provides information about a variable.
// It is currently being used to store info about active attribs and uniforms.
struct TVariableInfo {
    TVariableInfo(ShDataType type, int size);
    TVariableInfo();

    TPersistString name;
    TPersistString mappedName;
    ShDataType type;
    int size;
};
typedef std::vector<TVariableInfo> TVariableInfoList;

// Traverses intermediate tree to collect all attributes and uniforms.
class CollectAttribsUniforms : public TIntermTraverser {
public:
    CollectAttribsUniforms(TVariableInfoList& attribs,
                           TVariableInfoList& uniforms,
                           ShHashFunction64 hashFunction);

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit, TIntermBinary*);
    virtual bool visitUnary(Visit, TIntermUnary*);
    virtual bool visitSelection(Visit, TIntermSelection*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    virtual bool visitLoop(Visit, TIntermLoop*);
    virtual bool visitBranch(Visit, TIntermBranch*);

private:
    TVariableInfoList& mAttribs;
    TVariableInfoList& mUniforms;

    ShHashFunction64 mHashFunction;
};

#endif  // COMPILER_VARIABLE_INFO_H_
