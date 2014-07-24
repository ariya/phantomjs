/*
 * Copyright (C) 2011, 2012 Apple Inc. All Rights Reserved.
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

#import "config.h"
#import "MemoryPressureHandler.h"

#import <WebCore/CSSValuePool.h>
#import <WebCore/GCController.h>
#import <WebCore/FontCache.h>
#import <WebCore/MemoryCache.h>
#import <WebCore/PageCache.h>
#import <WebCore/LayerPool.h>
#import <WebCore/ScrollingThread.h>
#import <WebCore/StorageThread.h>
#import <WebCore/WorkerThread.h>
#import <wtf/CurrentTime.h>
#import <wtf/FastMalloc.h>
#import <wtf/Functional.h>

#if !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#import "WebCoreSystemInterface.h"
#import <notify.h>
#endif

using std::max;

namespace WebCore {

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070

#if !PLATFORM(IOS)
static dispatch_source_t _cache_event_source = 0;
static dispatch_source_t _timer_event_source = 0;
static int _notifyToken;

// Disable memory event reception for a minimum of s_minimumHoldOffTime
// seconds after receiving an event.  Don't let events fire any sooner than
// s_holdOffMultiplier times the last cleanup processing time.  Effectively 
// this is 1 / s_holdOffMultiplier percent of the time.
// These value seems reasonable and testing verifies that it throttles frequent
// low memory events, greatly reducing CPU usage.
static const unsigned s_minimumHoldOffTime = 5;
static const unsigned s_holdOffMultiplier = 20;

void MemoryPressureHandler::install()
{
    if (m_installed || _timer_event_source)
        return;

    dispatch_async(dispatch_get_main_queue(), ^{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
        _cache_event_source = wkCreateMemoryStatusPressureCriticalDispatchOnMainQueue();
#else
        _cache_event_source = wkCreateVMPressureDispatchOnMainQueue();
#endif
        if (_cache_event_source) {
            dispatch_set_context(_cache_event_source, this);
            dispatch_source_set_event_handler(_cache_event_source, ^{ memoryPressureHandler().respondToMemoryPressure();});
            dispatch_resume(_cache_event_source);
        }
    });

    notify_register_dispatch("org.WebKit.lowMemory", &_notifyToken,
         dispatch_get_main_queue(), ^(int) { memoryPressureHandler().respondToMemoryPressure();});

    m_installed = true;
}

void MemoryPressureHandler::uninstall()
{
    if (!m_installed)
        return;

    dispatch_async(dispatch_get_main_queue(), ^{
        if (_cache_event_source) {
            dispatch_source_cancel(_cache_event_source);
            dispatch_release(_cache_event_source);
            _cache_event_source = 0;
        }

        if (_timer_event_source) {
            dispatch_source_cancel(_timer_event_source);
            dispatch_release(_timer_event_source);
            _timer_event_source = 0;
        }
    });

    m_installed = false;
    
    notify_cancel(_notifyToken);
}

void MemoryPressureHandler::holdOff(unsigned seconds)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        _timer_event_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        if (_timer_event_source) {
            dispatch_set_context(_timer_event_source, this);
            dispatch_source_set_timer(_timer_event_source, dispatch_time(DISPATCH_TIME_NOW, seconds * NSEC_PER_SEC), DISPATCH_TIME_FOREVER, 1 * s_minimumHoldOffTime);
            dispatch_source_set_event_handler(_timer_event_source, ^{
                if (_timer_event_source) {
                    dispatch_source_cancel(_timer_event_source);
                    dispatch_release(_timer_event_source);
                    _timer_event_source = 0;
                }
                memoryPressureHandler().install();
            });
            dispatch_resume(_timer_event_source);
        }
    });
}

void MemoryPressureHandler::respondToMemoryPressure()
{
    uninstall();

    double startTime = monotonicallyIncreasingTime();

    m_lowMemoryHandler(false);

    unsigned holdOffTime = (monotonicallyIncreasingTime() - startTime) * s_holdOffMultiplier;

    holdOff(max(holdOffTime, s_minimumHoldOffTime));
}
#endif // !PLATFORM(IOS)

void MemoryPressureHandler::releaseMemory(bool)
{
    int savedPageCacheCapacity = pageCache()->capacity();
    pageCache()->setCapacity(0);
    pageCache()->setCapacity(savedPageCacheCapacity);

    NSURLCache *nsurlCache = [NSURLCache sharedURLCache];
    NSUInteger savedNsurlCacheMemoryCapacity = [nsurlCache memoryCapacity];
    [nsurlCache setMemoryCapacity:0];
    [nsurlCache setMemoryCapacity:savedNsurlCacheMemoryCapacity];

    fontCache()->purgeInactiveFontData();

    memoryCache()->pruneToPercentage(0);

    LayerPool::sharedPool()->drain();

    cssValuePool().drain();

    gcController().discardAllCompiledCode();

    // FastMalloc has lock-free thread specific caches that can only be cleared from the thread itself.
    StorageThread::releaseFastMallocFreeMemoryInAllThreads();
#if ENABLE(WORKERS)
    WorkerThread::releaseFastMallocFreeMemoryInAllThreads();
#endif
#if ENABLE(THREADED_SCROLLING)
    ScrollingThread::dispatch(bind(WTF::releaseFastMallocFreeMemory));
#endif
    WTF::releaseFastMallocFreeMemory();
}
#endif

} // namespace WebCore
