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
#ifndef OriginUsageRecord_h
#define OriginUsageRecord_h

#if ENABLE(DATABASE)

#include "PlatformString.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h> 
#include <wtf/text/StringHash.h>

namespace WebCore {

// Objects of this class can be used from multiple threads with external synchronization.
// String arguments are also supposed to be deeply copied by the caller when necessary.
class OriginUsageRecord {
    WTF_MAKE_NONCOPYABLE(OriginUsageRecord); WTF_MAKE_FAST_ALLOCATED;
public:
    OriginUsageRecord();

    void addDatabase(const String& identifier, const String& fullPath);
    void removeDatabase(const String& identifier);
    void markDatabase(const String& identifier); // Size may have changed, and will need to be recalculated.
    unsigned long long diskUsage();

private:
    struct DatabaseEntry {
        DatabaseEntry() { }
        DatabaseEntry(const String& filename) : filename(filename) { }
        DatabaseEntry(const String& filename, unsigned long long size) : filename(filename), size(size) { }
        String filename;
        unsigned long long size; // This member remains uninitialized until disk usage is calculated.
    };
    HashMap<String, DatabaseEntry> m_databaseMap;
    HashSet<String> m_unknownSet;

    unsigned long long m_cachedDiskUsage;
    bool m_cachedDiskUsageIsValid;
};

} // namespace WebCore

#endif

#endif
