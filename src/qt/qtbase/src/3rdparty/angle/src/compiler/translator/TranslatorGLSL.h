//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATORGLSL_H_
#define COMPILER_TRANSLATORGLSL_H_

#include "compiler/translator/ShHandle.h"

class TranslatorGLSL : public TCompiler {
public:
    TranslatorGLSL(ShShaderType type, ShShaderSpec spec);

protected:
    virtual void translate(TIntermNode* root);
};

#endif  // COMPILER_TRANSLATORGLSL_H_
