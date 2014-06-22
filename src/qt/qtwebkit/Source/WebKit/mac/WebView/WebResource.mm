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

#import "WebResourceInternal.h"

#import "WebFrameInternal.h"
#import "WebKitLogging.h"
#import "WebKitVersionChecks.h"
#import "WebNSDictionaryExtras.h"
#import "WebNSObjectExtras.h"
#import "WebNSURLExtras.h"
#import <JavaScriptCore/InitializeThreading.h>
#import <wtf/PassRefPtr.h>
#import <WebCore/ArchiveResource.h>
#import <WebCore/LegacyWebArchive.h>
#import <WebCore/RunLoop.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/TextEncoding.h>
#import <WebCore/ThreadCheck.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/WebCoreURLResponse.h>
#import <wtf/MainThread.h>

using namespace WebCore;

static NSString * const WebResourceDataKey =              @"WebResourceData";
static NSString * const WebResourceFrameNameKey =         @"WebResourceFrameName";
static NSString * const WebResourceMIMETypeKey =          @"WebResourceMIMEType";
static NSString * const WebResourceURLKey =               @"WebResourceURL";
static NSString * const WebResourceTextEncodingNameKey =  @"WebResourceTextEncodingName";
static NSString * const WebResourceResponseKey =          @"WebResourceResponse";

@interface WebResourcePrivate : NSObject {
@public
    ArchiveResource* coreResource;
}
- (id)initWithCoreResource:(PassRefPtr<ArchiveResource>)coreResource;
@end

@implementation WebResourcePrivate

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
    WebCoreObjCFinalizeOnMainThread(self);
}

- (id)init
{
    return [super init];
}

- (id)initWithCoreResource:(PassRefPtr<ArchiveResource>)passedResource
{
    self = [super init];
    if (!self)
        return nil;
    // Acquire the PassRefPtr<>'s ref as our own manual ref
    coreResource = passedResource.leakRef();
    return self;
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebResourcePrivate class], self))
        return;

    if (coreResource)
        coreResource->deref();
    [super dealloc];
}

- (void)finalize
{
    if (coreResource)
        coreResource->deref();
    [super finalize];
}

@end

@implementation WebResource

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    _private = [[WebResourcePrivate alloc] init];
    return self;
}

- (id)initWithData:(NSData *)data URL:(NSURL *)URL MIMEType:(NSString *)MIMEType textEncodingName:(NSString *)textEncodingName frameName:(NSString *)frameName
{
    return [self _initWithData:data URL:URL MIMEType:MIMEType textEncodingName:textEncodingName frameName:frameName response:nil copyData:YES];
}

- (id)initWithCoder:(NSCoder *)decoder
{
    WebCoreThreadViolationCheckRoundTwo();

    self = [super init];
    if (!self)
        return nil;

    NSData *data = nil;
    NSURL *url = nil;
    NSString *mimeType = nil, *textEncoding = nil, *frameName = nil;
    NSURLResponse *response = nil;
    
    @try {
        id object = [decoder decodeObjectForKey:WebResourceDataKey];
        if ([object isKindOfClass:[NSData class]])
            data = object;
        object = [decoder decodeObjectForKey:WebResourceURLKey];
        if ([object isKindOfClass:[NSURL class]])
            url = object;
        object = [decoder decodeObjectForKey:WebResourceMIMETypeKey];
        if ([object isKindOfClass:[NSString class]])
            mimeType = object;
        object = [decoder decodeObjectForKey:WebResourceTextEncodingNameKey];
        if ([object isKindOfClass:[NSString class]])
            textEncoding = object;
        object = [decoder decodeObjectForKey:WebResourceFrameNameKey];
        if ([object isKindOfClass:[NSString class]])
            frameName = object;
        object = [decoder decodeObjectForKey:WebResourceResponseKey];
        if ([object isKindOfClass:[NSURLResponse class]])
            response = object;
    } @catch(id) {
        [self release];
        return nil;
    }

    _private = [[WebResourcePrivate alloc] initWithCoreResource:ArchiveResource::create(SharedBuffer::wrapNSData(data), url, mimeType, textEncoding, frameName, response)];

    return self;
}

- (void)encodeWithCoder:(NSCoder *)encoder
{
    ArchiveResource *resource = _private->coreResource;
    
    NSData *data = nil;
    NSURL *url = nil;
    NSString *mimeType = nil, *textEncoding = nil, *frameName = nil;
    NSURLResponse *response = nil;
    
    if (resource) {
        if (resource->data())
            data = [resource->data()->createNSData() autorelease];
        url = resource->url();
        mimeType = resource->mimeType();
        textEncoding = resource->textEncoding();
        frameName = resource->frameName();
        response = resource->response().nsURLResponse();
    }
    [encoder encodeObject:data forKey:WebResourceDataKey];
    [encoder encodeObject:url forKey:WebResourceURLKey];
    [encoder encodeObject:mimeType forKey:WebResourceMIMETypeKey];
    [encoder encodeObject:textEncoding forKey:WebResourceTextEncodingNameKey];
    [encoder encodeObject:frameName forKey:WebResourceFrameNameKey];
    [encoder encodeObject:response forKey:WebResourceResponseKey];
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

- (NSData *)data
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] data];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    if (!_private->coreResource)
        return nil;
    if (!_private->coreResource->data())
        return nil;
    return [_private->coreResource->data()->createNSData() autorelease];
}

- (NSURL *)URL
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] URL];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    if (!_private->coreResource)
        return nil;
    NSURL *url = _private->coreResource->url();
    return url;
}

- (NSString *)MIMEType
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] MIMEType];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    if (!_private->coreResource)
        return nil;
    NSString *mimeType = _private->coreResource->mimeType();
    return mimeType;
}

- (NSString *)textEncodingName
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] textEncodingName];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    if (!_private->coreResource)
        return nil;
    NSString *textEncodingName = _private->coreResource->textEncoding();
    return textEncodingName;
}

- (NSString *)frameName
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] frameName];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    if (!_private->coreResource)
        return nil;
    NSString *frameName = _private->coreResource->frameName();
    return frameName;
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %@>", [self className], [self URL]];
}

@end

@implementation WebResource (WebResourceInternal)

- (id)_initWithCoreResource:(PassRefPtr<ArchiveResource>)coreResource
{
    self = [super init];
    if (!self)
        return nil;
            
    ASSERT(coreResource);
    
    // WebResources should not be init'ed with nil data, and doing so breaks certain uses of NSHTMLReader
    // See <rdar://problem/5820157> for more info
    if (!coreResource->data()) {
        [self release];
        return nil;
    }
    
    _private = [[WebResourcePrivate alloc] initWithCoreResource:coreResource];
            
    return self;
}

- (WebCore::ArchiveResource *)_coreResource
{
    return _private->coreResource;
}

@end

@implementation WebResource (WebResourcePrivate)

// SPI for Mail (5066325)
// FIXME: This "ignoreWhenUnarchiving" concept is an ugly one - can we find a cleaner solution for those who need this SPI?
- (void)_ignoreWhenUnarchiving
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround()) {
        [[self _webkit_invokeOnMainThread] _ignoreWhenUnarchiving];
        return;
    }
#endif

    WebCoreThreadViolationCheckRoundTwo();

    if (!_private->coreResource)
        return;
    _private->coreResource->ignoreWhenUnarchiving();
}

- (id)_initWithData:(NSData *)data 
                URL:(NSURL *)URL 
           MIMEType:(NSString *)MIMEType 
   textEncodingName:(NSString *)textEncodingName 
          frameName:(NSString *)frameName 
           response:(NSURLResponse *)response
           copyData:(BOOL)copyData
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] _initWithData:data URL:URL MIMEType:MIMEType textEncodingName:textEncodingName frameName:frameName response:response copyData:copyData];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    self = [super init];
    if (!self)
        return nil;
    
    if (!data || !URL || !MIMEType) {
        [self release];
        return nil;
    }

    _private = [[WebResourcePrivate alloc] initWithCoreResource:ArchiveResource::create(SharedBuffer::wrapNSData(copyData ? [[data copy] autorelease] : data), URL, MIMEType, textEncodingName, frameName, response)];

    return self;
}

- (id)_initWithData:(NSData *)data URL:(NSURL *)URL response:(NSURLResponse *)response
{
    // Pass NO for copyData since the data doesn't need to be copied since we know that callers will no longer modify it.
    // Copying it will also cause a performance regression.
    return [self _initWithData:data
                           URL:URL
                      MIMEType:[response MIMEType]
              textEncodingName:[response textEncodingName]
                     frameName:nil
                      response:response
                      copyData:NO];
}

- (NSString *)_suggestedFilename
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] _suggestedFilename];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    if (!_private->coreResource)
        return nil;
    NSString *suggestedFilename = _private->coreResource->response().suggestedFilename();
    return suggestedFilename;
}

- (NSFileWrapper *)_fileWrapperRepresentation
{
    NSFileWrapper *wrapper = [[[NSFileWrapper alloc] initRegularFileWithContents:[self data]] autorelease];
    NSString *filename = [self _suggestedFilename];
    if (!filename || ![filename length])
        filename = [[self URL] _webkit_suggestedFilenameWithMIMEType:[self MIMEType]];
    [wrapper setPreferredFilename:filename];
    return wrapper;
}

- (NSURLResponse *)_response
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] _response];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    NSURLResponse *response = nil;
    if (_private->coreResource)
        response = _private->coreResource->response().nsURLResponse();
    return response ? response : [[[NSURLResponse alloc] init] autorelease];        
}

- (NSString *)_stringValue
{
#ifdef MAIL_THREAD_WORKAROUND
    if (needMailThreadWorkaround())
        return [[self _webkit_invokeOnMainThread] _stringValue];
#endif

    WebCoreThreadViolationCheckRoundTwo();

    WebCore::TextEncoding encoding;
    if (_private->coreResource)
        encoding = _private->coreResource->textEncoding();
    if (!encoding.isValid())
        encoding = WindowsLatin1Encoding();
    
    SharedBuffer* coreData = _private->coreResource ? _private->coreResource->data() : 0;
    return encoding.decode(reinterpret_cast<const char*>(coreData ? coreData->data() : 0), coreData ? coreData->size() : 0);
}

@end

#ifdef MAIL_THREAD_WORKAROUND

static const double newMailBundleVersion = 1050.0;

@implementation WebResource (WebMailThreadWorkaround)

+ (BOOL)_needMailThreadWorkaroundIfCalledOffMainThread
{
    static BOOL isOldMail = applicationIsAppleMail() && [[[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey] doubleValue] < newMailBundleVersion;
    return isOldMail;
}

@end

#endif
