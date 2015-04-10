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

#include "config.h"
#include "VTableSpectrum.h"

#include "JSObject.h"
#include "Structure.h"
#include <algorithm>
#include <stdio.h>
#include <wtf/Vector.h>

#if PLATFORM(MAC)
#include <dlfcn.h>
#endif

namespace JSC {

VTableSpectrum::VTableSpectrum()
{
}

VTableSpectrum::~VTableSpectrum()
{
}

void VTableSpectrum::countVPtr(void* vTablePointer)
{
    add(vTablePointer);
}

void VTableSpectrum::count(JSCell* cell)
{
    // FIXME: we need to change this class to count ClassInfos rather than vptrs
    UNUSED_PARAM(cell);
}

void VTableSpectrum::dump(FILE* output, const char* comment)
{
    fprintf(output, "%s:\n", comment);
    
    Vector<KeyAndCount> list = buildList();
    
    for (size_t index = list.size(); index-- > 0;) {
        KeyAndCount item = list.at(index);
#if PLATFORM(MAC)
        Dl_info info;
        if (dladdr(item.key, &info)) {
            char* findResult = strrchr(info.dli_fname, '/');
            const char* strippedFileName;
            
            if (findResult)
                strippedFileName = findResult + 1;
            else
                strippedFileName = info.dli_fname;
            
            fprintf(output, "    %s:%s(%p): %lu\n", strippedFileName, info.dli_sname, item.key, item.count);
            continue;
        }
#endif
        fprintf(output, "    %p: %lu\n", item.key, item.count);
    }
    
    fflush(output);
}

} // namespace JSC
