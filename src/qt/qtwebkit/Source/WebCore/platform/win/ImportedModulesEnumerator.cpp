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
#include "ImportedModulesEnumerator.h"

#include "ImportedFunctionsEnumerator.h"

// See <http://msdn.microsoft.com/en-us/magazine/cc301808.aspx> and
// <http://msdn.microsoft.com/en-us/windows/hardware/gg463119> for more information about the PE
// image format.

namespace WebCore {

ImportedModulesEnumerator::ImportedModulesEnumerator(const PEImage& image)
    : m_image(image)
{
    if (m_image.isValid())
        m_descriptor = static_cast<const IMAGE_IMPORT_DESCRIPTOR*>(m_image.dataDirectoryEntryAddress(IMAGE_DIRECTORY_ENTRY_IMPORT));
    else
        m_descriptor = 0;
}

bool ImportedModulesEnumerator::isAtEnd() const
{
    return !m_descriptor || !m_descriptor->Characteristics;
}

void ImportedModulesEnumerator::next()
{
    ASSERT(!isAtEnd());
    ++m_descriptor;
}

const char* ImportedModulesEnumerator::currentModuleName() const
{
    ASSERT(m_descriptor);
    return static_cast<const char*>(m_image.convertRVAToAddress(m_descriptor->Name));
}

ImportedFunctionsEnumerator ImportedModulesEnumerator::functionsEnumerator() const
{
    ASSERT(m_descriptor);

    const IMAGE_THUNK_DATA* importNameTable = static_cast<const IMAGE_THUNK_DATA*>(m_image.convertRVAToAddress(m_descriptor->OriginalFirstThunk));
    const IMAGE_THUNK_DATA* importAddressTable = static_cast<const IMAGE_THUNK_DATA*>(m_image.convertRVAToAddress(m_descriptor->FirstThunk));

    return ImportedFunctionsEnumerator(m_image, importNameTable, importAddressTable);
}

} // namespace WebCore
