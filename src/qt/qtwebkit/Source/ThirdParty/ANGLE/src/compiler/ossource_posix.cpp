//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// This file contains the posix specific functions
//
#include "compiler/osinclude.h"

#if !defined(ANGLE_OS_POSIX)
#error Trying to build a posix specific file in a non-posix build.
#endif

//
// Thread Local Storage Operations
//
OS_TLSIndex OS_AllocTLSIndex()
{
    pthread_key_t pPoolIndex;

    //
    // Create global pool key.
    //
    if ((pthread_key_create(&pPoolIndex, NULL)) != 0) {
        assert(0 && "OS_AllocTLSIndex(): Unable to allocate Thread Local Storage");
        return false;
    }
    else {
        return pPoolIndex;
    }
}


bool OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue)
{
    if (nIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "OS_SetTLSValue(): Invalid TLS Index");
        return false;
    }

    if (pthread_setspecific(nIndex, lpvValue) == 0)
        return true;
    else
        return false;
}


bool OS_FreeTLSIndex(OS_TLSIndex nIndex)
{
    if (nIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "OS_SetTLSValue(): Invalid TLS Index");
        return false;
    }

    //
    // Delete the global pool key.
    //
    if (pthread_key_delete(nIndex) == 0)
        return true;
    else
        return false;
}
