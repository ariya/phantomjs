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

#include "config.h"
#include "DeferredData.h"

#include "NetworkJob.h"

namespace WebCore {

DeferredData::DeferredData(NetworkJob& job)
    : m_job(job)
    , m_processDataTimer(this, &DeferredData::fireProcessDataTimer)
    , m_deferredStatusReceived(false)
    , m_status(0)
    , m_bytesSent(0)
    , m_totalBytesToBeSent(0)
    , m_deferredCloseStatus(BlackBerry::Platform::FilterStream::StatusSuccess)
    , m_deferredClose(false)
    , m_inProcessDeferredData(false)
{
}

void DeferredData::deferOpen(int status, const String& message)
{
    m_deferredStatusReceived = true;
    m_status = status;
    m_message = message;
}

void DeferredData::deferHeaderReceived(const String& key, const String& value)
{
    m_headerKeys.append(key);
    m_headerValues.append(value);
}

void DeferredData::deferMultipartHeaderReceived(const String& key, const String& value)
{
    m_multipartHeaderKeys.append(key);
    m_multipartheaderValues.append(value);
}

void DeferredData::deferDataSent(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    m_bytesSent = bytesSent;
    m_totalBytesToBeSent = totalBytesToBeSent;
}

void DeferredData::deferDataReceived(const char* buf, size_t len)
{
    m_dataSegments.append(Vector<char>());
    m_dataSegments.rbegin()->append(buf, len);
}

void DeferredData::deferClose(int status)
{
    m_deferredCloseStatus = status;
    m_deferredClose = true;
}

bool DeferredData::processHeaders(Vector<String>& headerKeys, Vector<String>& headerValues, HandleHeadersFunction function)
{
    size_t numHeaders = headerKeys.size();
    ASSERT(headerValues.size() == numHeaders);
    for (size_t i = 0; i < numHeaders; ++i) {
        (m_job.*function)(headerKeys[i], headerValues[i]);

        if (m_job.isDeferringLoading()) {
            // Remove all the headers that have already been processed.
            headerKeys.remove(0, i + 1);
            headerValues.remove(0, i + 1);
            return false;
        }

        if (m_job.isCancelled()) {
            // Don't bother removing headers; job will be deleted.
            return false;
        }
    }
    headerKeys.clear();
    headerValues.clear();
    return true;
}

void DeferredData::processDeferredData()
{
    if (m_inProcessDeferredData)
        return;

    RecursionGuard guard(m_inProcessDeferredData);

    // Sending data to webkit causes it to be parsed and javascript executed,
    // which might cause it to cancel the job or set defersLoading again. So
    // to be safe, check this after every step.
    if (m_job.isDeferringLoading() || m_job.isCancelled())
        return;

    if (m_deferredStatusReceived) {
        m_job.handleNotifyStatusReceived(m_status, m_message);
        m_deferredStatusReceived = false;
        if (m_job.isDeferringLoading() || m_job.isCancelled())
            return;
    }

    if (!processHeaders(m_headerKeys, m_headerValues, &NetworkJob::handleNotifyHeaderReceived)
        || !processHeaders(m_multipartHeaderKeys, m_multipartheaderValues, &NetworkJob::handleNotifyMultipartHeaderReceived))
        return;

    // Only process 32k of data at a time to avoid blocking the event loop for too long.
    static const unsigned maxData = 32 * 1024;

    unsigned totalData = 0;
    while (!m_dataSegments.isEmpty()) {
        const Vector<char>& buffer = m_dataSegments.first();
        m_job.handleNotifyDataReceived(buffer.data(), buffer.size());
        totalData += buffer.size();

        if (m_job.isCancelled()) {
            // Don't bother removing segment; job will be deleted.
            return;
        }

        m_dataSegments.removeFirst();

        if (m_job.isDeferringLoading()) {
            // Stop processing until deferred loading is turned off again.
            return;
        }

        if (totalData >= maxData && !m_dataSegments.isEmpty()) {
            // Pause for now, and schedule a timer to continue after running the event loop.
            scheduleProcessDeferredData();
            return;
        }
    }

    if (m_totalBytesToBeSent) {
        m_job.handleNotifyDataSent(m_bytesSent, m_totalBytesToBeSent);
        m_bytesSent = 0;
        m_totalBytesToBeSent = 0;
    }

    if (m_deferredClose) {
        m_job.handleNotifyClose(m_deferredCloseStatus);
        m_deferredClose = false;
        if (m_job.isDeferringLoading() || m_job.isCancelled())
            return;
    }
}

void DeferredData::fireProcessDataTimer(Timer<DeferredData>*)
{
    processDeferredData();
}

} // namespace WebCore
