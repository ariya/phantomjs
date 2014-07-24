/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "RunLoop.h"

namespace WebCore {

static bool s_useApplicationRunLoopOnMainRunLoop;

void RunLoop::setUseApplicationRunLoopOnMainRunLoop()
{
    s_useApplicationRunLoopOnMainRunLoop = true;
}

void RunLoop::run()
{
    current()->m_nestingLevel++;
    if (current() == main() && current()->m_nestingLevel == 1 && s_useApplicationRunLoopOnMainRunLoop) {
        // Use -[NSApplication run] for the main run loop.
        [NSApp run];
    } else {
        @autoreleasepool {
            CFRunLoopRun();
        }
    }
    current()->m_nestingLevel--;
}

void RunLoop::stop()
{
    ASSERT(m_runLoop == CFRunLoopGetCurrent());

    // Nesting level can be 0 if the run loop is started externally (such as the case for XPC services).
    if (m_runLoop == main()->m_runLoop && m_nestingLevel <= 1 && s_useApplicationRunLoopOnMainRunLoop) {
        ASSERT([NSApp isRunning]);
        [NSApp stop:nil];
        NSEvent *event = [NSEvent otherEventWithType:NSApplicationDefined
                                            location:NSMakePoint(0, 0)
                                       modifierFlags:0
                                           timestamp:0.0
                                         windowNumber:0
                                             context:nil
                                             subtype: 0
                                               data1:0
                                               data2:0];
        [NSApp postEvent:event atStart:true];
    } else
        CFRunLoopStop(m_runLoop.get());
}

} // namespace WebCore
