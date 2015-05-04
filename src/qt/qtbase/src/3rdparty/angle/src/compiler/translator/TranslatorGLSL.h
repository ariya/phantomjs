//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATORGLSL_H_
#define COMPILER_TRANSLATORGLSL_H_

#include "compiler/translator/Compiler.h"

class TranslatorGLSL : public TCompiler
{
  public:
    TranslatorGLSL(sh::GLenum type, ShShaderSpec spec);

  protected:
    virtual void translate(TIntermNode *root);

  private:
    void writeVersion(TIntermNode *root);
    void writeExtensionBehavior();
};

#endif  // COMPILER_TRANSLATORGLSL_H_
