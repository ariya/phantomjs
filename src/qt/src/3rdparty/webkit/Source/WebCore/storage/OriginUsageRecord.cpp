/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
#include "OriginUsageRecord.h"

#if ENABLE(DATABASE)

#include "SQLiteFileSystem.h"

namespace WebCore {

OriginUsageRecord::OriginUsageRecord()
    : m_cachedDiskUsageIsValid(false)
{
}

void OriginUsageRecord::addDatabase(const String& identifier, const String& fullPath)
{
    ASSERT(!m_databaseMap.contains(identifier));
    ASSERT_ARG(identifier, identifier.impl()->hasOneRef() || identifier.isEmpty());
    ASSERT_ARG(fullPath, fullPath.impl()->hasOneRef() || fullPath.isEmpty());

    m_databaseMap.set(identifier, DatabaseEntry(fullPath));
    m_unknownSet.add(identifier);

    m_cachedDiskUsageIsValid = false;
}

void OriginUsageRecord::removeDatabase(const String& identifier)
{
    ASSERT(m_databaseMap.contains(identifier));

    m_databaseMap.remove(identifier);
    m_unknownSet.remove(identifier);
    m_cachedDiskUsageIsValid = false;
}

void OriginUsageRecord::markDatabase(const String& identifier)
{
    ASSERT(m_databaseMap.contains(identifier));
    ASSERT_ARG(identifier, identifier.impl()->hasOneRef() || identifier.isEmpty());

    m_unknownSet.add(identifier);
    m_cachedDiskUsageIsValid = false;
}

unsigned long long OriginUsageRecord::diskUsage()
{
    // Use the last cached usage value if we have it.
    if (m_cachedDiskUsageIsValid)
        return m_cachedDiskUsage;

    // stat() for the sizes known to be dirty.
    HashSet<String>::iterator iUnknown = m_unknownSet.begin();
    HashSet<String>::iterator endUnknown = m_unknownSet.end();
    for (; iUnknown != endUnknown; ++iUnknown) {
        const String& path = m_databaseMap.get(*iUnknown).filename;
        ASSERT(!path.isEmpty());

        // When we can't determine the file size, we'll just have to assume the file is missing/inaccessible.
        long long size = SQLiteFileSystem::getDatabaseFileSize(path);
        m_databaseMap.set(*iUnknown, DatabaseEntry(path, size));
    }
    m_unknownSet.clear();

    // Recalculate the cached usage value.
    m_cachedDiskUsage = 0;
    HashMap<String, DatabaseEntry>::iterator iDatabase = m_databaseMap.begin();
    HashMap<String, DatabaseEntry>::iterator endDatabase = m_databaseMap.end();
    for (; iDatabase != endDatabase; ++iDatabase)
        m_cachedDiskUsage += iDatabase->second.size;

    m_cachedDiskUsageIsValid = true;
    return m_cachedDiskUsage;
}

}

#endif
