//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATORESSL_H_
#define COMPILER_TRANSLATORESSL_H_

#include "compiler/ShHandle.h"

class TranslatorESSL : public TCompiler {
public:
    TranslatorESSL(ShShaderType type, ShShaderSpec spec);

protected:
    virtual void translate(TIntermNode* root);

private:
    void writeExtensionBehavior();
};

#endif  // COMPILER_TRANSLATORESSL_H_
