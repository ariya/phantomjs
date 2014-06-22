//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/TranslatorGLSL.h"

#include "compiler/OutputGLSL.h"
#include "compiler/VersionGLSL.h"

static void writeVersion(ShShaderType type, TIntermNode* root,
                         TInfoSinkBase& sink) {
    TVersionGLSL versionGLSL(type);
    root->traverse(&versionGLSL);
    int version = versionGLSL.getVersion();
    // We need to write version directive only if it is greater than 110.
    // If there is no version directive in the shader, 110 is implied.
    if (version > 110) {
        sink << "#version " << version << "\n";
    }
}

TranslatorGLSL::TranslatorGLSL(ShShaderType type, ShShaderSpec spec)
    : TCompiler(type, spec) {
}

void TranslatorGLSL::translate(TIntermNode* root) {
    TInfoSinkBase& sink = getInfoSink().obj;

    // Write GLSL version.
    writeVersion(getShaderType(), root, sink);

    // Write emulated built-in functions if needed.
    getBuiltInFunctionEmulator().OutputEmulatedFunctionDefinition(
        sink, false);

    // Write array bounds clamping emulation if needed.
    getArrayBoundsClamper().OutputClampingFunctionDefinition(sink);

    // Write translated shader.
    TOutputGLSL outputGLSL(sink, getArrayIndexClampingStrategy(), getHashFunction(), getNameMap(), getSymbolTable());
    root->traverse(&outputGLSL);
}
