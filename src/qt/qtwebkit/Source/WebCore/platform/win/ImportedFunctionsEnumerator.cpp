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

#include "config.h"
#include "ImportedFunctionsEnumerator.h"

// See <http://msdn.microsoft.com/en-us/magazine/cc301808.aspx> and
// <http://msdn.microsoft.com/en-us/windows/hardware/gg463119> for more information about the PE
// image format.

namespace WebCore {

ImportedFunctionsEnumerator::ImportedFunctionsEnumerator(const PEImage& image, const IMAGE_THUNK_DATA* importNameTable, const IMAGE_THUNK_DATA* importAddressTable)
    : m_image(image)
    , m_nameTableEntry(importNameTable)
    , m_addressTableEntry(importAddressTable)
{
    ASSERT(!importNameTable == !importAddressTable);
}

bool ImportedFunctionsEnumerator::isAtEnd() const
{
    ASSERT(!m_nameTableEntry || !m_nameTableEntry->u1.AddressOfData == !m_addressTableEntry->u1.Function);
    return !m_nameTableEntry || !m_nameTableEntry->u1.AddressOfData;
}

void ImportedFunctionsEnumerator::next()
{
    ASSERT(!isAtEnd());
    ++m_nameTableEntry;
    ++m_addressTableEntry;
}

const char* ImportedFunctionsEnumerator::currentFunctionName() const
{
    ASSERT(m_nameTableEntry);

    // Ordinal imports have no name.
    if (IMAGE_SNAP_BY_ORDINAL(m_nameTableEntry->u1.Ordinal))
        return 0;

    const IMAGE_IMPORT_BY_NAME* importByName = static_cast<const IMAGE_IMPORT_BY_NAME*>(m_image.convertRVAToAddress(m_nameTableEntry->u1.AddressOfData));
    return reinterpret_cast<const char*>(importByName->Name);
}

const void* const* ImportedFunctionsEnumerator::addressOfCurrentFunctionPointer() const
{
    ASSERT(m_addressTableEntry);
    COMPILE_ASSERT(sizeof(void*) == sizeof(m_addressTableEntry->u1.Function), FunctionAddressSizeMatchesPointerSize);
    return reinterpret_cast<const void* const*>(&m_addressTableEntry->u1.Function);
}

} // namespace WebCore
