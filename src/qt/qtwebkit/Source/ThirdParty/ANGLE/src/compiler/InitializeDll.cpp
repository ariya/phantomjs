//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/InitializeDll.h"

#include "compiler/InitializeGlobals.h"
#include "compiler/InitializeParseContext.h"
#include "compiler/osinclude.h"

OS_TLSIndex ThreadInitializeIndex = OS_INVALID_TLS_INDEX;

bool InitProcess()
{
    if (ThreadInitializeIndex != OS_INVALID_TLS_INDEX) {
        //
        // Function is re-entrant.
        //
        return true;
    }

    ThreadInitializeIndex = OS_AllocTLSIndex();

    if (ThreadInitializeIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "InitProcess(): Failed to allocate TLS area for init flag");
        return false;
    }


    if (!InitializePoolIndex()) {
        assert(0 && "InitProcess(): Failed to initalize global pool");
        return false;
    }

    if (!InitializeParseContextIndex()) {
        assert(0 && "InitProcess(): Failed to initalize parse context");
        return false;
    }

    return InitThread();
}

bool DetachProcess()
{
    bool success = true;

    if (ThreadInitializeIndex == OS_INVALID_TLS_INDEX)
        return true;

    success = DetachThread();

    if (!FreeParseContextIndex())
        success = false;

    FreePoolIndex();

    OS_FreeTLSIndex(ThreadInitializeIndex);
    ThreadInitializeIndex = OS_INVALID_TLS_INDEX;

    return success;
}

bool InitThread()
{
    //
    // This function is re-entrant
    //
    if (ThreadInitializeIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "InitThread(): Process hasn't been initalised.");
        return false;
    }

    if (OS_GetTLSValue(ThreadInitializeIndex) != 0)
        return true;

    InitializeGlobalPools();

    if (!InitializeGlobalParseContext())
        return false;

    if (!OS_SetTLSValue(ThreadInitializeIndex, (void *)1)) {
        assert(0 && "InitThread(): Unable to set init flag.");
        return false;
    }

    return true;
}

bool DetachThread()
{
    bool success = true;

    if (ThreadInitializeIndex == OS_INVALID_TLS_INDEX)
        return true;

    //
    // Function is re-entrant and this thread may not have been initalised.
    //
    if (OS_GetTLSValue(ThreadInitializeIndex) != 0) {
        if (!OS_SetTLSValue(ThreadInitializeIndex, (void *)0)) {
            assert(0 && "DetachThread(): Unable to clear init flag.");
            success = false;
        }

        if (!FreeParseContext())
            success = false;

        FreeGlobalPools();
    }

    return success;
}

