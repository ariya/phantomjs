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

#include "config.h"
#include "Storage.h"

#if ENABLE(DOM_STORAGE)

#include "Frame.h"
#include "Page.h"
#include "Settings.h"
#include "StorageArea.h"
#include "PlatformString.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

PassRefPtr<Storage> Storage::create(Frame* frame, PassRefPtr<StorageArea> storageArea)
{
    return adoptRef(new Storage(frame, storageArea));
}

Storage::Storage(Frame* frame, PassRefPtr<StorageArea> storageArea)
    : m_frame(frame)
    , m_storageArea(storageArea)
{
    ASSERT(m_frame);
    ASSERT(m_storageArea);
}

Storage::~Storage()
{
}

unsigned Storage::length() const
{
    if (!m_frame || !m_frame->page() || m_frame->page()->settings()->privateBrowsingEnabled())
        return 0;

    return m_storageArea->length();
}

String Storage::key(unsigned index) const
{
    if (!m_frame || !m_frame->page() || m_frame->page()->settings()->privateBrowsingEnabled())
        return String();

    return m_storageArea->key(index);
}

String Storage::getItem(const String& key) const
{
    if (!m_frame || !m_frame->page() || m_frame->page()->settings()->privateBrowsingEnabled())
        return String();

    return m_storageArea->getItem(key);
}

void Storage::setItem(const String& key, const String& value, ExceptionCode& ec)
{
    ec = 0;
    if (!m_frame)
        return;

    m_storageArea->setItem(key, value, ec, m_frame);
}

void Storage::removeItem(const String& key)
{
    if (!m_frame)
        return;

    m_storageArea->removeItem(key, m_frame);
}

void Storage::clear()
{
    if (!m_frame)
        return;

    m_storageArea->clear(m_frame);
}

bool Storage::contains(const String& key) const
{
    if (!m_frame || !m_frame->page() || m_frame->page()->settings()->privateBrowsingEnabled())
        return false;

    return m_storageArea->contains(key);
}

}

#endif // ENABLE(DOM_STORAGE)
