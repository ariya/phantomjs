//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef CROSSCOMPILERGLSL_OUTPUTGLSL_H_
#define CROSSCOMPILERGLSL_OUTPUTGLSL_H_

#include "compiler/translator/OutputGLSLBase.h"

class TOutputGLSL : public TOutputGLSLBase
{
public:
    TOutputGLSL(TInfoSinkBase& objSink,
                ShArrayIndexClampingStrategy clampingStrategy,
                ShHashFunction64 hashFunction,
                NameMap& nameMap,
                TSymbolTable& symbolTable,
                int shaderVersion);

protected:
    virtual bool writeVariablePrecision(TPrecision);
    virtual void visitSymbol(TIntermSymbol* node);
    virtual TString translateTextureFunction(TString& name);
};

#endif  // CROSSCOMPILERGLSL_OUTPUTGLSL_H_
