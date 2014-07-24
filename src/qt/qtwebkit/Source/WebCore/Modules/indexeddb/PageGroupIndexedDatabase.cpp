/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "PageGroupIndexedDatabase.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBFactoryBackendInterface.h"
#include "PageGroup.h"

namespace WebCore {

PageGroupIndexedDatabase::PageGroupIndexedDatabase()
{
}

PageGroupIndexedDatabase::~PageGroupIndexedDatabase()
{
}

const char* PageGroupIndexedDatabase::supplementName()
{
    return "PageGroupIndexedDatabase";
}

PageGroupIndexedDatabase* PageGroupIndexedDatabase::from(PageGroup& group)
{
    PageGroupIndexedDatabase* supplement = static_cast<PageGroupIndexedDatabase*>(Supplement<PageGroup>::from(&group, supplementName()));
    if (!supplement) {
        supplement = new PageGroupIndexedDatabase();
        provideTo(&group, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

IDBFactoryBackendInterface* PageGroupIndexedDatabase::factoryBackend()
{
    // Do not add page setting based access control here since this object is shared by all pages in
    // the group and having per-page controls is misleading.
    if (!m_factoryBackend)
        m_factoryBackend = IDBFactoryBackendInterface::create();
    return m_factoryBackend.get();
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
