//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/TranslatorHLSL.h"

//
// This function must be provided to create the actual
// compile object used by higher level code.  It returns
// a subclass of TCompiler.
//
TCompiler* ConstructCompiler(
    ShShaderType type, ShShaderSpec spec, ShShaderOutput output)
{
  switch (output)
  {
    case SH_HLSL9_OUTPUT:
    case SH_HLSL11_OUTPUT:
      return new TranslatorHLSL(type, spec, output);
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
