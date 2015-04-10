/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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

#import "WebNetscapeContainerCheckContextInfo.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

@implementation WebNetscapeContainerCheckContextInfo

- (id)initWithCheckRequestID:(uint32_t)checkRequestID callbackFunc:(void (*)(NPP npp, uint32_t checkID, NPBool allowed, void* context))callbackFunc context:(void*)context
{
    self = [super init];
    if (!self)
        return nil;
    
    _checkRequestID = checkRequestID;
    _callback = callbackFunc;
    _context = context;
    return self;
}

- (uint32_t)checkRequestID
{
    return _checkRequestID;   
}

- (void (*)(NPP npp, uint32_t, NPBool, void*))callback
{
    return _callback;
}

- (void*)context
{
    return _context;
}

@end

#endif // ENABLE(NETSCAPE_PLUGIN_API)
