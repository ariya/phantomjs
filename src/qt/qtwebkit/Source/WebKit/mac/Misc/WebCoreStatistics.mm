/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#import "WebCoreStatistics.h"

#import "DOMElementInternal.h"
#import "WebCache.h"
#import "WebFrameInternal.h"
#import <JavaScriptCore/JSLock.h>
#import <JavaScriptCore/MemoryStatistics.h>
#import <WebCore/FontCache.h>
#import <WebCore/Frame.h>
#import <WebCore/GCController.h>
#import <WebCore/GlyphPageTreeNode.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/IconDatabase.h>
#import <WebCore/JSDOMWindow.h>
#import <WebCore/PageCache.h>
#import <WebCore/PageConsole.h>
#import <WebCore/PrintContext.h>
#import <WebCore/RenderTreeAsText.h>
#import <WebCore/RenderView.h>

using namespace JSC;
using namespace WebCore;

@implementation WebCoreStatistics

+ (NSArray *)statistics
{
    return [WebCache statistics];
}

+ (size_t)javaScriptObjectsCount
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    return JSDOMWindow::commonVM()->heap.objectCount();
}

+ (size_t)javaScriptGlobalObjectsCount
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    return JSDOMWindow::commonVM()->heap.globalObjectCount();
}

+ (size_t)javaScriptProtectedObjectsCount
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    return JSDOMWindow::commonVM()->heap.protectedObjectCount();
}

+ (size_t)javaScriptProtectedGlobalObjectsCount
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    return JSDOMWindow::commonVM()->heap.protectedGlobalObjectCount();
}

+ (NSCountedSet *)javaScriptProtectedObjectTypeCounts
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    
    NSCountedSet *result = [NSCountedSet set];

    OwnPtr<TypeCountSet> counts(JSDOMWindow::commonVM()->heap.protectedObjectTypeCounts());
    HashCountedSet<const char*>::iterator end = counts->end();
    for (HashCountedSet<const char*>::iterator it = counts->begin(); it != end; ++it)
        for (unsigned i = 0; i < it->value; ++i)
            [result addObject:[NSString stringWithUTF8String:it->key]];
    
    return result;
}

+ (NSCountedSet *)javaScriptObjectTypeCounts
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    
    NSCountedSet *result = [NSCountedSet set];

    OwnPtr<TypeCountSet> counts(JSDOMWindow::commonVM()->heap.objectTypeCounts());
    HashCountedSet<const char*>::iterator end = counts->end();
    for (HashCountedSet<const char*>::iterator it = counts->begin(); it != end; ++it)
        for (unsigned i = 0; i < it->value; ++i)
            [result addObject:[NSString stringWithUTF8String:it->key]];
    
    return result;
}

+ (void)garbageCollectJavaScriptObjects
{
    gcController().garbageCollectNow();
}

+ (void)garbageCollectJavaScriptObjectsOnAlternateThreadForDebugging:(BOOL)waitUntilDone
{
    gcController().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
}

+ (void)setJavaScriptGarbageCollectorTimerEnabled:(BOOL)enable
{
    gcController().setJavaScriptGarbageCollectorTimerEnabled(enable);
}

+ (size_t)iconPageURLMappingCount
{
    return iconDatabase().pageURLMappingCount();
}

+ (size_t)iconRetainedPageURLCount
{
    return iconDatabase().retainedPageURLCount();
}

+ (size_t)iconRecordCount
{
    return iconDatabase().iconRecordCount();
}

+ (size_t)iconsWithDataCount
{
    return iconDatabase().iconRecordCountWithData();
}

+ (size_t)cachedFontDataCount
{
    return fontCache()->fontDataCount();
}

+ (size_t)cachedFontDataInactiveCount
{
    return fontCache()->inactiveFontDataCount();
}

+ (void)purgeInactiveFontData
{
    fontCache()->purgeInactiveFontData();
}

+ (size_t)glyphPageCount
{
    return GlyphPageTreeNode::treeGlyphPageCount();
}

+ (BOOL)shouldPrintExceptions
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    return PageConsole::shouldPrintExceptions();
}

+ (void)setShouldPrintExceptions:(BOOL)print
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    PageConsole::setShouldPrintExceptions(print);
}

+ (void)emptyCache
{
    [WebCache empty];
}

+ (void)setCacheDisabled:(BOOL)disabled
{
    [WebCache setDisabled:disabled];
}

+ (void)startIgnoringWebCoreNodeLeaks
{
    WebCore::Node::startIgnoringLeaks();
}

+ (void)stopIgnoringWebCoreNodeLeaks
{
    WebCore::Node::stopIgnoringLeaks();
}

+ (NSDictionary *)memoryStatistics
{
    WTF::FastMallocStatistics fastMallocStatistics = WTF::fastMallocStatistics();
    
    JSLockHolder lock(JSDOMWindow::commonVM());
    size_t heapSize = JSDOMWindow::commonVM()->heap.size();
    size_t heapFree = JSDOMWindow::commonVM()->heap.capacity() - heapSize;
    GlobalMemoryStatistics globalMemoryStats = globalMemoryStatistics();
    
    return [NSDictionary dictionaryWithObjectsAndKeys:
                [NSNumber numberWithInt:fastMallocStatistics.reservedVMBytes], @"FastMallocReservedVMBytes",
                [NSNumber numberWithInt:fastMallocStatistics.committedVMBytes], @"FastMallocCommittedVMBytes",
                [NSNumber numberWithInt:fastMallocStatistics.freeListBytes], @"FastMallocFreeListBytes",
                [NSNumber numberWithInt:heapSize], @"JavaScriptHeapSize",
                [NSNumber numberWithInt:heapFree], @"JavaScriptFreeSize",
                [NSNumber numberWithUnsignedInt:(unsigned int)globalMemoryStats.stackBytes], @"JavaScriptStackSize",
                [NSNumber numberWithUnsignedInt:(unsigned int)globalMemoryStats.JITBytes], @"JavaScriptJITSize",
            nil];
}

+ (void)returnFreeMemoryToSystem
{
    WTF::releaseFastMallocFreeMemory();
}

+ (int)cachedPageCount
{
    return pageCache()->pageCount();
}

+ (int)cachedFrameCount
{
    return pageCache()->frameCount();
}

// Deprecated
+ (int)autoreleasedPageCount
{
    return 0;
}

// Deprecated
+ (size_t)javaScriptNoGCAllowedObjectsCount
{
    return 0;
}

+ (size_t)javaScriptReferencedObjectsCount
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    return JSDOMWindow::commonVM()->heap.protectedObjectCount();
}

+ (NSSet *)javaScriptRootObjectClasses
{
    return [self javaScriptRootObjectTypeCounts];
}

+ (size_t)javaScriptInterpretersCount
{
    return [self javaScriptProtectedGlobalObjectsCount];
}

+ (NSCountedSet *)javaScriptRootObjectTypeCounts
{
    return [self javaScriptProtectedObjectTypeCounts];
}

@end

@implementation WebFrame (WebKitDebug)

- (NSString *)renderTreeAsExternalRepresentationForPrinting:(BOOL)forPrinting
{
    return externalRepresentation(_private->coreFrame, forPrinting ? RenderAsTextPrintingMode : RenderAsTextBehaviorNormal);
}

- (int)numberOfPagesWithPageWidth:(float)pageWidthInPixels pageHeight:(float)pageHeightInPixels
{
    return PrintContext::numberOfPages(_private->coreFrame, FloatSize(pageWidthInPixels, pageHeightInPixels));
}

- (void)printToCGContext:(CGContextRef)cgContext pageWidth:(float)pageWidthInPixels pageHeight:(float)pageHeightInPixels
{
    Frame* coreFrame = _private->coreFrame;
    if (!coreFrame)
        return;

    GraphicsContext graphicsContext(cgContext);
    PrintContext::spoolAllPagesWithBoundaries(coreFrame, graphicsContext, FloatSize(pageWidthInPixels, pageHeightInPixels));
}

@end
