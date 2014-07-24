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
#include "PEImage.h"

// See <http://msdn.microsoft.com/en-us/magazine/cc301808.aspx> and
// <http://msdn.microsoft.com/en-us/windows/hardware/gg463119> for more information about the PE
// image format.

namespace WebCore {

PEImage::PEImage(HMODULE module)
    : m_module(module)
    , m_ntHeaders(0)
{
    const IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(m_module);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    const IMAGE_NT_HEADERS* ntHeaders = static_cast<const IMAGE_NT_HEADERS*>(convertRVAToAddress(dosHeader->e_lfanew));
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
        return;

    if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
        return;

    m_ntHeaders = ntHeaders;
}

const void* PEImage::convertRVAToAddress(DWORD rva) const
{
    return reinterpret_cast<unsigned char*>(m_module) + rva;
}

const void* PEImage::dataDirectoryEntryAddress(DWORD entryIndex) const
{
    if (!isValid())
        return 0;

    if (m_ntHeaders->OptionalHeader.NumberOfRvaAndSizes <= entryIndex)
        return 0;

    const IMAGE_DATA_DIRECTORY& directory = m_ntHeaders->OptionalHeader.DataDirectory[entryIndex];

    if (!directory.Size)
        return 0;

    return convertRVAToAddress(directory.VirtualAddress);
}

} // namespace WebCore
