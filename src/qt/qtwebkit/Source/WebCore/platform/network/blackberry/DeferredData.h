/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DeferredData_h
#define DeferredData_h

#include "Timer.h"

#include <wtf/Deque.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class NetworkJob;

class RecursionGuard {
public:
    RecursionGuard(bool& guard)
        : m_guard(guard)
    {
        ASSERT(!m_guard);
        m_guard = true;
    }

    ~RecursionGuard()
    {
        m_guard = false;
    }

private:
    bool& m_guard;
};

class DeferredData {
public:
    DeferredData(NetworkJob&);
    void deferOpen(int status, const String& message);
    void deferHeaderReceived(const String& key, const String& value);
    void deferMultipartHeaderReceived(const String& key, const String& value);
    void deferDataReceived(const char* buf, size_t len);
    void deferDataSent(unsigned long long bytesSent, unsigned long long totalBytesToBeSent);
    void deferClose(int status);

    bool hasDeferredData() const
    {
        return m_deferredStatusReceived || !m_headerKeys.isEmpty() || !m_multipartHeaderKeys.isEmpty() || !m_dataSegments.isEmpty() || m_deferredClose;
    }

    void processDeferredData();

    void scheduleProcessDeferredData()
    {
        if (!m_processDataTimer.isActive())
            m_processDataTimer.startOneShot(0);
    }

private:
    typedef void (NetworkJob::*HandleHeadersFunction)(const String& key, const String& value);

    // Returns false if the job is deferred or canceled, otherwise returns true.
    bool processHeaders(Vector<String>& headerKeys, Vector<String>& headerValues, HandleHeadersFunction);

    void fireProcessDataTimer(Timer<DeferredData>*);
    NetworkJob& m_job;
    Timer<DeferredData> m_processDataTimer;
    bool m_deferredStatusReceived;
    int m_status;
    String m_message;
    Vector<String> m_headerKeys;
    Vector<String> m_headerValues;
    Vector<String> m_multipartHeaderKeys;
    Vector<String> m_multipartheaderValues;
    Deque<Vector<char> > m_dataSegments;
    unsigned long long m_bytesSent;
    unsigned long long m_totalBytesToBeSent;
    int m_deferredCloseStatus;
    bool m_deferredClose;
    bool m_inProcessDeferredData;
};

} // namespace WebCore

#endif // DeferredData_h
