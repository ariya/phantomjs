//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/osinclude.h"
//
// This file contains contains Windows Runtime specific functions
//

#if !defined(ANGLE_OS_WINRT)
#error Trying to build a WinRT specific file in a non-WinRT build.
#endif

#include <vector>


//
// Thread Local Storage Operations
//
__declspec(thread) std::vector<void *> *tls = nullptr;
__declspec(thread) std::vector<OS_TLSIndex> *freeIndices = nullptr;

OS_TLSIndex OS_AllocTLSIndex()
{
    if (!tls)
        tls = new std::vector<void*>;

    if (freeIndices && !freeIndices->empty()) {
        OS_TLSIndex index = freeIndices->back();
        freeIndices->pop_back();
        return index;
    } else {
        tls->push_back(nullptr);
        return tls->size() - 1;
    }
}


void *OS_GetTLSValue(OS_TLSIndex nIndex)
{
    ASSERT(nIndex != OS_INVALID_TLS_INDEX);
    ASSERT(tls);

    return tls->at(nIndex);
}


bool OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue)
{
    if (!tls || nIndex >= tls->size() || nIndex == OS_INVALID_TLS_INDEX) {
        ASSERT(0 && "OS_SetTLSValue(): Invalid TLS Index");
        return false;
    }

    tls->at(nIndex) = lpvValue;
    return true;
}


bool OS_FreeTLSIndex(OS_TLSIndex nIndex)
{
    if (!tls || nIndex >= tls->size() || nIndex == OS_INVALID_TLS_INDEX) {
        ASSERT(0 && "OS_SetTLSValue(): Invalid TLS Index");
        return false;
    }

    if (!freeIndices)
        freeIndices = new std::vector<OS_TLSIndex>;

    freeIndices->push_back(nIndex);

    return true;
}
