//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATORHLSL_H_
#define COMPILER_TRANSLATORHLSL_H_

#include "compiler/translator/Compiler.h"

class TranslatorHLSL : public TCompiler
{
  public:
    TranslatorHLSL(sh::GLenum type, ShShaderSpec spec, ShShaderOutput output);
    virtual TranslatorHLSL *getAsTranslatorHLSL() { return this; }

    bool hasInterfaceBlock(const std::string &interfaceBlockName) const;
    unsigned int getInterfaceBlockRegister(const std::string &interfaceBlockName) const;

    bool hasUniform(const std::string &uniformName) const;
    unsigned int getUniformRegister(const std::string &uniformName) const;

  protected:
    virtual void translate(TIntermNode* root);

    std::map<std::string, unsigned int> mInterfaceBlockRegisterMap;
    std::map<std::string, unsigned int> mUniformRegisterMap;
};

#endif  // COMPILER_TRANSLATORHLSL_H_
