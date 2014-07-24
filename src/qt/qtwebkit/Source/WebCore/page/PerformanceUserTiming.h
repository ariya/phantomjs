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

#ifndef PerformanceUserTiming_h
#define PerformanceUserTiming_h

#if ENABLE(USER_TIMING)

#include "EventException.h"
#include "ExceptionCode.h"
#include "Performance.h"
#include "PerformanceTiming.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Performance;
class PerformanceEntry;

typedef unsigned long long (PerformanceTiming::*NavigationTimingFunction)() const;
typedef HashMap<String, Vector<RefPtr<PerformanceEntry> > > PerformanceEntryMap;

class UserTiming : public RefCounted<UserTiming> {
public:
    static PassRefPtr<UserTiming> create(Performance* performance) { return adoptRef(new UserTiming(performance)); }

    void mark(const String& markName, ExceptionCode&);
    void clearMarks(const String& markName);

    void measure(const String& measureName, const String& startMark, const String& endMark, ExceptionCode&);
    void clearMeasures(const String& measureName);

    Vector<RefPtr<PerformanceEntry> > getMarks() const;
    Vector<RefPtr<PerformanceEntry> > getMeasures() const;

    Vector<RefPtr<PerformanceEntry> > getMarks(const String& name) const;
    Vector<RefPtr<PerformanceEntry> > getMeasures(const String& name) const;

private:
    explicit UserTiming(Performance*);

    double findExistingMarkStartTime(const String& markName, ExceptionCode&);
    Performance* m_performance;
    PerformanceEntryMap m_marksMap;
    PerformanceEntryMap m_measuresMap;
};

}

#endif // ENABLE(USER_TIMING)

#endif // !defined(PerformanceUserTiming_h)
