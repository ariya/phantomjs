//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/RegenerateStructNames.h"
#include "compiler/translator/compilerdebug.h"

void RegenerateStructNames::visitSymbol(TIntermSymbol *symbol)
{
    ASSERT(symbol);
    TType *type = symbol->getTypePointer();
    ASSERT(type);
    TStructure *userType = type->getStruct();
    if (!userType)
        return;

    if (mSymbolTable.findBuiltIn(userType->name(), mShaderVersion))
    {
        // Built-in struct, do not touch it.
        return;
    }

    int uniqueId = userType->uniqueId();

    ASSERT(mScopeDepth > 0);
    if (mScopeDepth == 1)
    {
        // If a struct is defined at global scope, we don't map its name.
        // This is because at global level, the struct might be used to
        // declare a uniform, so the same name needs to stay the same for
        // vertex/fragment shaders. However, our mapping uses internal ID,
        // which will be different for the same struct in vertex/fragment
        // shaders.
        // This is OK because names for any structs defined in other scopes
        // will begin with "_webgl", which is reserved. So there will be
        // no conflicts among unmapped struct names from global scope and
        // mapped struct names from other scopes.
        // However, we need to keep track of these global structs, so if a
        // variable is used in a local scope, we don't try to modify the
        // struct name through that variable.
        mDeclaredGlobalStructs.insert(uniqueId);
        return;
    }
    if (mDeclaredGlobalStructs.count(uniqueId) > 0)
        return;
    // Map {name} to _webgl_struct_{uniqueId}_{name}.
    const char kPrefix[] = "_webgl_struct_";
    if (userType->name().find(kPrefix) == 0)
    {
        // The name has already been regenerated.
        return;
    }
    std::string id = Str(uniqueId);
    TString tmp = kPrefix + TString(id.c_str());
    tmp += "_" + userType->name();
    userType->setName(tmp);
}

bool RegenerateStructNames::visitAggregate(Visit, TIntermAggregate *aggregate)
{
    ASSERT(aggregate);
    switch (aggregate->getOp())
    {
      case EOpSequence:
        ++mScopeDepth;
        {
            TIntermSequence &sequence = *(aggregate->getSequence());
            for (size_t ii = 0; ii < sequence.size(); ++ii)
            {
                TIntermNode *node = sequence[ii];
                ASSERT(node != NULL);
                node->traverse(this);
            }
        }
        --mScopeDepth;
        return false;
      default:
        return true;
    }
}
