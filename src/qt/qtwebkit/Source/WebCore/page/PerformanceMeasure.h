/*
 * Copyright (C) 2012 Intel Inc. All rights reserved.
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

#ifndef PerformanceMeasure_h
#define PerformanceMeasure_h

#if ENABLE(USER_TIMING)

#include "PerformanceEntry.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class PerformanceMeasure : public PerformanceEntry {
public:
    static PassRefPtr<PerformanceMeasure> create(const String& name, double startTime, double duration) { return adoptRef(new PerformanceMeasure(name, startTime, duration)); }

    virtual bool isMeasure() { return true; }

private:
    PerformanceMeasure(const String& name, double startTime, double duration) : PerformanceEntry(name, "measure", startTime, duration) { }
    ~PerformanceMeasure() { }
};

}

#endif // ENABLE(USER_TIMING)

#endif // !defined(PerformanceMeasure_h)
