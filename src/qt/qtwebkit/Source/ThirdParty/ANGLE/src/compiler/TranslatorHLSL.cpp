//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/TranslatorHLSL.h"

#include "compiler/InitializeParseContext.h"
#include "compiler/OutputHLSL.h"

TranslatorHLSL::TranslatorHLSL(ShShaderType type, ShShaderSpec spec, ShShaderOutput output)
    : TCompiler(type, spec), mOutputType(output)
{
}

void TranslatorHLSL::translate(TIntermNode *root)
{
    TParseContext& parseContext = *GetGlobalParseContext();
    sh::OutputHLSL outputHLSL(parseContext, getResources(), mOutputType);

    outputHLSL.output();
    mActiveUniforms = outputHLSL.getUniforms();
}
