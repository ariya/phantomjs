/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#import "WebCache.h"

#import "WebNSObjectExtras.h"
#import "WebPreferences.h"
#import "WebSystemInterface.h"
#import "WebView.h"
#import "WebViewInternal.h"
#import <WebCore/ApplicationCacheStorage.h>
#import <WebCore/CrossOriginPreflightResultCache.h>
#import <WebCore/MemoryCache.h>
#import <WebCore/RunLoop.h>
#import <runtime/InitializeThreading.h>
#import <wtf/MainThread.h>

@implementation WebCache

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
    InitWebCoreSystemInterface();   
}

+ (NSArray *)statistics
{
    WebCore::MemoryCache::Statistics s = WebCore::memoryCache()->getStatistics();

    return [NSArray arrayWithObjects:
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.count], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.count], @"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.count], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.count], @"JavaScript",
            nil],
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.size], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.size] ,@"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.size], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.size], @"JavaScript",
            nil],
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.liveSize], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.liveSize] ,@"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.liveSize], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.liveSize], @"JavaScript",
            nil],
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.decodedSize], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.decodedSize] ,@"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.decodedSize], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.decodedSize], @"JavaScript",
            nil],
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.purgeableSize], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.purgeableSize] ,@"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.purgeableSize], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.purgeableSize], @"JavaScript",
            nil],
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.purgedSize], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.purgedSize] ,@"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.purgedSize], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.purgedSize], @"JavaScript",
            nil],
        nil];
}

+ (void)empty
{
    // Toggling the cache model like this forces the cache to evict all its in-memory resources.
    WebCacheModel cacheModel = [WebView _cacheModel];
    [WebView _setCacheModel:WebCacheModelDocumentViewer];
    [WebView _setCacheModel:cacheModel];

    // Empty the application cache.
    WebCore::cacheStorage().empty();

    // Empty the Cross-Origin Preflight cache
    WebCore::CrossOriginPreflightResultCache::shared().empty();
}

+ (void)setDisabled:(BOOL)disabled
{
    if (!pthread_main_np())
        return [[self _webkit_invokeOnMainThread] setDisabled:disabled];

    WebCore::memoryCache()->setDisabled(disabled);
}

+ (BOOL)isDisabled
{
    return WebCore::memoryCache()->disabled();
}

@end
