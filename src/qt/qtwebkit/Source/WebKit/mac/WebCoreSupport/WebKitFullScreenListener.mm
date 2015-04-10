/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebKitFullScreenListener.h"

#import <WebCore/Element.h>

#if ENABLE(FULLSCREEN_API)

using namespace WebCore;

@implementation WebKitFullScreenListener

- (id)initWithElement:(Element*)element
{
    if (!(self = [super init]))
        return nil;

    _element = element;
    return self;
}

- (void)webkitWillEnterFullScreen
{
    if (_element)
        _element->document()->webkitWillEnterFullScreenForElement(_element.get());
}

- (void)webkitDidEnterFullScreen
{
    if (_element)
        _element->document()->webkitDidEnterFullScreenForElement(_element.get());
}

- (void)webkitWillExitFullScreen
{
    if (_element)
        _element->document()->webkitWillExitFullScreenForElement(_element.get());
}

- (void)webkitDidExitFullScreen
{
    if (_element)
        _element->document()->webkitDidExitFullScreenForElement(_element.get());
}

@end
#endif
