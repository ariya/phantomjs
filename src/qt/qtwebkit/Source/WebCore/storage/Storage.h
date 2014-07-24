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

#ifndef Storage_h
#define Storage_h

#include "DOMWindowProperty.h"
#include "ScriptWrappable.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Frame;
class StorageArea;

typedef int ExceptionCode;

class Storage : public ScriptWrappable, public RefCounted<Storage>, public DOMWindowProperty {
public:
    static PassRefPtr<Storage> create(Frame*, PassRefPtr<StorageArea>);
    ~Storage();

    unsigned length(ExceptionCode&) const;
    String key(unsigned index, ExceptionCode&) const;
    String getItem(const String& key, ExceptionCode&) const;
    void setItem(const String& key, const String& value, ExceptionCode&);
    void removeItem(const String& key, ExceptionCode&);
    void clear(ExceptionCode&);
    bool contains(const String& key, ExceptionCode&) const;

    StorageArea& area() const { return *m_storageArea; }

private:
    Storage(Frame*, PassRefPtr<StorageArea>);

    bool isDisabledByPrivateBrowsing() const;

    const RefPtr<StorageArea> m_storageArea;
};

} // namespace WebCore

#endif // Storage_h
