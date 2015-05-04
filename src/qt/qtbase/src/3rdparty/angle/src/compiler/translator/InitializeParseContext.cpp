//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/InitializeParseContext.h"

#include "common/tls.h"

#include <assert.h>

TLSIndex GlobalParseContextIndex = TLS_INVALID_INDEX;

bool InitializeParseContextIndex()
{
    assert(GlobalParseContextIndex == TLS_INVALID_INDEX);

    GlobalParseContextIndex = CreateTLSIndex();
    return GlobalParseContextIndex != TLS_INVALID_INDEX;
}

void FreeParseContextIndex()
{
    assert(GlobalParseContextIndex != TLS_INVALID_INDEX);

    DestroyTLSIndex(GlobalParseContextIndex);
    GlobalParseContextIndex = TLS_INVALID_INDEX;
}

void SetGlobalParseContext(TParseContext* context)
{
    assert(GlobalParseContextIndex != TLS_INVALID_INDEX);
    SetTLSValue(GlobalParseContextIndex, context);
}

TParseContext* GetGlobalParseContext()
{
    assert(GlobalParseContextIndex != TLS_INVALID_INDEX);
    return static_cast<TParseContext*>(GetTLSValue(GlobalParseContextIndex));
}

