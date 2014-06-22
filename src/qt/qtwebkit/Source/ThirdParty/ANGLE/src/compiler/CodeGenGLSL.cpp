//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/TranslatorGLSL.h"
#include "compiler/TranslatorESSL.h"

//
// This function must be provided to create the actual
// compile object used by higher level code.  It returns
// a subclass of TCompiler.
//
TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output)
{
    switch (output) {
      case SH_GLSL_OUTPUT:
        return new TranslatorGLSL(type, spec);
      case SH_ESSL_OUTPUT:
        return new TranslatorESSL(type, spec);
      default:
        return NULL;
    }
}

//
// Delete the compiler made by ConstructCompiler
//
void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}
