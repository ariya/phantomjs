//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/TranslatorESSL.h"

#include "compiler/OutputESSL.h"

TranslatorESSL::TranslatorESSL(ShShaderType type, ShShaderSpec spec)
    : TCompiler(type, spec) {
}

void TranslatorESSL::translate(TIntermNode* root) {
    TInfoSinkBase& sink = getInfoSink().obj;

    // Write built-in extension behaviors.
    writeExtensionBehavior();

    // Write emulated built-in functions if needed.
    getBuiltInFunctionEmulator().OutputEmulatedFunctionDefinition(
        sink, getShaderType() == SH_FRAGMENT_SHADER);

    // Write array bounds clamping emulation if needed.
    getArrayBoundsClamper().OutputClampingFunctionDefinition(sink);

    // Write translated shader.
    TOutputESSL outputESSL(sink, getArrayIndexClampingStrategy(), getHashFunction(), getNameMap(), getSymbolTable());
    root->traverse(&outputESSL);
}

void TranslatorESSL::writeExtensionBehavior() {
    TInfoSinkBase& sink = getInfoSink().obj;
    const TExtensionBehavior& extensionBehavior = getExtensionBehavior();
    for (TExtensionBehavior::const_iterator iter = extensionBehavior.begin();
         iter != extensionBehavior.end(); ++iter) {
        if (iter->second != EBhUndefined) {
            sink << "#extension " << iter->first << " : "
                 << getBehaviorString(iter->second) << "\n";
        }
    }
}
