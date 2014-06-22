/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebKitCOMAPI.h"

#include "WebKit.h"
#include "WebKitDLL.h"

#include <WebCore/COMPtr.h>

struct CLSIDHash {
    static unsigned hash(const CLSID& clsid)
    {
        RPC_STATUS status;
        return ::UuidHash(const_cast<CLSID*>(&clsid), &status);
    }
    static bool equal(const CLSID& a, const CLSID& b) { return ::IsEqualCLSID(a, b); }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct CLSIDHashTraits : WTF::GenericHashTraits<CLSID> {
    static void constructDeletedValue(CLSID& slot) { slot = CLSID_NULL; }
    static bool isDeletedValue(const CLSID& value) { return value == CLSID_NULL; }
    // FIXME: This is a work around for the regression introducing in r130643 when running DRT in debug mode on Windows.
    // Expanding the size of the hash table causes an assertion failure ASSERT(!isDeletedBucket) when reinserting items into the new table,
    // presumably due to the collision issues described in r132302.
    // This work around avoids the issue entirely simply by making sure that the table will not have to resize in running DRT. 
    static const int minimumTableSize = 64;
};

static COMPtr<IClassFactory> classFactory(const CLSID& clsid)
{
    typedef HashMap<CLSID, COMPtr<IClassFactory>, CLSIDHash, CLSIDHashTraits> FactoryMap;
    static FactoryMap& factories = *new FactoryMap;

    FactoryMap::AddResult result = factories.add(clsid, 0);
    COMPtr<IClassFactory>& factory = result.iterator->value;
    if (result.isNewEntry && FAILED(DllGetClassObject(clsid, __uuidof(factory), reinterpret_cast<void**>(&factory))))
        factory = 0;

    return factory;
}

HRESULT WebKitCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
    COMPtr<IClassFactory> factory = classFactory(rclsid);
    if (!factory)
        return REGDB_E_CLASSNOTREG;

    return factory->CreateInstance(pUnkOuter, riid, ppvObject);
}
