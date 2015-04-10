/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#import "WebArchive.h"
#import "WebArchiveInternal.h"

#import "WebKitLogging.h"
#import "WebNSObjectExtras.h"
#import "WebResourceInternal.h"
#import "WebTypesInternal.h"
#import <JavaScriptCore/InitializeThreading.h>
#import <WebCore/ArchiveResource.h>
#import <WebCore/LegacyWebArchive.h>
#import <WebCore/RunLoop.h>
#import <WebCore/ThreadCheck.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <wtf/MainThread.h>

using namespace WebCore;

NSString *WebArchivePboardType = @"Apple Web Archive pasteboard type";

static NSString * const WebMainResourceKey = @"WebMainResource";
static NSString * const WebSubresourcesKey = @"WebSubresources";
static NSString * const WebSubframeArchivesKey = @"WebSubframeArchives";

@interface WebArchivePrivate : NSObject {
@public
    WebResource *cachedMainResource;
    NSArray *cachedSubresources;
    NSArray *cachedSubframeArchives;
@private
    RefPtr<LegacyWebArchive> coreArchive;
}

- (id)initWithCoreArchive:(PassRefPtr<LegacyWebArchive>)coreArchive;
- (LegacyWebArchive*)coreArchive;
- (void)setCoreArchive:(PassRefPtr<LegacyWebArchive>)newCoreArchive;
@end

@implementation WebArchivePrivate

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
    WebCoreObjCFinalizeOnMainThread(self);
}

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    coreArchive = LegacyWebArchive::create();
    return self;
}

- (id)initWithCoreArchive:(PassRefPtr<LegacyWebArchive>)_coreArchive
{
    self = [super init];
    if (!self || !_coreArchive) {
        [self release];
        return nil;
    }
    coreArchive = _coreArchive;
    return self;
}

- (LegacyWebArchive*)coreArchive
{
    return coreArchive.get();
}

- (void)setCoreArchive:(PassRefPtr<LegacyWebArchive>)newCoreArchive
{
    ASSERT(coreArchive);
    ASSERT(newCoreArchive);
    coreArchive = newCoreArchive;
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebArchivePrivate class], self))
        return;
    
    [cachedMainResource release];
    [cachedSubresources release];
    [cachedSubframeArchives release];
    
    [super dealloc];
}

@end

@implementation WebArchive

- (id)init
{
    WebCoreThreadViolationCheckRoundTwo();

    self = [super init];
    if (!self)
        return nil;
    _private = [[WebArchivePrivate alloc] init];
    return self;
}

static BOOL isArrayOfClass(id object, Class elementClass)
{
    if (![object isKindOfClass:[NSArray class]])
        return NO;
    NSArray *array = (NSArray *)object;
    NSUInteger count = [array count];
    for (NSUInteger i = 0; i < count; ++i)
        if (![[array objectAtIndex:i] isKindOfClass:elementClass])
            return NO;
    return YES;
}

- (id)initWithMainResource:(WebResource *)mainResource subresources:(NSArray *)subresources subframeArchives:(NSArray *)subframeArchives
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] initWithMainResource:mainResource subresources:subresources subframeArchives:subframeArchives];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    self = [super init];
    if (!self)
        return nil;

    _private = [[WebArchivePrivate alloc] init];

    _private->cachedMainResource = [mainResource retain];
    if (!_private->cachedMainResource) {
        [self release];
        return nil;
    }
    
    if (!subresources || isArrayOfClass(subresources, [WebResource class]))
        _private->cachedSubresources = [subresources retain];
    else {
        [self release];
        return nil;
    }

    if (!subframeArchives || isArrayOfClass(subframeArchives, [WebArchive class]))
        _private->cachedSubframeArchives = [subframeArchives retain];
    else {
        [self release];
        return nil;
    }
    
    RefPtr<ArchiveResource> coreMainResource = mainResource ? [mainResource _coreResource] : 0;

    Vector<PassRefPtr<ArchiveResource> > coreResources;
    NSEnumerator *enumerator = [subresources objectEnumerator];
    WebResource *subresource;
    while ((subresource = [enumerator nextObject]) != nil)
        coreResources.append([subresource _coreResource]);

    Vector<PassRefPtr<LegacyWebArchive> > coreArchives;
    enumerator = [subframeArchives objectEnumerator];
    WebArchive *subframeArchive;
    while ((subframeArchive = [enumerator nextObject]) != nil)
        coreArchives.append([subframeArchive->_private coreArchive]);

    [_private setCoreArchive:LegacyWebArchive::create(coreMainResource.release(), coreResources, coreArchives)];
    if (![_private coreArchive]) {
        [self release];
        return nil;
    }

    return self;
}

- (id)initWithData:(NSData *)data
{
    WebCoreThreadViolationCheckRoundTwo();

    self = [super init];
    if (!self)
        return nil;
        
#if !LOG_DISABLED
    CFAbsoluteTime start = CFAbsoluteTimeGetCurrent();
#endif

    _private = [[WebArchivePrivate alloc] init];
    RefPtr<LegacyWebArchive> coreArchive = LegacyWebArchive::create(SharedBuffer::wrapNSData(data).get());
    if (!coreArchive) {
        [self release];
        return nil;
    }
        
    [_private setCoreArchive:coreArchive.release()];
        
#if !LOG_DISABLED
    CFAbsoluteTime end = CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime duration = end - start;
#endif
    LOG(Timing, "Parsing web archive with [NSPropertyListSerialization propertyListFromData::::] took %f seconds", duration);
    
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder
{    
    WebResource *mainResource = nil;
    NSArray *subresources = nil;
    NSArray *subframeArchives = nil;
    
    @try {
        id object = [decoder decodeObjectForKey:WebMainResourceKey];
        if ([object isKindOfClass:[WebResource class]])
            mainResource = object;
        object = [decoder decodeObjectForKey:WebSubresourcesKey];
        if (isArrayOfClass(object, [WebResource class]))
            subresources = object;
        object = [decoder decodeObjectForKey:WebSubframeArchivesKey];
        if (isArrayOfClass(object, [WebArchive class]))
            subframeArchives = object;
    } @catch(id) {
        [self release];
        return nil;
    }

    return [self initWithMainResource:mainResource subresources:subresources subframeArchives:subframeArchives];
}

- (void)encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeObject:[self mainResource] forKey:WebMainResourceKey];
    [encoder encodeObject:[self subresources] forKey:WebSubresourcesKey];
    [encoder encodeObject:[self subframeArchives] forKey:WebSubframeArchivesKey];    
}

- (void)dealloc
{
    [_private release];
    [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone
{
    return [self retain];
}

- (WebResource *)mainResource
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] mainResource];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    // Currently from WebKit API perspective, WebArchives are entirely immutable once created
    // If they ever become mutable, we'll need to rethink this. 
    if (!_private->cachedMainResource) {
        LegacyWebArchive* coreArchive = [_private coreArchive];
        if (coreArchive)
            _private->cachedMainResource = [[WebResource alloc] _initWithCoreResource:coreArchive->mainResource()];
    }
    
    return [[_private->cachedMainResource retain] autorelease];
}

- (NSArray *)subresources
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] subresources];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    // Currently from WebKit API perspective, WebArchives are entirely immutable once created
    // If they ever become mutable, we'll need to rethink this.     
    if (!_private->cachedSubresources) {
        LegacyWebArchive* coreArchive = [_private coreArchive];
        if (!coreArchive)
            _private->cachedSubresources = [[NSArray alloc] init];
        else {
            const Vector<RefPtr<ArchiveResource> >& subresources(coreArchive->subresources());
            NSMutableArray *mutableArray = [[NSMutableArray alloc] initWithCapacity:subresources.size()];
            _private->cachedSubresources = mutableArray;
            for (unsigned i = 0; i < subresources.size(); ++i) {
                WebResource *resource = [[WebResource alloc] _initWithCoreResource:subresources[i].get()];
                if (resource) {
                    [mutableArray addObject:resource];
                    [resource release];
                }
            }
        }
    }
    // Maintain the WebKit 3 behavior of this API, which is documented and
    // relied upon by some clients, of returning nil if there are no subresources.
    return [_private->cachedSubresources count] ? [[_private->cachedSubresources retain] autorelease] : nil;
}

- (NSArray *)subframeArchives
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] subframeArchives];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    // Currently from WebKit API perspective, WebArchives are entirely immutable once created
    // If they ever become mutable, we'll need to rethink this.  
    if (!_private->cachedSubframeArchives) {
        LegacyWebArchive* coreArchive = [_private coreArchive];
        if (!coreArchive)
            _private->cachedSubframeArchives = [[NSArray alloc] init];
        else {
            const Vector<RefPtr<Archive> >& subframeArchives(coreArchive->subframeArchives());
            NSMutableArray *mutableArray = [[NSMutableArray alloc] initWithCapacity:subframeArchives.size()];
            _private->cachedSubframeArchives = mutableArray;
            for (unsigned i = 0; i < subframeArchives.size(); ++i) {
                WebArchive *archive = [[WebArchive alloc] _initWithCoreLegacyWebArchive:(LegacyWebArchive *)subframeArchives[i].get()];
                [mutableArray addObject:archive];
                [archive release];
            }
        }
    }
    
    return [[_private->cachedSubframeArchives retain] autorelease];
}

- (NSData *)data
{
    WebCoreThreadViolationCheckRoundTwo();

#if !LOG_DISABLED
    CFAbsoluteTime start = CFAbsoluteTimeGetCurrent();
#endif

    RetainPtr<CFDataRef> data = [_private coreArchive]->rawDataRepresentation();
    
#if !LOG_DISABLED
    CFAbsoluteTime end = CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime duration = end - start;
#endif
    LOG(Timing, "Serializing web archive to raw CFPropertyList data took %f seconds", duration);
        
    return [[(NSData *)data.get() retain] autorelease];
}

@end

@implementation WebArchive (WebInternal)

- (id)_initWithCoreLegacyWebArchive:(PassRefPtr<WebCore::LegacyWebArchive>)coreLegacyWebArchive
{
    WebCoreThreadViolationCheckRoundTwo();

    self = [super init];
    if (!self)
        return nil;
    
    _private = [[WebArchivePrivate alloc] initWithCoreArchive:coreLegacyWebArchive];
    if (!_private) {
        [self release];
        return nil;
    }

    return self;
}

- (WebCore::LegacyWebArchive *)_coreLegacyWebArchive
{
    WebCoreThreadViolationCheckRoundTwo();

    return [_private coreArchive];
}

@end
