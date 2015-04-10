/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef ImportedFunctionsEnumerator_h
#define ImportedFunctionsEnumerator_h

#include "PEImage.h"

namespace WebCore {

// Enumerates the functions from a single module imported by the given PEImage.
class ImportedFunctionsEnumerator {
public:
    explicit ImportedFunctionsEnumerator(const PEImage&, const IMAGE_THUNK_DATA* importNameTable, const IMAGE_THUNK_DATA* importAddressTable);

    bool isAtEnd() const;
    void next();

    const char* currentFunctionName() const;
    const void* const* addressOfCurrentFunctionPointer() const;

private:
    const DWORD& currentFunctionAddress() const;

    PEImage m_image;

    // These point to corresponding entries in the Import Name Table (INT) and Import Address Table
    // (IAT) for a particular module. The INT and IAT are parallel arrays that are terminated by an
    // all-0 entry.
    const IMAGE_THUNK_DATA* m_nameTableEntry;
    const IMAGE_THUNK_DATA* m_addressTableEntry;
};

} // namespace WebCore

#endif // ImportedFunctionsEnumerator_h
