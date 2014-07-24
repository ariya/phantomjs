/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 */

#include "config.h"
#include "ProgressTracker.h"

#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderStateMachine.h"
#include "FrameLoaderClient.h"
#include "InspectorInstrumentation.h"
#include "Logging.h"
#include "ResourceResponse.h"
#include <wtf/text/CString.h>
#include <wtf/CurrentTime.h>

using std::min;

namespace WebCore {

// Always start progress at initialProgressValue. This helps provide feedback as 
// soon as a load starts.
static const double initialProgressValue = 0.1;
    
// Similarly, always leave space at the end. This helps show the user that we're not done
// until we're done.
static const double finalProgressValue = 0.9; // 1.0 - initialProgressValue

static const int progressItemDefaultEstimatedLength = 1024 * 16;

// Check if the load is progressing this often.
static const double progressHeartbeatInterval = 0.1;
// How many heartbeats must pass without progress before deciding the load is currently stalled.
static const unsigned loadStalledHeartbeatCount = 4;
// How many bytes are required between heartbeats to consider it progress.
static const unsigned minumumBytesPerHeartbeatForProgress = 1024;

struct ProgressItem {
    WTF_MAKE_NONCOPYABLE(ProgressItem); WTF_MAKE_FAST_ALLOCATED;
public:
    ProgressItem(long long length) 
        : bytesReceived(0)
        , estimatedLength(length) { }
    
    long long bytesReceived;
    long long estimatedLength;
};

unsigned long ProgressTracker::s_uniqueIdentifier = 0;

ProgressTracker::ProgressTracker()
    : m_totalPageAndResourceBytesToLoad(0)
    , m_totalBytesReceived(0)
    , m_lastNotifiedProgressValue(0)
    , m_lastNotifiedProgressTime(0)
    , m_progressNotificationInterval(0.02)
    , m_progressNotificationTimeInterval(0.1)
    , m_finalProgressChangedSent(false)
    , m_progressValue(0)
    , m_numProgressTrackedFrames(0)
    , m_progressHeartbeatTimer(this, &ProgressTracker::progressHeartbeatTimerFired)
    , m_heartbeatsWithNoProgress(0)
    , m_totalBytesReceivedBeforePreviousHeartbeat(0)
{
}

ProgressTracker::~ProgressTracker()
{
}

PassOwnPtr<ProgressTracker> ProgressTracker::create()
{
    return adoptPtr(new ProgressTracker);
}

double ProgressTracker::estimatedProgress() const
{
    return m_progressValue;
}

void ProgressTracker::reset()
{
    m_progressItems.clear();    

    m_totalPageAndResourceBytesToLoad = 0;
    m_totalBytesReceived = 0;
    m_progressValue = 0;
    m_lastNotifiedProgressValue = 0;
    m_lastNotifiedProgressTime = 0;
    m_finalProgressChangedSent = false;
    m_numProgressTrackedFrames = 0;
    m_originatingProgressFrame = 0;

    m_heartbeatsWithNoProgress = 0;
    m_totalBytesReceivedBeforePreviousHeartbeat = 0;
    m_progressHeartbeatTimer.stop();
}

void ProgressTracker::progressStarted(Frame* frame)
{
    LOG(Progress, "Progress started (%p) - frame %p(\"%s\"), value %f, tracked frames %d, originating frame %p", this, frame, frame->tree()->uniqueName().string().utf8().data(), m_progressValue, m_numProgressTrackedFrames, m_originatingProgressFrame.get());

    frame->loader()->client()->willChangeEstimatedProgress();
    
    if (m_numProgressTrackedFrames == 0 || m_originatingProgressFrame == frame) {
        reset();
        m_progressValue = initialProgressValue;
        m_originatingProgressFrame = frame;

        m_progressHeartbeatTimer.startRepeating(progressHeartbeatInterval);
        m_originatingProgressFrame->loader()->loadProgressingStatusChanged();

        m_originatingProgressFrame->loader()->client()->postProgressStartedNotification();
    }
    m_numProgressTrackedFrames++;

    frame->loader()->client()->didChangeEstimatedProgress();
    InspectorInstrumentation::frameStartedLoading(frame);
}

void ProgressTracker::progressCompleted(Frame* frame)
{
    LOG(Progress, "Progress completed (%p) - frame %p(\"%s\"), value %f, tracked frames %d, originating frame %p", this, frame, frame->tree()->uniqueName().string().utf8().data(), m_progressValue, m_numProgressTrackedFrames, m_originatingProgressFrame.get());
    
    if (m_numProgressTrackedFrames <= 0)
        return;
    
    frame->loader()->client()->willChangeEstimatedProgress();
        
    m_numProgressTrackedFrames--;
    if (!m_numProgressTrackedFrames || m_originatingProgressFrame == frame)
        finalProgressComplete();
    
    frame->loader()->client()->didChangeEstimatedProgress();
}

void ProgressTracker::finalProgressComplete()
{
    LOG(Progress, "Final progress complete (%p)", this);
    
    RefPtr<Frame> frame = m_originatingProgressFrame.release();
    
    // Before resetting progress value be sure to send client a least one notification
    // with final progress value.
    if (!m_finalProgressChangedSent) {
        m_progressValue = 1;
        frame->loader()->client()->postProgressEstimateChangedNotification();
    }

    reset();

    frame->loader()->client()->setMainFrameDocumentReady(true);
    frame->loader()->client()->postProgressFinishedNotification();
    frame->loader()->loadProgressingStatusChanged();

    InspectorInstrumentation::frameStoppedLoading(frame.get());
}

void ProgressTracker::incrementProgress(unsigned long identifier, const ResourceResponse& response)
{
    LOG(Progress, "Progress incremented (%p) - value %f, tracked frames %d, originating frame %p", this, m_progressValue, m_numProgressTrackedFrames, m_originatingProgressFrame.get());

    if (m_numProgressTrackedFrames <= 0)
        return;
    
    long long estimatedLength = response.expectedContentLength();
    if (estimatedLength < 0)
        estimatedLength = progressItemDefaultEstimatedLength;
    
    m_totalPageAndResourceBytesToLoad += estimatedLength;

    if (ProgressItem* item = m_progressItems.get(identifier)) {
        item->bytesReceived = 0;
        item->estimatedLength = estimatedLength;
    } else
        m_progressItems.set(identifier, adoptPtr(new ProgressItem(estimatedLength)));
}

void ProgressTracker::incrementProgress(unsigned long identifier, const char*, int length)
{
    ProgressItem* item = m_progressItems.get(identifier);
    
    // FIXME: Can this ever happen?
    if (!item)
        return;

    RefPtr<Frame> frame = m_originatingProgressFrame;
    
    frame->loader()->client()->willChangeEstimatedProgress();
    
    unsigned bytesReceived = length;
    double increment, percentOfRemainingBytes;
    long long remainingBytes, estimatedBytesForPendingRequests;
    
    item->bytesReceived += bytesReceived;
    if (item->bytesReceived > item->estimatedLength) {
        m_totalPageAndResourceBytesToLoad += ((item->bytesReceived * 2) - item->estimatedLength);
        item->estimatedLength = item->bytesReceived * 2;
    }
    
    int numPendingOrLoadingRequests = frame->loader()->numPendingOrLoadingRequests(true);
    estimatedBytesForPendingRequests = progressItemDefaultEstimatedLength * numPendingOrLoadingRequests;
    remainingBytes = ((m_totalPageAndResourceBytesToLoad + estimatedBytesForPendingRequests) - m_totalBytesReceived);
    if (remainingBytes > 0)  // Prevent divide by 0.
        percentOfRemainingBytes = (double)bytesReceived / (double)remainingBytes;
    else
        percentOfRemainingBytes = 1.0;
    
    // For documents that use WebCore's layout system, treat first layout as the half-way point.
    // FIXME: The hasHTMLView function is a sort of roundabout way of asking "do you use WebCore's layout system".
    bool useClampedMaxProgress = frame->loader()->client()->hasHTMLView()
        && !frame->loader()->stateMachine()->firstLayoutDone();
    double maxProgressValue = useClampedMaxProgress ? 0.5 : finalProgressValue;
    increment = (maxProgressValue - m_progressValue) * percentOfRemainingBytes;
    m_progressValue += increment;
    m_progressValue = min(m_progressValue, maxProgressValue);
    ASSERT(m_progressValue >= initialProgressValue);
    
    m_totalBytesReceived += bytesReceived;
    
    double now = currentTime();
    double notifiedProgressTimeDelta = now - m_lastNotifiedProgressTime;
    
    LOG(Progress, "Progress incremented (%p) - value %f, tracked frames %d", this, m_progressValue, m_numProgressTrackedFrames);
    double notificationProgressDelta = m_progressValue - m_lastNotifiedProgressValue;
    if ((notificationProgressDelta >= m_progressNotificationInterval ||
         notifiedProgressTimeDelta >= m_progressNotificationTimeInterval) &&
        m_numProgressTrackedFrames > 0) {
        if (!m_finalProgressChangedSent) {
            if (m_progressValue == 1)
                m_finalProgressChangedSent = true;
            
            frame->loader()->client()->postProgressEstimateChangedNotification();

            m_lastNotifiedProgressValue = m_progressValue;
            m_lastNotifiedProgressTime = now;
        }
    }
    
    frame->loader()->client()->didChangeEstimatedProgress();
}

void ProgressTracker::completeProgress(unsigned long identifier)
{
    ProgressItem* item = m_progressItems.get(identifier);
    
    // This can happen if a load fails without receiving any response data.
    if (!item)
        return;
    
    // Adjust the total expected bytes to account for any overage/underage.
    long long delta = item->bytesReceived - item->estimatedLength;
    m_totalPageAndResourceBytesToLoad += delta;

    m_progressItems.remove(identifier);
}

unsigned long ProgressTracker::createUniqueIdentifier()
{
    return ++s_uniqueIdentifier;
}

bool ProgressTracker::isMainLoadProgressing() const
{
    if (!m_originatingProgressFrame)
        return false;
    // See if the load originated from a subframe.
    if (m_originatingProgressFrame->tree()->parent())
        return false;
    return m_progressValue && m_progressValue < finalProgressValue && m_heartbeatsWithNoProgress < loadStalledHeartbeatCount;
}

void ProgressTracker::progressHeartbeatTimerFired(Timer<ProgressTracker>*)
{
    if (m_totalBytesReceived < m_totalBytesReceivedBeforePreviousHeartbeat + minumumBytesPerHeartbeatForProgress)
        ++m_heartbeatsWithNoProgress;
    else
        m_heartbeatsWithNoProgress = 0;

    m_totalBytesReceivedBeforePreviousHeartbeat = m_totalBytesReceived;

    m_originatingProgressFrame->loader()->loadProgressingStatusChanged();

    if (m_progressValue >= finalProgressValue)
        m_progressHeartbeatTimer.stop();
}

}
