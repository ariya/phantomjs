/*
 * Copyright (C) 2004, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Jonas Witt <jonas.witt@gmail.com>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source ec must retain the above copyright
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

#import "config.h"
#import "DOMEventInternal.h"

#import "DOMBeforeLoadEvent.h"
#import "DOMKeyboardEvent.h"
#import "DOMMessageEvent.h"
#import "DOMMouseEvent.h"
#import "DOMMutationEvent.h"
#import "DOMOverflowEvent.h"
#import "DOMProgressEvent.h"
#import "DOMTextEvent.h"
#import "DOMWheelEvent.h"
#import "Event.h"
#import "EventNames.h"

using WebCore::eventNames;

Class kitClass(WebCore::Event* impl)
{
    if (impl->isUIEvent()) {
        if (impl->isKeyboardEvent())
            return [DOMKeyboardEvent class];
        if (impl->isMouseEvent())
            return [DOMMouseEvent class];
        if (impl->hasInterface(eventNames().interfaceForTextEvent))
            return [DOMTextEvent class];
        if (impl->hasInterface(eventNames().interfaceForWheelEvent))
            return [DOMWheelEvent class];        
        return [DOMUIEvent class];
    }
    if (impl->hasInterface(eventNames().interfaceForMutationEvent))
        return [DOMMutationEvent class];
    if (impl->hasInterface(eventNames().interfaceForOverflowEvent))
        return [DOMOverflowEvent class];
    if (impl->hasInterface(eventNames().interfaceForMessageEvent))
        return [DOMMessageEvent class];
    if (impl->hasInterface(eventNames().interfaceForProgressEvent) || impl->hasInterface(eventNames().interfaceForXMLHttpRequestProgressEvent))
        return [DOMProgressEvent class];
    if (impl->hasInterface(eventNames().interfaceForBeforeLoadEvent))
        return [DOMBeforeLoadEvent class];
    return [DOMEvent class];
}
