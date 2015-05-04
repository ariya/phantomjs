//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/TranslatorHLSL.h"

#include "compiler/translator/InitializeParseContext.h"
#include "compiler/translator/OutputHLSL.h"

TranslatorHLSL::TranslatorHLSL(sh::GLenum type, ShShaderSpec spec, ShShaderOutput output)
    : TCompiler(type, spec, output)
{
}

void TranslatorHLSL::translate(TIntermNode *root)
{
    TParseContext& parseContext = *GetGlobalParseContext();
    sh::OutputHLSL outputHLSL(parseContext, this);

    outputHLSL.output();

    mInterfaceBlockRegisterMap = outputHLSL.getInterfaceBlockRegisterMap();
    mUniformRegisterMap = outputHLSL.getUniformRegisterMap();
}

bool TranslatorHLSL::hasInterfaceBlock(const std::string &interfaceBlockName) const
{
    return (mInterfaceBlockRegisterMap.count(interfaceBlockName) > 0);
}

unsigned int TranslatorHLSL::getInterfaceBlockRegister(const std::string &interfaceBlockName) const
{
    ASSERT(hasInterfaceBlock(interfaceBlockName));
    return mInterfaceBlockRegisterMap.find(interfaceBlockName)->second;
}

bool TranslatorHLSL::hasUniform(const std::string &uniformName) const
{
    return (mUniformRegisterMap.count(uniformName) > 0);
}

unsigned int TranslatorHLSL::getUniformRegister(const std::string &uniformName) const
{
    ASSERT(hasUniform(uniformName));
    return mUniformRegisterMap.find(uniformName)->second;
}
