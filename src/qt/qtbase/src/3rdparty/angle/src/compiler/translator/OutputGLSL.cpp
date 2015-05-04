//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/OutputGLSL.h"

TOutputGLSL::TOutputGLSL(TInfoSinkBase& objSink,
                         ShArrayIndexClampingStrategy clampingStrategy,
                         ShHashFunction64 hashFunction,
                         NameMap& nameMap,
                         TSymbolTable& symbolTable,
                         int shaderVersion)
    : TOutputGLSLBase(objSink, clampingStrategy, hashFunction, nameMap, symbolTable, shaderVersion)
{
}

bool TOutputGLSL::writeVariablePrecision(TPrecision)
{
    return false;
}

void TOutputGLSL::visitSymbol(TIntermSymbol* node)
{
    TInfoSinkBase& out = objSink();

    if (node->getSymbol() == "gl_FragDepthEXT")
    {
        out << "gl_FragDepth";
    }
    else
    {
        TOutputGLSLBase::visitSymbol(node);
    }
}

TString TOutputGLSL::translateTextureFunction(TString& name)
{
    static const char *simpleRename[] = {
        "texture2DLodEXT", "texture2DLod",
        "texture2DProjLodEXT", "texture2DProjLod",
        "textureCubeLodEXT", "textureCubeLod",
        "texture2DGradEXT", "texture2DGradARB",
        "texture2DProjGradEXT", "texture2DProjGradARB",
        "textureCubeGradEXT", "textureCubeGradARB",
        NULL, NULL
    };

    for (int i = 0; simpleRename[i] != NULL; i += 2) {
        if (name == simpleRename[i]) {
            return simpleRename[i+1];
        }
    }

    return name;
}
