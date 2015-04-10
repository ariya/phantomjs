/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SyncCallbackHelper_h
#define SyncCallbackHelper_h

#if ENABLE(FILE_SYSTEM)

#include "DirectoryEntry.h"
#include "EntriesCallback.h"
#include "EntryArraySync.h"
#include "EntryCallback.h"
#include "ErrorCallback.h"
#include "FileEntry.h"
#include "FileError.h"
#include "FileException.h"
#include "FileSystemCallback.h"
#include "MetadataCallback.h"
#include "VoidCallback.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class AsyncFileSystem;
class DirectoryEntrySync;
class EntryArraySync;
class EntrySync;
class FileEntrySync;

typedef int ExceptionCode;

// A helper template for FileSystemSync implementation.
template <typename SuccessCallback, typename ObserverType, typename CallbackArg, typename ResultType>
class SyncCallbackHelper {
    WTF_MAKE_NONCOPYABLE(SyncCallbackHelper);
public:
    typedef SyncCallbackHelper<SuccessCallback, ObserverType, CallbackArg, ResultType> HelperType;
    SyncCallbackHelper(ObserverType* observer = 0)
        : m_observer(observer)
        , m_successCallback(SuccessCallbackImpl::create(this))
        , m_errorCallback(ErrorCallbackImpl::create(this))
        , m_exceptionCode(0)
        , m_completed(false)
    {
    }

    PassRefPtr<ResultType> getResult(ExceptionCode& ec)
    {
        if (m_observer) {
            while (!m_completed) {
                if (!m_observer->waitForOperationToComplete()) {
                    m_exceptionCode = FileException::ABORT_ERR;
                    break;
                }
            }
        }
        ec = m_exceptionCode;
        return m_result.release();
    }

    PassRefPtr<SuccessCallback> successCallback() { return m_successCallback; }
    PassRefPtr<ErrorCallback> errorCallback() { return m_errorCallback; }

private:
    class SuccessCallbackImpl : public SuccessCallback {
    public:
        static PassRefPtr<SuccessCallbackImpl> create(HelperType* helper)
        {
            return adoptRef(new SuccessCallbackImpl(helper));
        }

        virtual bool handleEvent()
        {
            m_helper->setError(0);
            return true;
        }

        virtual bool handleEvent(CallbackArg* arg)
        {
            m_helper->setResult(ResultType::create(arg));
            return true;
        }

    private:
        explicit SuccessCallbackImpl(HelperType* helper)
            : m_helper(helper)
        {
        }
        HelperType* m_helper;
    };

    class ErrorCallbackImpl : public ErrorCallback {
    public:
        static PassRefPtr<ErrorCallbackImpl> create(HelperType* helper)
        {
            return adoptRef(new ErrorCallbackImpl(helper));
        }

        virtual bool handleEvent(FileError* error)
        {
            ASSERT(error);
            m_helper->setError(error->code());
            return true;
        }

    private:
        explicit ErrorCallbackImpl(HelperType* helper)
            : m_helper(helper)
        {
        }
        HelperType* m_helper;
    };

    friend class SuccessCallbackImpl;
    friend class ErrorCallbackImpl;

    void setError(int code)
    {
        m_exceptionCode = FileException::ErrorCodeToExceptionCode(code);
        m_completed = true;
    }

    void setResult(PassRefPtr<ResultType> result)
    {
        m_result = result;
        m_completed = true;
    }

    ObserverType* m_observer;
    RefPtr<SuccessCallbackImpl> m_successCallback;
    RefPtr<ErrorCallbackImpl> m_errorCallback;
    RefPtr<ResultType> m_result;
    ExceptionCode m_exceptionCode;
    bool m_completed;
};

struct EmptyType : public RefCounted<EmptyType> {
    static PassRefPtr<EmptyType> create(EmptyType*)
    {
        return 0;
    }
};

struct EmptyObserverType {
    bool waitForOperationToComplete()
    {
        return false;
    }
};

typedef SyncCallbackHelper<EntryCallback, AsyncFileSystem, Entry, EntrySync> EntrySyncCallbackHelper;
typedef SyncCallbackHelper<EntriesCallback, AsyncFileSystem, EntryArray, EntryArraySync> EntriesSyncCallbackHelper;
typedef SyncCallbackHelper<MetadataCallback, AsyncFileSystem, Metadata, Metadata> MetadataSyncCallbackHelper;
typedef SyncCallbackHelper<VoidCallback, AsyncFileSystem, EmptyType, EmptyType> VoidSyncCallbackHelper;
typedef SyncCallbackHelper<FileSystemCallback, EmptyObserverType, DOMFileSystem, DOMFileSystemSync> FileSystemSyncCallbackHelper;

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // SyncCallbackHelper_h
