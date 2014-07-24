/*
 * Copyright (C) 2005, 2007, 2008 Apple Inc. All rights reserved.
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

#import <wtf/Assertions.h>

#ifndef LOG_CHANNEL_PREFIX
#define LOG_CHANNEL_PREFIX WebKitLog
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !LOG_DISABLED
extern WTFLogChannel WebKitLogTiming;
extern WTFLogChannel WebKitLogLoading;
extern WTFLogChannel WebKitLogFontCache;
extern WTFLogChannel WebKitLogFontSubstitution;
extern WTFLogChannel WebKitLogFontSelection;
extern WTFLogChannel WebKitLogDownload;
extern WTFLogChannel WebKitLogDocumentLoad;
extern WTFLogChannel WebKitLogPlugins;
extern WTFLogChannel WebKitLogEvents;
extern WTFLogChannel WebKitLogView;
extern WTFLogChannel WebKitLogRedirect;
extern WTFLogChannel WebKitLogPageCache;
extern WTFLogChannel WebKitLogCacheSizes;
extern WTFLogChannel WebKitLogFormDelegate;
extern WTFLogChannel WebKitLogFileDatabaseActivity;
extern WTFLogChannel WebKitLogHistory;
extern WTFLogChannel WebKitLogBindings;
extern WTFLogChannel WebKitLogEncoding;
extern WTFLogChannel WebKitLogLiveConnect;
extern WTFLogChannel WebKitLogBackForward;
extern WTFLogChannel WebKitLogProgress;
extern WTFLogChannel WebKitLogPluginEvents;
extern WTFLogChannel WebKitLogIconDatabase;
extern WTFLogChannel WebKitLogTextInput;

void WebKitInitializeLoggingChannelsIfNecessary(void);
#endif // !LOG_DISABLED

// FIXME: Why is this in the "logging" header file?
// Use WebCoreThreadViolationCheck instead for checks that throw an exception even in production builds.
#if !defined(NDEBUG) && !defined(DISABLE_THREAD_CHECK)
#define ASSERT_MAIN_THREAD() do \
    if (!pthread_main_np()) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, "<not running on main thread>"); \
        CRASH(); \
    } \
while (0)
#else
#define ASSERT_MAIN_THREAD() ((void)0)
#endif

void ReportDiscardedDelegateException(SEL delegateSelector, id exception);

#ifdef __cplusplus
}
#endif
