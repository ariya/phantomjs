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

#ifndef LocalStorageTask_h
#define LocalStorageTask_h

#if ENABLE(DOM_STORAGE)

#include "PlatformString.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class StorageAreaSync;
    class LocalStorageThread;

    // FIXME: Rename this class to StorageTask
    class LocalStorageTask {
        WTF_MAKE_NONCOPYABLE(LocalStorageTask); WTF_MAKE_FAST_ALLOCATED;
    public:
        enum Type { AreaImport, AreaSync, DeleteEmptyDatabase, SetOriginDetails, ImportOrigins, DeleteAllOrigins, DeleteOrigin, TerminateThread };

        ~LocalStorageTask();

        static PassOwnPtr<LocalStorageTask> createImport(StorageAreaSync* area) { return adoptPtr(new LocalStorageTask(AreaImport, area)); }
        static PassOwnPtr<LocalStorageTask> createSync(StorageAreaSync* area) { return adoptPtr(new LocalStorageTask(AreaSync, area)); }
        static PassOwnPtr<LocalStorageTask> createDeleteEmptyDatabase(StorageAreaSync* area) { return adoptPtr(new LocalStorageTask(DeleteEmptyDatabase, area)); }
        static PassOwnPtr<LocalStorageTask> createOriginIdentifiersImport() { return adoptPtr(new LocalStorageTask(ImportOrigins)); }
        static PassOwnPtr<LocalStorageTask> createSetOriginDetails(const String& originIdentifier, const String& databaseFilename) { return adoptPtr(new LocalStorageTask(SetOriginDetails, originIdentifier, databaseFilename)); }
        static PassOwnPtr<LocalStorageTask> createDeleteOrigin(const String& originIdentifier) { return adoptPtr(new LocalStorageTask(DeleteOrigin, originIdentifier)); }
        static PassOwnPtr<LocalStorageTask> createDeleteAllOrigins() { return adoptPtr(new LocalStorageTask(DeleteAllOrigins)); }
        static PassOwnPtr<LocalStorageTask> createTerminate(LocalStorageThread* thread) { return adoptPtr(new LocalStorageTask(TerminateThread, thread)); }

        void performTask();

    private:
        LocalStorageTask(Type, StorageAreaSync*);
        LocalStorageTask(Type, LocalStorageThread*);
        LocalStorageTask(Type, const String& originIdentifier);
        LocalStorageTask(Type, const String& originIdentifier, const String& databaseFilename);
        LocalStorageTask(Type);

        Type m_type;
        StorageAreaSync* m_area;
        LocalStorageThread* m_thread;
        String m_originIdentifier;
        String m_databaseFilename;
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // LocalStorageTask_h
