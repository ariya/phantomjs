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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
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

#ifndef DOMWindowFileSystem_h
#define DOMWindowFileSystem_h

#if ENABLE(FILE_SYSTEM)

#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DOMWindow;
class EntryCallback;
class ErrorCallback;
class FileSystemCallback;

class DOMWindowFileSystem {
public:

    static void webkitRequestFileSystem(DOMWindow*, int type, long long size, PassRefPtr<FileSystemCallback>, PassRefPtr<ErrorCallback>);
    static void webkitResolveLocalFileSystemURL(DOMWindow*, const String&, PassRefPtr<EntryCallback>, PassRefPtr<ErrorCallback>);

    // They are placed here and in all capital letters so they can be checked against the constants in the
    // IDL at compile time.
    enum {
        TEMPORARY,
        PERSISTENT,
    };

private:
    DOMWindowFileSystem();
    ~DOMWindowFileSystem();
};

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // DOMWindowFileSystem_h
