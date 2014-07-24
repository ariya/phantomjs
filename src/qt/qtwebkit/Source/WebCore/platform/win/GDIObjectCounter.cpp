/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#ifndef NDEBUG

#include "GDIObjectCounter.h"

#include "Logging.h"
#include <wtf/text/CString.h>

#include <windows.h>

namespace WebCore {

GDIObjectCounter::GDIObjectCounter(const String& identifier)
{
    init(identifier);
}

GDIObjectCounter::GDIObjectCounter(const String& className, void* instance)
{
    init(String::format("%s (%p)", className.latin1().data(), instance));
}

void GDIObjectCounter::init(const String& identifier)
{
    m_identifier = identifier;
    m_startCount = currentGDIObjectsInUse();
    m_endCount = 0;
}

GDIObjectCounter::~GDIObjectCounter()
{
    m_endCount = currentGDIObjectsInUse();
    int leaked = m_endCount - m_startCount;
    if (leaked != 0)
        LOG(PlatformLeaks, "%s: leaked %d GDI object%s!", m_identifier.latin1().data(), leaked, leaked == 1 ? "" : "s");
}

unsigned GDIObjectCounter::currentGDIObjectsInUse()
{
    return ::GetGuiResources(::GetCurrentProcess(), GR_GDIOBJECTS);
}

} // namespace WebCore

#endif // !defined(NDEBUG)
