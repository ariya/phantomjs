//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_VARIABLE_INFO_H_
#define COMPILER_VARIABLE_INFO_H_

#include <GLSLANG/ShaderLang.h>

#include "compiler/translator/IntermNode.h"

class TSymbolTable;

namespace sh
{

// Traverses intermediate tree to collect all attributes, uniforms, varyings.
class CollectVariables : public TIntermTraverser
{
  public:
    CollectVariables(std::vector<Attribute> *attribs,
                     std::vector<Attribute> *outputVariables,
                     std::vector<Uniform> *uniforms,
                     std::vector<Varying> *varyings,
                     std::vector<InterfaceBlock> *interfaceBlocks,
                     ShHashFunction64 hashFunction,
                     const TSymbolTable &symbolTable);

    virtual void visitSymbol(TIntermSymbol *symbol);
    virtual bool visitAggregate(Visit, TIntermAggregate *node);
    virtual bool visitBinary(Visit visit, TIntermBinary *binaryNode);

  private:
    template <typename VarT>
    void visitVariable(const TIntermSymbol *variable, std::vector<VarT> *infoList) const;

    template <typename VarT>
    void visitInfoList(const TIntermSequence &sequence, std::vector<VarT> *infoList) const;

    std::vector<Attribute> *mAttribs;
    std::vector<Attribute> *mOutputVariables;
    std::vector<Uniform> *mUniforms;
    std::vector<Varying> *mVaryings;
    std::vector<InterfaceBlock> *mInterfaceBlocks;

    std::map<std::string, InterfaceBlockField *> mInterfaceBlockFields;

    bool mPointCoordAdded;
    bool mFrontFacingAdded;
    bool mFragCoordAdded;

    bool mPositionAdded;
    bool mPointSizeAdded;

    ShHashFunction64 mHashFunction;

    const TSymbolTable &mSymbolTable;
};

// Expand struct uniforms to flattened lists of split variables
void ExpandUniforms(const std::vector<Uniform> &compact,
                    std::vector<ShaderVariable> *expanded);

}

#endif  // COMPILER_VARIABLE_INFO_H_
