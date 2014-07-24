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

#include "config.h"
#include "SharedBuffer.h"

#include "WebCoreObjCExtras.h"
#include <runtime/InitializeThreading.h>
#include <string.h>
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>


using namespace WebCore;

@interface WebCoreSharedBufferData : NSData
{
    RefPtr<SharedBuffer> sharedBuffer;
}

- (id)initWithSharedBuffer:(SharedBuffer*)buffer;
@end

@implementation WebCoreSharedBufferData

+ (void)initialize
{
#if !USE(WEB_THREAD)
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
#endif // !USE(WEB_THREAD)
    WebCoreObjCFinalizeOnMainThread(self);
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebCoreSharedBufferData class], self))
        return;
    
    [super dealloc];
}

- (void)finalize
{
    [super finalize];
}

- (id)initWithSharedBuffer:(SharedBuffer*)buffer
{
    self = [super init];
    
    if (self)
        sharedBuffer = buffer;
    
    return self;
}

- (NSUInteger)length
{
    return sharedBuffer->size();
}

- (const void *)bytes
{
    return reinterpret_cast<const void*>(sharedBuffer->data());
}

@end

namespace WebCore {

PassRefPtr<SharedBuffer> SharedBuffer::wrapNSData(NSData *nsData)
{
    return adoptRef(new SharedBuffer((CFDataRef)nsData));
}

NSData *SharedBuffer::createNSData()
{    
    return [[WebCoreSharedBufferData alloc] initWithSharedBuffer:this];
}

CFDataRef SharedBuffer::createCFData()
{
    if (m_cfData) {
        CFRetain(m_cfData.get());
        return m_cfData.get();
    }
    
    return (CFDataRef)adoptNS([[WebCoreSharedBufferData alloc] initWithSharedBuffer:this]).leakRef();
}

PassRefPtr<SharedBuffer> SharedBuffer::createWithContentsOfFile(const String& filePath)
{
    NSData *resourceData = [NSData dataWithContentsOfFile:filePath];
    if (resourceData) 
        return SharedBuffer::wrapNSData(resourceData);
    return 0;
}

}
