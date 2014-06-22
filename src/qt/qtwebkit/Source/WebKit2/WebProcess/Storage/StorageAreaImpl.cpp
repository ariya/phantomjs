/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StorageAreaImpl.h"

#include "StorageAreaMap.h"
#include <WebCore/Document.h>
#include <WebCore/ExceptionCode.h>
#include <WebCore/Frame.h>
#include <WebCore/Page.h>
#include <WebCore/SchemeRegistry.h>
#include <WebCore/Settings.h>

using namespace WebCore;

namespace WebKit {

static uint64_t generateStorageAreaID()
{
    static uint64_t storageAreaID;
    return ++storageAreaID;
}

PassRefPtr<StorageAreaImpl> StorageAreaImpl::create(PassRefPtr<StorageAreaMap> storageAreaMap)
{
    return adoptRef(new StorageAreaImpl(storageAreaMap));
}

StorageAreaImpl::StorageAreaImpl(PassRefPtr<StorageAreaMap> storageAreaMap)
    : m_storageAreaID(generateStorageAreaID())
    , m_storageAreaMap(storageAreaMap)
{
}

StorageAreaImpl::~StorageAreaImpl()
{
}

unsigned StorageAreaImpl::length()
{
    return m_storageAreaMap->length();
}

String StorageAreaImpl::key(unsigned index)
{
    return m_storageAreaMap->key(index);
}

String StorageAreaImpl::item(const String& key)
{
    return m_storageAreaMap->item(key);
}

void StorageAreaImpl::setItem(Frame* sourceFrame, const String& key, const String& value, bool& quotaException)
{
    ASSERT(!value.isNull());

    m_storageAreaMap->setItem(sourceFrame, this, key, value, quotaException);
}

void StorageAreaImpl::removeItem(Frame* sourceFrame, const String& key)
{
    m_storageAreaMap->removeItem(sourceFrame, this, key);
}

void StorageAreaImpl::clear(Frame* sourceFrame)
{
    m_storageAreaMap->clear(sourceFrame, this);
}

bool StorageAreaImpl::contains(const String& key)
{
    return m_storageAreaMap->contains(key);
}

bool StorageAreaImpl::canAccessStorage(Frame* frame)
{
    return frame && frame->page();
}

StorageType StorageAreaImpl::storageType() const
{
    return m_storageAreaMap->storageType();
}

size_t StorageAreaImpl::memoryBytesUsedByCache()
{
    return 0;
}

void StorageAreaImpl::incrementAccessCount()
{
    // Storage access is handled in the UI process, so there's nothing to do here.
}

void StorageAreaImpl::decrementAccessCount()
{
    // Storage access is handled in the UI process, so there's nothing to do here.
}

void StorageAreaImpl::closeDatabaseIfIdle()
{
    // FIXME: Implement this.
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
