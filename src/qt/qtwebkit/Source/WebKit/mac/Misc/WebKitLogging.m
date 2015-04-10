/*
 * Copyright (C) 2005, 2007 Apple Inc. All rights reserved.
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

#import "WebKitLogging.h"

#if !LOG_DISABLED

WTFLogChannel WebKitLogTextInput =              { 0x00000010, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogTiming =                 { 0x00000020, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogLoading =                { 0x00000040, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogFontCache =              { 0x00000100, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogFontSubstitution =       { 0x00000200, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogDownload =               { 0x00000800, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogDocumentLoad =           { 0x00001000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogPlugins =                { 0x00002000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogEvents =                 { 0x00010000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogView =                   { 0x00020000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogRedirect =               { 0x00040000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogPageCache =              { 0x00080000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogCacheSizes =             { 0x00100000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogFormDelegate =           { 0x00200000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogFileDatabaseActivity =   { 0x00400000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogHistory =                { 0x00800000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogBindings =               { 0x01000000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogFontSelection =          { 0x02000000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogEncoding =               { 0x04000000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogLiveConnect =            { 0x08000000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogBackForward =            { 0x10000000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogProgress =               { 0x20000000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogPluginEvents =           { 0x40000000, "WebKitLogLevel", WTFLogChannelOff };
WTFLogChannel WebKitLogIconDatabase =           { 0x80000000, "WebKitLogLevel", WTFLogChannelOff };

static void initializeLogChannel(WTFLogChannel *channel)
{
    NSString *logLevelString = [[NSUserDefaults standardUserDefaults] objectForKey:[NSString stringWithUTF8String:channel->defaultName]];

    // If there's no log level string from the user defaults, don't obliterate the compiled in values.
    if (logLevelString) {
        channel->state = WTFLogChannelOff;
        unsigned logLevel;
        if (![[NSScanner scannerWithString:logLevelString] scanHexInt:&logLevel])
            NSLog(@"unable to parse hex value for %s (%@), logging is off", channel->defaultName, logLevelString);
        if ((logLevel & channel->mask) == channel->mask)
            channel->state = WTFLogChannelOn;
    }
}

void WebKitInitializeLoggingChannelsIfNecessary()
{
    static bool haveInitializedLoggingChannels = false;
    if (haveInitializedLoggingChannels)
        return;
    haveInitializedLoggingChannels = true;
    
    initializeLogChannel(&WebKitLogTiming);
    initializeLogChannel(&WebKitLogLoading);
    initializeLogChannel(&WebKitLogFontCache);
    initializeLogChannel(&WebKitLogFontSubstitution);
    initializeLogChannel(&WebKitLogDownload);
    initializeLogChannel(&WebKitLogDocumentLoad);
    initializeLogChannel(&WebKitLogPlugins);
    initializeLogChannel(&WebKitLogEvents);
    initializeLogChannel(&WebKitLogView);
    initializeLogChannel(&WebKitLogRedirect);
    initializeLogChannel(&WebKitLogPageCache);
    initializeLogChannel(&WebKitLogCacheSizes);
    initializeLogChannel(&WebKitLogFormDelegate);
    initializeLogChannel(&WebKitLogFileDatabaseActivity);
    initializeLogChannel(&WebKitLogHistory);
    initializeLogChannel(&WebKitLogBindings);
    initializeLogChannel(&WebKitLogFontSelection);
    initializeLogChannel(&WebKitLogEncoding);
    initializeLogChannel(&WebKitLogLiveConnect);
    initializeLogChannel(&WebKitLogBackForward);
    initializeLogChannel(&WebKitLogProgress);
    initializeLogChannel(&WebKitLogPluginEvents);
    initializeLogChannel(&WebKitLogIconDatabase);
    initializeLogChannel(&WebKitLogTextInput);
}
#endif // !LOG_DISABLED

void ReportDiscardedDelegateException(SEL delegateSelector, id exception)
{
    if ([exception isKindOfClass:[NSException class]])
        NSLog(@"*** WebKit discarded an uncaught exception in the %s delegate: <%@> %@",
            sel_getName(delegateSelector), [exception name], [exception reason]);
    else
        NSLog(@"*** WebKit discarded an uncaught exception in the %s delegate: %@",
            sel_getName(delegateSelector), exception);
}
