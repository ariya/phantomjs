//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/InitializeParseContext.h"

#include "compiler/translator/osinclude.h"

OS_TLSIndex GlobalParseContextIndex = OS_INVALID_TLS_INDEX;

bool InitializeParseContextIndex()
{
    assert(GlobalParseContextIndex == OS_INVALID_TLS_INDEX);

    GlobalParseContextIndex = OS_AllocTLSIndex();
    return GlobalParseContextIndex != OS_INVALID_TLS_INDEX;
}

void FreeParseContextIndex()
{
    assert(GlobalParseContextIndex != OS_INVALID_TLS_INDEX);

    OS_FreeTLSIndex(GlobalParseContextIndex);
    GlobalParseContextIndex = OS_INVALID_TLS_INDEX;
}

void SetGlobalParseContext(TParseContext* context)
{
    assert(GlobalParseContextIndex != OS_INVALID_TLS_INDEX);
    OS_SetTLSValue(GlobalParseContextIndex, context);
}

TParseContext* GetGlobalParseContext()
{
    assert(GlobalParseContextIndex != OS_INVALID_TLS_INDEX);
    return static_cast<TParseContext*>(OS_GetTLSValue(GlobalParseContextIndex));
}

