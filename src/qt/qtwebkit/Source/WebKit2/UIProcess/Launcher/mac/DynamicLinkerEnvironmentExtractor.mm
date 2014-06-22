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
#include "DynamicLinkerEnvironmentExtractor.h"

#include "EnvironmentVariables.h"
#include <mach-o/loader.h>
#include <mach-o/swap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

DynamicLinkerEnvironmentExtractor::DynamicLinkerEnvironmentExtractor(NSString *executablePath, cpu_type_t architecture)
    : m_executablePath(executablePath)
    , m_architecture(architecture)
{
    NSData *mainExecutableData = [NSData dataWithContentsOfFile:m_executablePath.get() options:NSDataReadingMappedIfSafe error:0];
    if (!mainExecutableData)
        return;

    const void* mainExecutableBytes = [mainExecutableData bytes];
    size_t length = [mainExecutableData length];
    if (length < sizeof(uint32_t))
        return;

    uint32_t magicValue = *static_cast<const uint32_t*>(mainExecutableBytes);
    if (magicValue == FAT_MAGIC || magicValue == FAT_CIGAM) {
        processFatFile(mainExecutableBytes, length);
        return;
    }

    processSingleArchitecture(mainExecutableBytes, length);
}

#define DEFINE_BYTE_SWAPPER(type) inline type byteSwapIfNeeded(const type& data, bool shouldByteSwap) \
{ \
    type swapped = data; \
    if (shouldByteSwap) \
        swap_##type(&swapped, NX_UnknownByteOrder); \
    return swapped; \
}

DEFINE_BYTE_SWAPPER(load_command)
DEFINE_BYTE_SWAPPER(dylinker_command)
DEFINE_BYTE_SWAPPER(mach_header)
DEFINE_BYTE_SWAPPER(mach_header_64)

#undef DEFINE_BYTE_SWAPPER

void DynamicLinkerEnvironmentExtractor::processEnvironmentVariable(const char* environmentString)
{
    const char* equalsLocation = strchr(environmentString, '=');
    if (!equalsLocation)
        return;

    size_t nameLength = equalsLocation - environmentString;
    String name(environmentString, nameLength);

    // LC_DYLD_ENVIRONMENT only respects DYLD_*_PATH variables.
    if (!name.startsWith("DYLD_") || !name.endsWith("_PATH"))
        return;

    CString value(equalsLocation + 1);
    m_extractedVariables.append(std::make_pair(name.latin1(), value));
}

size_t DynamicLinkerEnvironmentExtractor::processLoadCommand(const void* data, size_t length, bool shouldByteSwap)
{
    if (length < sizeof(load_command))
        return 0;

    const load_command* rawLoadCommand = static_cast<const load_command*>(data);
    load_command loadCommand = byteSwapIfNeeded(*rawLoadCommand, shouldByteSwap);

    if (length < loadCommand.cmdsize)
        return 0;

    if (loadCommand.cmd == LC_DYLD_ENVIRONMENT) {
        if (length < sizeof(dylinker_command))
            return 0;

        dylinker_command environmentCommand = byteSwapIfNeeded(*reinterpret_cast<const dylinker_command*>(rawLoadCommand), shouldByteSwap);
        if (loadCommand.cmdsize < environmentCommand.name.offset)
            return 0;

        size_t environmentStringLength = loadCommand.cmdsize - environmentCommand.name.offset;
        Vector<char, 256> environmentString;
        environmentString.reserveCapacity(environmentStringLength + 1);
        environmentString.append(reinterpret_cast<const char*>(rawLoadCommand) + environmentCommand.name.offset, environmentStringLength);
        environmentString.append(0);

        processEnvironmentVariable(environmentString.data());
    }

    return loadCommand.cmdsize;
}

void DynamicLinkerEnvironmentExtractor::processLoadCommands(const void* data, size_t length, int32_t numberOfCommands, bool shouldByteSwap)
{
    const void* dataRemaining = data;
    size_t lengthRemaining = length;
    for (int i = 0; i < numberOfCommands; i++) {
        size_t commandLength = processLoadCommand(dataRemaining, lengthRemaining, shouldByteSwap);
        if (!commandLength || lengthRemaining < commandLength)
            return;

        dataRemaining = static_cast<const char*>(dataRemaining) + commandLength;
        lengthRemaining -= commandLength;
    }
}

void DynamicLinkerEnvironmentExtractor::processSingleArchitecture(const void* data, size_t length)
{
    if (length < sizeof(mach_header))
        return;

    const mach_header* header = static_cast<const mach_header*>(data);
    if (header->magic == MH_MAGIC || header->magic == MH_CIGAM) {
        bool shouldByteSwap = header->magic == MH_CIGAM;
        mach_header swappedHeader = byteSwapIfNeeded(*header, shouldByteSwap);
        if (swappedHeader.cputype == m_architecture)
            processLoadCommands(static_cast<const char*>(data) + sizeof(*header), length - sizeof(*header), swappedHeader.ncmds, shouldByteSwap);
        return;
    }

    if (length < sizeof(mach_header_64))
        return;

    const mach_header_64* header64 = static_cast<const mach_header_64*>(data);
    bool shouldByteSwap = header->magic == MH_CIGAM_64;
    mach_header_64 swappedHeader64 = byteSwapIfNeeded(*header64, shouldByteSwap);
    if (swappedHeader64.cputype == m_architecture)
        processLoadCommands(static_cast<const char*>(data) + sizeof(*header64), length - sizeof(*header64), swappedHeader64.ncmds, shouldByteSwap);
}

void DynamicLinkerEnvironmentExtractor::processFatFile(const void* data, size_t length)
{
    if (length < sizeof(fat_header))
        return;

    const fat_header* header = static_cast<const fat_header*>(data);

    size_t numberOfArchitectures = OSSwapBigToHostInt32(header->nfat_arch);

    // Ensure that we have enough data remaining for numberOfArchitectures fat_arch structs.
    if ((length - sizeof(fat_header)) / sizeof(fat_arch) < numberOfArchitectures)
        return;

    const fat_arch* archs = reinterpret_cast<const fat_arch*>(reinterpret_cast<const char*>(data) + sizeof(*header));
    for (uint32_t i = 0; i < numberOfArchitectures; i++) {
        uint32_t architectureOffset = OSSwapBigToHostInt32(archs[i].offset);
        uint32_t architectureSize = OSSwapBigToHostInt32(archs[i].size);
        if (length < architectureOffset + architectureSize)
            return;

        processSingleArchitecture(static_cast<const char*>(data) + architectureOffset, architectureSize);
    }
}

void DynamicLinkerEnvironmentExtractor::getExtractedEnvironmentVariables(EnvironmentVariables& environmentVariables) const
{
    size_t extractedVariableCount = m_extractedVariables.size();
    for (size_t i = 0; i < extractedVariableCount; ++i) {
        const CString& name = m_extractedVariables[i].first;

        // Preserve any existing environment variable by this name so that it will take
        // precedence over what we extracted from the executable file.
        if (environmentVariables.get(name.data()))
            continue;

        environmentVariables.set(name.data(), m_extractedVariables[i].second.data());
    }
}

} // namespace WebKit
