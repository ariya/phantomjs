/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StorageNamespace_h
#define StorageNamespace_h

#if ENABLE(DOM_STORAGE)

#include "PlatformString.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class Page;
class SecurityOrigin;
class StorageArea;

// This interface is required for Chromium since these actions need to be proxied between processes.
class StorageNamespace : public RefCounted<StorageNamespace> {
public:
    static PassRefPtr<StorageNamespace> localStorageNamespace(const String& path, unsigned quota);
    static PassRefPtr<StorageNamespace> sessionStorageNamespace(Page*, unsigned quota);

    virtual ~StorageNamespace() { }
    virtual PassRefPtr<StorageArea> storageArea(PassRefPtr<SecurityOrigin>) = 0;
    virtual PassRefPtr<StorageNamespace> copy() = 0;
    virtual void close() = 0;
    virtual void unlock() = 0;
    virtual void clearOriginForDeletion(SecurityOrigin*) = 0;
    virtual void clearAllOriginsForDeletion() = 0;
    virtual void sync() = 0;
};

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // StorageNamespace_h
