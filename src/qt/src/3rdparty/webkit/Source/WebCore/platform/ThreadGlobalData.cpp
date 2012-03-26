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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"
#include "ThreadGlobalData.h"

#include "EventNames.h"
#include "ThreadTimers.h"
#include <wtf/UnusedParam.h>
#include <wtf/WTFThreadData.h>
#include <wtf/text/StringImpl.h>

#if USE(ICU_UNICODE)
#include "TextCodecICU.h"
#endif

#if PLATFORM(MAC)
#include "TextCodecMac.h"
#endif

#if ENABLE(WORKERS)
#include <wtf/Threading.h>
#include <wtf/ThreadSpecific.h>
using namespace WTF;
#endif

namespace WebCore {

#if ENABLE(WORKERS)
ThreadSpecific<ThreadGlobalData>* ThreadGlobalData::staticData;
#else
ThreadGlobalData* ThreadGlobalData::staticData;
#endif

ThreadGlobalData::ThreadGlobalData()
    : m_eventNames(new EventNames)
    , m_threadTimers(new ThreadTimers)
#ifndef NDEBUG
    , m_isMainThread(isMainThread())
#endif
#if USE(ICU_UNICODE)
    , m_cachedConverterICU(new ICUConverterWrapper)
#endif
#if PLATFORM(MAC)
    , m_cachedConverterTEC(new TECConverterWrapper)
#endif
{
    // This constructor will have been called on the main thread before being called on
    // any other thread, and is only called once per thread - this makes this a convenient
    // point to call methods that internally perform a one-time initialization that is not
    // threadsafe.
    wtfThreadData();
    StringImpl::empty();
}

ThreadGlobalData::~ThreadGlobalData()
{
    destroy();
}

void ThreadGlobalData::destroy()
{
#if PLATFORM(MAC)
    delete m_cachedConverterTEC;
    m_cachedConverterTEC = 0;
#endif

#if USE(ICU_UNICODE)
    delete m_cachedConverterICU;
    m_cachedConverterICU = 0;
#endif

    delete m_eventNames;
    m_eventNames = 0;
    delete m_threadTimers;
    m_threadTimers = 0;
}

} // namespace WebCore
