//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/timing/RestrictVertexShaderTiming.h"

void RestrictVertexShaderTiming::visitSymbol(TIntermSymbol* node)
{
    if (IsSampler(node->getBasicType())) {
        ++mNumErrors;
        mSink.message(EPrefixError,
                      node->getLine(),
                      "Samplers are not permitted in vertex shaders");
    }
}
