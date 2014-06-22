/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef VisitedLinkTable_h
#define VisitedLinkTable_h

#include <WebCore/LinkHash.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebKit {

class SharedMemory;

class VisitedLinkTable {
public:
    VisitedLinkTable();
    ~VisitedLinkTable();

    void setSharedMemory(PassRefPtr<SharedMemory>);

    // This should only be called from the UI process.
    bool addLinkHash(WebCore::LinkHash);

    bool isLinkVisited(WebCore::LinkHash) const;

    SharedMemory* sharedMemory() const { return m_sharedMemory.get(); }

private:
    RefPtr<SharedMemory> m_sharedMemory;

    unsigned m_tableSize;
    unsigned m_tableSizeMask;
    WebCore::LinkHash* m_table;
};

}

#endif // VisitedLinkTable_h
