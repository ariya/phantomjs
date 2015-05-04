//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// UniformHLSL.h:
//   Methods for GLSL to HLSL translation for uniforms and interface blocks.
//

#ifndef TRANSLATOR_UNIFORMHLSL_H_
#define TRANSLATOR_UNIFORMHLSL_H_

#include "compiler/translator/Types.h"

namespace sh
{
class StructureHLSL;

class UniformHLSL
{
  public:
    UniformHLSL(StructureHLSL *structureHLSL, TranslatorHLSL *translator);

    void reserveUniformRegisters(unsigned int registerCount);
    void reserveInterfaceBlockRegisters(unsigned int registerCount);
    TString uniformsHeader(ShShaderOutput outputType, const ReferencedSymbols &referencedUniforms);
    TString interfaceBlocksHeader(const ReferencedSymbols &referencedInterfaceBlocks);

    // Used for direct index references
    static TString interfaceBlockInstanceString(const TInterfaceBlock& interfaceBlock, unsigned int arrayIndex);

    const std::map<std::string, unsigned int> &getInterfaceBlockRegisterMap() const
    {
        return mInterfaceBlockRegisterMap;
    }
    const std::map<std::string, unsigned int> &getUniformRegisterMap() const
    {
        return mUniformRegisterMap;
    }

  private:
    TString interfaceBlockString(const TInterfaceBlock &interfaceBlock, unsigned int registerIndex, unsigned int arrayIndex);
    TString interfaceBlockMembersString(const TInterfaceBlock &interfaceBlock, TLayoutBlockStorage blockStorage);
    TString interfaceBlockStructString(const TInterfaceBlock &interfaceBlock);
    const Uniform *findUniformByName(const TString &name) const;

    // Returns the uniform's register index
    unsigned int declareUniformAndAssignRegister(const TType &type, const TString &name);

    unsigned int mUniformRegister;
    unsigned int mInterfaceBlockRegister;
    unsigned int mSamplerRegister;
    StructureHLSL *mStructureHLSL;
    ShShaderOutput mOutputType;

    const std::vector<Uniform> &mUniforms;
    std::map<std::string, unsigned int> mInterfaceBlockRegisterMap;
    std::map<std::string, unsigned int> mUniformRegisterMap;
};

}

#endif // TRANSLATOR_UNIFORMHLSL_H_
