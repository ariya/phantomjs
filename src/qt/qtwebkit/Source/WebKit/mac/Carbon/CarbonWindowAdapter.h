/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <HIToolbox/CarbonEvents.h>
#import <HIToolbox/MacWindows.h>

@interface CarbonWindowAdapter : NSWindow 
{
@private

    // The Carbon window that's being encapsulated, and whether or not this object owns (has responsibility for disposing) it.
    WindowRef _windowRef;
    BOOL _windowRefIsOwned;
    BOOL _carbon;

    // The UPP for the event handler that we use to deal with various Carbon events, and the event handler itself.
    EventHandlerUPP _handleEventUPP;
    EventHandlerRef _eventHandler;
    
    // Yes if this object should let Carbon handle kEventWindowActivated and kEventWindowDeactivated events.  No otherwise.
    BOOL _passingCarbonWindowActivationEvents;
}

// Initializers.
- (id)initWithCarbonWindowRef:(WindowRef)inWindowRef takingOwnership:(BOOL)inWindowRefIsOwned;
- (id)initWithCarbonWindowRef:(WindowRef)inWindowRef takingOwnership:(BOOL)inWindowRefIsOwned disableOrdering:(BOOL)inDisableOrdering carbon:(BOOL)inCarbon;

// Accessors.
- (WindowRef)windowRef;

// Update this window's frame and content rectangles to match the Carbon window's structure and content bounds rectangles.  Return yes if the update was really necessary, no otherwise.
- (BOOL)reconcileToCarbonWindowBounds;

// Handle an event just like an NSWindow would.
- (void)sendSuperEvent:(NSEvent *)inEvent;

- (void)relinquishFocus;

@end
