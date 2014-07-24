/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#include "config.h"
#include "StorageEvent.h"

#include "EventNames.h"
#include "Storage.h"

namespace WebCore {

StorageEventInit::StorageEventInit()
{
}

PassRefPtr<StorageEvent> StorageEvent::create()
{
    return adoptRef(new StorageEvent);
}

StorageEvent::StorageEvent()
{
}

StorageEvent::~StorageEvent()
{
}

PassRefPtr<StorageEvent> StorageEvent::create(const AtomicString& type, const String& key, const String& oldValue, const String& newValue, const String& url, Storage* storageArea)
{
    return adoptRef(new StorageEvent(type, key, oldValue, newValue, url, storageArea));
}

PassRefPtr<StorageEvent> StorageEvent::create(const AtomicString& type, const StorageEventInit& initializer)
{
    return adoptRef(new StorageEvent(type, initializer));
}

StorageEvent::StorageEvent(const AtomicString& type, const String& key, const String& oldValue, const String& newValue, const String& url, Storage* storageArea)
    : Event(type, false, false)
    , m_key(key)
    , m_oldValue(oldValue)
    , m_newValue(newValue)
    , m_url(url)
    , m_storageArea(storageArea)
{
}

StorageEvent::StorageEvent(const AtomicString& type, const StorageEventInit& initializer)
    : Event(type, initializer)
    , m_key(initializer.key)
    , m_oldValue(initializer.oldValue)
    , m_newValue(initializer.newValue)
    , m_url(initializer.url)
    , m_storageArea(initializer.storageArea)
{
}

void StorageEvent::initStorageEvent(const AtomicString& type, bool canBubble, bool cancelable, const String& key, const String& oldValue, const String& newValue, const String& url, Storage* storageArea)
{
    if (dispatched())
        return;

    initEvent(type, canBubble, cancelable);

    m_key = key;
    m_oldValue = oldValue;
    m_newValue = newValue;
    m_url = url;
    m_storageArea = storageArea;
}

const AtomicString& StorageEvent::interfaceName() const
{
    return eventNames().interfaceForStorageEvent;
}

} // namespace WebCore
