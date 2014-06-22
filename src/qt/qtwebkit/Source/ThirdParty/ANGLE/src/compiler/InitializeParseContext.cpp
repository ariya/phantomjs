//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/InitializeParseContext.h"

#include "compiler/osinclude.h"

OS_TLSIndex GlobalParseContextIndex = OS_INVALID_TLS_INDEX;

bool InitializeParseContextIndex()
{
    if (GlobalParseContextIndex != OS_INVALID_TLS_INDEX) {
        assert(0 && "InitializeParseContextIndex(): Parse Context already initalized");
        return false;
    }

    //
    // Allocate a TLS index.
    //
    GlobalParseContextIndex = OS_AllocTLSIndex();
    
    if (GlobalParseContextIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "InitializeParseContextIndex(): Parse Context already initalized");
        return false;
    }

    return true;
}

bool FreeParseContextIndex()
{
    OS_TLSIndex tlsiIndex = GlobalParseContextIndex;

    if (GlobalParseContextIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "FreeParseContextIndex(): Parse Context index not initalized");
        return false;
    }

    GlobalParseContextIndex = OS_INVALID_TLS_INDEX;

    return OS_FreeTLSIndex(tlsiIndex);
}

bool InitializeGlobalParseContext()
{
    if (GlobalParseContextIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "InitializeGlobalParseContext(): Parse Context index not initalized");
        return false;
    }

    TThreadParseContext *lpParseContext = static_cast<TThreadParseContext *>(OS_GetTLSValue(GlobalParseContextIndex));
    if (lpParseContext != 0) {
        assert(0 && "InitializeParseContextIndex(): Parse Context already initalized");
        return false;
    }

    TThreadParseContext *lpThreadData = new TThreadParseContext();
    if (lpThreadData == 0) {
        assert(0 && "InitializeGlobalParseContext(): Unable to create thread parse context");
        return false;
    }

    lpThreadData->lpGlobalParseContext = 0;
    OS_SetTLSValue(GlobalParseContextIndex, lpThreadData);

    return true;
}

bool FreeParseContext()
{
    if (GlobalParseContextIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "FreeParseContext(): Parse Context index not initalized");
        return false;
    }

    TThreadParseContext *lpParseContext = static_cast<TThreadParseContext *>(OS_GetTLSValue(GlobalParseContextIndex));
    if (lpParseContext)
        delete lpParseContext;

    return true;
}

TParseContextPointer& GetGlobalParseContext()
{
    //
    // Minimal error checking for speed
    //

    TThreadParseContext *lpParseContext = static_cast<TThreadParseContext *>(OS_GetTLSValue(GlobalParseContextIndex));

    return lpParseContext->lpGlobalParseContext;
}

