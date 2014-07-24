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

// I don't think this class belongs in WebKit. Lets move it out.

// Things that I've never bothered working out:
// For non-sheet windows, handle Carbon WindowMove events so as to do the same things as -[NSWindow _windowMoved].
// Check to see how this stuff deals with various screen size change scenarious.
// M.P. Warning - 9/17/01

// There are some invariants I'm maintaining for objects of this class which have been successfully initialized but not deallocated.  These all make it easier to not override every single method of NSWindow.
// _auxiliaryStorage->auxWFlags.hasShadow will always be false if the Carbon window has a kWindowNoShadowAttribute, and vice versa.
// _auxiliaryStorage->_auxWFlags.minimized will always reflect the window's Carbon collapsed state.
// _borderView will always point to an NSCarbonWindowFrame.
// _contentView will always point to an NSCarbonWindowContentView;
// _frame will always reflect the window's Carbon kWindowStructureRgn bounds.
// _styleMask will always have _NSCarbonWindowMask set, and will have NSClosableWindowMask, NSMiniaturizableWindowMask, NSResizableWindowMask, and/or NSTitledWindowMask set as appropriate.
// _wflags.oneShot and _wflags.delayedOneShot will always be false.
// _wFlags.visible will always reflect the window's Carbon visibility.
// _windowNum will always be greater than zero, and valid.
// The instance variables involved are ones that came to my attention during the initial writing of this class; I haven't methodically gone through NSWindow's ivar list or anything like that.  M.P. Notice - 10/10/00

// Things that have to be worked on if NSCarbonWindows are ever used for something other than dialogs and sheets:
// Clicking on an NSCarbonWindow while a Cocoa app-modal dialog is shown does not beep, as it should [old bug, maybe fixed now].
// Handling of mouse clicks or key presses for any window control (close, miniaturize, zoom) might not be all there.
// Handling of miniaturization of Carbon windows via title bar double-click might not be all there.
// The background on NSCarbonWindowTester's sample window (not sample dialog or sample sheet) might be wrong.
// The controls on NSCarbonWindowTester's sample window look inactive when the window is inactive, but have first-click behavior.
// M.P. Warning - 12/14/00

// Some things would have to be made public if someone wanted to subclass this so as to support more menu item commands.  M.P. Warning - 9/19/00

#ifndef __LP64__

#import "CarbonWindowAdapter.h"

#import "CarbonWindowFrame.h"
#import "CarbonWindowContentView.h"
#import "HIViewAdapter.h"

#import <WebKitSystemInterface.h>

#import <AppKit/AppKit.h>
//#import <CoreGraphics/CGSWindow.h>
#import <HIToolbox/CarbonEvents.h>
#import <HIToolbox/Controls.h>
#import <HIToolbox/HIView.h>
#import <assert.h>

#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/RunLoop.h>
#import <runtime/InitializeThreading.h>
#import <wtf/MainThread.h>

#import "WebKitLogging.h"
#import "WebNSObjectExtras.h"
#import "WebTypesInternal.h"

@interface NSWindow(HIWebFrameView)
- (id)_initContent:(const NSRect *)contentRect styleMask:(unsigned int)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag contentView:aView;
- (void)_oldPlaceWindow:(NSRect)frameRect;
- (void)_windowMovedToRect:(NSRect)actualFrame;
- (void)_setWindowNumber:(NSInteger)nativeWindow;
- (NSGraphicsContext *)_threadContext;
- (void)_setFrame:(NSRect)newWindowFrameRect;
- (void)_setVisible:(BOOL)flag;
@end

@interface NSApplication(HIWebFrameView)
- (void)setIsActive:(BOOL)aFlag;
- (id)_setMouseActivationInProgress:(BOOL)flag;
- (BOOL)_handleKeyEquivalent:(NSEvent*)theEvent;
@end

@interface NSInputContext
- (BOOL)processInputKeyBindings:(NSEvent *)event;
@end

// Forward declarations.
static OSStatus NSCarbonWindowHandleEvent(EventHandlerCallRef inEventHandlerCallRef, EventRef inEventRef, void *inUserData);

@implementation CarbonWindowAdapter


// Return an appropriate window frame class.
+ (Class)frameViewClassForStyleMask:(unsigned int)style {

    // There's only one appropriate window style, and only one appropriate window frame class.
    assert(style & WKCarbonWindowMask());
    return [CarbonWindowFrame class];

}


// Overriding of the parent class' designated initializer, just for safety's sake.
- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int)style backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag {

    // Do the standard Cocoa thing.
    self = [super initWithContentRect:contentRect styleMask:style backing:bufferingType defer:flag];
    if (self==nil) return nil;

    // Simple.
    _windowRef = NULL;
    _windowRefIsOwned = NO;
    _eventHandler = NULL;
    
    // Done.
    return self;

}

// Given a reference to a Carbon window that is to be encapsulated, an indicator of whether or not this object should take responsibility for disposing of the Carbon window, and an indicator of whether to disable Carbon window ordering, initialize.  This is the class' designated initializer.
- (id)initWithCarbonWindowRef:(WindowRef)inWindowRef takingOwnership:(BOOL)inWindowRefIsOwned disableOrdering:(BOOL)inDisableOrdering carbon:(BOOL)inCarbon {

    NSBackingStoreType backingStoreType;
    CarbonWindowContentView *carbonWindowContentView;
    NSWindow *windowAsProperty;
    OSStatus osStatus;
    UInt32 windowFeatures;
    WindowAttributes windowAttributes;
    unsigned int styleMask;
    void *nativeWindow;
    WindowModality windowModality;
	ControlRef		contentView;
	
    // Simple.
    // It's very weak to have to put this before the invocation of [super initWithContentRect:...], but -setContentView: is invoked from within that initializer.  It turns out that the common admonition about not calling virtual functions from within C++ constructors makes sense in Objective-C too.  M.P. Notice - 10/10/00
    _windowRef = inWindowRef;
    //_auxiliaryStorage->_windowRef = inWindowRef;
    _windowRefIsOwned = inWindowRefIsOwned;
	_carbon = inCarbon;

    // Find out the window's CoreGraphics window reference.
    nativeWindow = WKGetNativeWindowFromWindowRef(inWindowRef);

    // Find out the window's Carbon window attributes.
    GetWindowAttributes(inWindowRef, &windowAttributes);

    // Find out the window's Carbon window features.
    GetWindowFeatures(inWindowRef, &windowFeatures);

    // Figure out the window's backing store type.
    // At one time, this had code stolen from CreatePlatformWindow in HIToolbox/Windows/Platform/CGSPlatform.c
	// But now the non-retained window class is a Carbon secret that's not even in
	// WindowsPriv.h; maybe we'll have to revisit this if someone needs to use WebKit
	// in a non-retained window.
    backingStoreType = NSBackingStoreRetained;

    // Figure out the window's style mask.
    styleMask = WKCarbonWindowMask();
    if (windowAttributes & kWindowCloseBoxAttribute) styleMask |= NSClosableWindowMask;
    if (windowAttributes & kWindowResizableAttribute) styleMask |= NSResizableWindowMask;
    if (windowFeatures & kWindowCanCollapse) styleMask |= NSMiniaturizableWindowMask;
    if (windowFeatures & kWindowHasTitleBar) styleMask |= NSTitledWindowMask;

    osStatus = GetWindowModality(_windowRef, &windowModality, NULL);
    if (osStatus != noErr) {
        NSLog(@"Couldn't get window modality: error=%ld", osStatus);
        return nil;
    }
    
    // Create one of our special content views.
    carbonWindowContentView = [[[CarbonWindowContentView alloc] init] autorelease];

    // Do some standard Cocoa initialization.  The defer argument's value is YES because we don't want -[NSWindow _commonAwake] to get called.  It doesn't appear that any relevant NSWindow code checks _wFlags.deferred, so we should be able to get away with the lie.
    self = (CarbonWindowAdapter*)[super _initContent:NULL styleMask:styleMask backing:backingStoreType defer:YES contentView:carbonWindowContentView];
    if (!self) return nil;
    assert(_contentView);

    // Record accurately whether or not this window has a shadow, in case someone asks.
 //   _auxiliaryStorage->_auxWFlags.hasShadow = (windowAttributes & kWindowNoShadowAttribute) ? NO : YES;

    // Record the window number.
    [self _setWindowNumber:(NSInteger)nativeWindow];

    // Set up from the frame rectangle.
    // We didn't even really try to get it right at _initContent:... time, because it's more trouble that it's worth to write a real +[NSCarbonWindow frameRectForContentRect:styleMask:].  M.P. Notice - 10/10/00
    [self reconcileToCarbonWindowBounds];

    // Install an event handler for the Carbon window events in which we're interested.
    const EventTypeSpec kEvents[] = {
            { kEventClassWindow, kEventWindowActivated },
            { kEventClassWindow, kEventWindowDeactivated },
            { kEventClassWindow, kEventWindowBoundsChanged },
            { kEventClassWindow, kEventWindowShown },
            { kEventClassWindow, kEventWindowHidden }
    };
    
    const EventTypeSpec kControlBoundsChangedEvent = { kEventClassControl, kEventControlBoundsChanged };
    
    osStatus = InstallEventHandler( GetWindowEventTarget(_windowRef), NSCarbonWindowHandleEvent, GetEventTypeCount( kEvents ), kEvents, (void*)self, &_eventHandler);
    if (osStatus!=noErr) {
            [self release];
            return nil;
    }

    osStatus = InstallEventHandler( GetControlEventTarget( HIViewGetRoot( _windowRef ) ), NSCarbonWindowHandleEvent, 1, &kControlBoundsChangedEvent, (void*)self, &_eventHandler);
    if (osStatus!=noErr) {
            [self release];
            return nil;
    }

    HIViewFindByID( HIViewGetRoot( _windowRef ), kHIViewWindowContentID, &contentView );
    osStatus = InstallEventHandler( GetControlEventTarget( contentView ), NSCarbonWindowHandleEvent, 1, &kControlBoundsChangedEvent, (void*)self, &_eventHandler);
    if (osStatus!=noErr) {
            [self release];
            return nil;
    }
	
    // Put a pointer to this Cocoa NSWindow in a Carbon window property tag.
    // Right now, this is just used by NSViewCarbonControl.  M.P. Notice - 10/9/00
    windowAsProperty = self;
    osStatus = SetWindowProperty(_windowRef, WKCarbonWindowPropertyCreator(), WKCarbonWindowPropertyTag(), sizeof(NSWindow *), &windowAsProperty);
    if (osStatus!=noErr) {
        [self release];
        return nil;
    }

    // Ignore the Carbon window activation/deactivation events that Carbon sends to its windows at app activation/deactivation.  We'll send such events when we think it's appropriate.
    _passingCarbonWindowActivationEvents = NO;

    // Be sure to sync up visibility
    [self _setVisible:(BOOL)IsWindowVisible( _windowRef )];

    // Done.
    return self;

}

- (void)setViewsNeedDisplay:(BOOL)wellDoThey {
	// Make sure we can flush anything that needs it.

	// We need to sync the context here. I was hoping I didn't need to do this,
	// but apparently when scrolling, the AppKit view system draws directly.
	// When this occurs, I cannot intercept it to make it draw in my HIView
	// context. What ends up happening is that it draws, but nothing ever
	// flushes it.

	if ( [self windowNumber] != -1 )
	{
		CGContextRef cgContext = (CGContextRef)[[self _threadContext] graphicsPort];
		CGContextSynchronize( cgContext );
	}
}

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
    WebCoreObjCFinalizeOnMainThread(self);
}

// Given a reference to a Carbon window that is to be encapsulated, and an indicator of whether or not this object should take responsibility for disposing of the Carbon window, initialize.
- (id)initWithCarbonWindowRef:(WindowRef)inWindowRef takingOwnership:(BOOL)inWindowRefIsOwned {
    // for now, set disableOrdering to YES because that is what we've been doing and is therefore lower risk. However, I think it would be correct to set it to NO.
    return [self initWithCarbonWindowRef:inWindowRef takingOwnership:inWindowRefIsOwned disableOrdering:YES carbon:NO];
}


// Clean up.
- (void)dealloc {
    if (WebCoreObjCScheduleDeallocateOnMainThread([CarbonWindowAdapter class], self))
        return;

    // Clean up, if necessary.
    // if we didn't remove the event handler at dealloc time, we would risk getting sent events after the window has been deallocated.  See 2702179.  M.P. Notice - 6/1/01
    if (_eventHandler) RemoveEventHandler(_eventHandler);

    // Do the standard Cocoa thing.
    [super dealloc];
}

- (void)finalize {
    ASSERT_MAIN_THREAD();
    if (_eventHandler) RemoveEventHandler(_eventHandler);
    [super finalize];
}

- (WindowRef)windowRef {

    // Simple.
    return _windowRef;

}

// should always be YES, but check in order to avoid initialization or deallocation surprises
- (BOOL)_hasWindowRef {
    return (_windowRef != NULL);
}

// an NSCarbonWindow does not manage the windowRef.  The windowRef manages the NSCarbonWindow
- (BOOL)_managesWindowRef {
    return NO;
}

- (void)_removeWindowRef {
    _windowRef = NULL;
	
    if (_eventHandler) RemoveEventHandler(_eventHandler);
	
	_eventHandler = NULL;
}

- (WindowClass)_carbonWindowClass {
    WindowClass windowClass = kDocumentWindowClass;
    OSStatus osStatus;
    
    if ([self _hasWindowRef]) {
        osStatus = GetWindowClass([self windowRef], &windowClass);
        if (osStatus != noErr) {
            NSLog(@"Couldn't get window class: error=%ld", osStatus);
        }
    }
    return windowClass; 
}

// Update this window's frame and content frame rectangles to match the Carbon window's structure bounds and content bounds rectangles.  Return yes if the update was really necessary, no otherwise.
- (BOOL)reconcileToCarbonWindowBounds {

    OSStatus osStatus;
    NSRect newContentFrameRect;
    NSRect newWindowFrameRect;
    NSRect oldContentFrameRect;
    Rect windowContentBoundsRect;
    Rect windowStructureBoundsRect;

    // Initialize for safe returning.
    BOOL reconciliationWasNecessary = NO;

    // Precondition check.
    assert(_contentView);

    // Get the Carbon window's bounds, which are expressed in global screen coordinates, with (0,0) at the top-left of the main screen.
    osStatus = GetWindowBounds(_windowRef, kWindowStructureRgn, &windowStructureBoundsRect);
    if (osStatus!=noErr) NSLog(@"A Carbon window's structure bounds couldn't be gotten.");
    osStatus = GetWindowBounds(_windowRef, kWindowContentRgn, &windowContentBoundsRect);
    if (osStatus!=noErr) NSLog(@"A Carbon window's content bounds couldn't be gotten.");

    // Set the frame rectangle of the border view and this window from the Carbon window's structure region bounds.
    newWindowFrameRect.origin.x = windowStructureBoundsRect.left;
    newWindowFrameRect.origin.y = NSMaxY([(NSScreen *)[[NSScreen screens] objectAtIndex:0] frame]) - windowStructureBoundsRect.bottom;
    newWindowFrameRect.size.width = windowStructureBoundsRect.right - windowStructureBoundsRect.left;
    newWindowFrameRect.size.height = windowStructureBoundsRect.bottom - windowStructureBoundsRect.top;
    if (!NSEqualRects(newWindowFrameRect, _frame)) {
        [self _setFrame:newWindowFrameRect];
        [_borderView setFrameSize:newWindowFrameRect.size];
        reconciliationWasNecessary = YES;
    }

    // Set the content view's frame rect from the Carbon window's content region bounds.
    newContentFrameRect.origin.x = windowContentBoundsRect.left - windowStructureBoundsRect.left;
    newContentFrameRect.origin.y = windowStructureBoundsRect.bottom - windowContentBoundsRect.bottom;
    newContentFrameRect.size.width = windowContentBoundsRect.right - windowContentBoundsRect.left;
    newContentFrameRect.size.height = windowContentBoundsRect.bottom - windowContentBoundsRect.top;
    oldContentFrameRect = [(NSView *)_contentView frame];
    if (!NSEqualRects(newContentFrameRect, oldContentFrameRect)) {
        [(NSView *)_contentView setFrame:newContentFrameRect];
        reconciliationWasNecessary = YES;
    }

    // Done.
    return reconciliationWasNecessary;

}


// Handle an event just like an NSWindow would.
- (void)sendSuperEvent:(NSEvent *)inEvent {

    // Filter out a few events that just result in complaints in the log.
    // Ignore some unknown event that gets sent when NSTextViews in printing accessory views are focused.  M.P. Notice - 12/7/00
    BOOL ignoreEvent = NO;
    NSEventType eventType = [inEvent type];
    if (eventType==NSSystemDefined) {
        short eventSubtype = [inEvent subtype];
        if (eventSubtype==7) {
            ignoreEvent = YES;
        }
    } else if (eventType == NSKeyDown) {
        // Handle command-space as [NSApp sendEvent:] does.
        if ([NSInputContext processInputKeyBindings:inEvent]) {
            return;
        }
    }
    
    // Simple.
    if (!ignoreEvent) [super sendEvent:inEvent];
}

- (void)relinquishFocus
{
    NSResponder*  firstResponder;

    // Carbon thinks that a control has the keyboard focus,
    // or we wouldn't be being asked to relinquish focus.

	firstResponder = [self firstResponder];
	if ([firstResponder isKindOfClass:[NSView class]] ){
		// Make the window the first responder, so that no view is the key view.
        [self makeFirstResponder:self];
    }
}

- (BOOL)makeFirstResponder:(NSResponder *)aResponder
{
    // Let NSWindow focus the appropriate NSView.
    if (![super makeFirstResponder:aResponder])
        return NO;

    // Now, if the view we're focusing is in a HIWebView, find the
    // corresponding HIWebView for the NSView, and tell carbon to
    // clear any focused control.
    HIViewRef viewRef = 0;
    NSResponder *firstResponder = [self firstResponder];    
    if ([firstResponder isKindOfClass:[NSView class]]) {
        NSView *view = (NSView *)firstResponder;
        while (view) {
            viewRef = [HIViewAdapter getHIViewForNSView:view];
            if (viewRef)
                break;
            view = [view superview];
        }
    }

    HIViewRef focus;
    GetKeyboardFocus (_windowRef, &focus);
    if (focus != viewRef) {
        SetKeyboardFocus (_windowRef, viewRef, kControlIndicatorPart );
    }

    return YES;
}

// There's no override of _addCursorRect:cursor:forView:, despite the fact that NSWindow's invokes [self windowNumber], because Carbon windows won't have subviews, and therefore won't have cursor rects.


// There's no override of _autoResizeState, despite the fact that NSWindow's operates on _windowNum, because it looks like it might work on Carbon windows as is.


// Disappointingly, -_blockHeartBeat: is not immediately invoked to turn off heartbeating.  Heartbeating is turned off by setting the gDefaultButtonPaused global variable, and then this method is invoked later, if that global is set (at heartbeating time I guess).  Something has to change if we want to hook this up in Carbon windows.  M.P. Warning - 9/17/01
/*
// Do the right thing for a Carbon window.
- (void)_blockHeartBeat:(BOOL)flag {

    ControlRef defaultButton;
    OSStatus osStatus;

    // Do the standard Cocoa thing.
    [super _blockHeartBeat:flag];

    // If there's a default Carbon button in this Carbon window, make it stop pulsing, the Carbon way.
    // This is inspired by HIToolbox/Controls/Definitions/ButtonCDEF.c's ButtonEventHandler().  M.P. Notice - 12/5/00
    osStatus = GetWindowDefaultButton(_windowRef, &defaultButton);
    if (osStatus==noErr && defaultButton) {
        Boolean anotherButtonIsTracking = flag ? TRUE : FALSE;
        osStatus = SetControlData(defaultButton, kControlNoPart, kControlPushButtonAnotherButtonTrackingTag, sizeof(Boolean), &anotherButtonIsTracking);
        if (osStatus==noErr) DrawOneControl(defaultButton);
        else NSLog(@"Some data couldn't be set in a Carbon control.");
    }

}
*/


// Do the right thing for a Carbon window.
- (void)_cancelKey:(id)sender {

    // Most of the time the handling of the cancel key will be done by Carbon, but this method will be invoked if an NSCarbonWindow is wrapping a Carbon window that contains an NSViewCarbonControl, and the escape key or whatever is pressed with an NSTextView focused.  Just do what Carbon would do.
    ControlRef cancelButton;
    GetWindowCancelButton(_windowRef, &cancelButton);
    if (cancelButton) {
        if (IsControlActive(cancelButton)) {
            HIViewSimulateClick(cancelButton, kControlButtonPart, 0, NULL);
        }
    }

}



// Do the right thing for a Carbon window.
- (void)_commonAwake {

    // Complain, because this should never be called.  We insist that -[NSCarbonWindow initWithCarbonWindowRef] is the only valid initializer for instances of this class, and that there's no such thing as a one-shot NSCarbonWindow.
    NSLog(@"-[NSCarbonWindow _commonAwake] is not implemented.");

}


// There's no override of _commonInitFrame:styleMask:backing:defer:, despite the fact that NSWindow's modifies quite a few instance variables, because it gets called in a harmless way if the class instance is properly initialized with -[NSCarbonWindow initWithCarbonWindowRef:takingOwnership:].


// Do the right thing for a Carbon window.
- (id)_destroyRealWindow:(BOOL)orderingOut {

    // Complain, because this should never be called.  We don't support one-shot NSCarbonWindows.
    NSLog(@"-[NSCarbonWindow _destroyRealWindow:] is not implemented.");
    return self;
    
}


// There's no override of _discardCursorRectsForView, despite the fact that NSWindow's invokes [self windowNumber], because Carbon windows won't have subviews, and therefore won't have cursor rects.


// There's no override of _forceFlushWindowToScreen, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of _getPositionFromServer, despite the fact that NSWindow's operates on _windowNum, because it's only called from -[NSApplication _activateWindows], which is hopefully about to become obsolete any second now.


// There's no override of _globalWindowNum, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of _initContent:styleMask:backing:defer:contentView:, despite the fact that NSWindow's modifies _auxiliaryStorage->_auxWFlags.hasShadow, because it will never get called if the class instance is properly initialized with -[NSCarbonWindow initWithCarbonWindowRef:takingOwnership:].


// There's no override of _initContent:styleMask:backing:defer:counterpart:, despite the fact that NSWindow's modifies _auxiliaryStorage->_auxWFlags.hasShadow, because it will never get called if the class instance is properly initialized with -[NSCarbonWindow initWithCarbonWindowRef:takingOwnership:].


// Do what NSWindow would do, but then sychronize the Carbon window structures.
- (void)_oldPlaceWindow:(NSRect)frameRect {

    OSStatus osStatus;

    // Do the standard Cocoa thing.
    [super _oldPlaceWindow:frameRect];

    // Tell Carbon to update its various regions.
    // Despite its name, this function should be called early and often, even if the window isn't visible yet.  2702648.  M.P. Notice - 7/24/01
    osStatus = WKSyncWindowWithCGAfterMove(_windowRef);
    if (osStatus!=noErr) NSLog(@"A Carbon window's bounds couldn't be synchronized (%i).", (int)osStatus);

}


// There's no override of _orderOutAndCalcKeyWithCounter:, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of _realHeartBeatThreadContext, despite the fact that NSWindows's invokes [self windowNumber], because it looks like it might not do anything that will effect a Carbon window.


// There's no override of _registerWithDockIfNeeded, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of _removeCursorRect:cursor:forView:, despite the fact that NSWindow's invokes [self windowNumber], because Carbon windows won't have subviews, and therefore won't have cursor rects.


// There's no override of _setAvoidsActivation:, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of _setFrame:, despite the fact that NSWindow's modifies _frame, because it looks like it might work on Carbon windows as is.  The synchronization of the Carbon window bounds rect to the Cocoa frame rect is done in the overrides of _oldPlaceWindow: and _windowMovedToRect:.


// There's no override of _setFrameCommon:display:stashSize:, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of _setWindowNumber:, despite the fact that NSWindow's modifies _windowNum and invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// Do what NSWindow would do, but for a Carbon window.
// This function is mostly cut-and-pasted from -[NSWindow _termWindowIfOwner].  M.P. Notice - 8/7/00
- (void)_termWindowIfOwner {
    [self _setWindowNumber:-1];
    _wFlags.isTerminating = YES;
    if (_windowRef && _windowRefIsOwned) DisposeWindow(_windowRef);
    // KW - need to clear window shadow state so it gets reset correctly when new window created
//    if ([_borderView respondsToSelector:@selector(setShadowState:)]) {
//        [_borderView setShadowState:kFrameShadowNone];
//    }
    _wFlags.isTerminating = NO;
}


// There's no override of _threadContext, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might not do anything that will effect a Carbon window.


// There's no override of _windowMoved:, despite the fact that NSWindow's operates on _windowNum, because it looks like it might work on Carbon windows as is.


// Do what NSWindow would do, but then sychronize the Carbon window structures.
- (void)_windowMovedToRect:(NSRect)actualFrame {

    OSStatus osStatus;

    // Do the standard Cocoa thing.
    [super _windowMovedToRect:actualFrame];

    // Let Carbon know that the window has been moved, unless this method is being called "early."
    if (_wFlags.visible) {
        osStatus = WKSyncWindowWithCGAfterMove(_windowRef);
        if (osStatus!=noErr) NSLog(@"A Carbon window's bounds couldn't be synchronized (%i).", (int)osStatus);
    }

}

- (NSRect)constrainFrameRect:(NSRect)actualFrame toScreen:(NSScreen *)screen {
    // let Carbon decide window size and position
    return actualFrame;
}

- (void)selectKeyViewFollowingView:(NSView *)aView {
	HIViewRef	view = NULL;
	
	view = [HIViewAdapter getHIViewForNSView:aView];
	
	if ( view )
	{	
		HIViewRef	contentView;
		
		GetRootControl( GetControlOwner( view ), &contentView );
		HIViewAdvanceFocus( contentView, 0 );
	}
	else
	{
		[super selectKeyViewFollowingView:aView];
	}
}

- (void)selectKeyViewPrecedingView:(NSView *)aView {
	HIViewRef	view = NULL;
	
	view = [HIViewAdapter getHIViewForNSView:aView];
	
	if ( view )
	{	
		HIViewRef	contentView;
		
		GetRootControl( GetControlOwner( view ), &contentView );
		HIViewAdvanceFocus( contentView, shiftKey );
	}
	else
	{
		[super selectKeyViewPrecedingView:aView];
	}
}

- (void)makeKeyWindow {
	[NSApp _setMouseActivationInProgress:NO];
	[NSApp setIsActive:YES];
	[super makeKeyWindow];
	WKShowKeyAndMain();
}


// Do the right thing for a Carbon window.
- (BOOL)canBecomeKeyWindow {

    return YES;
}

// Do the right thing for a Carbon window.
- (BOOL)canBecomeMainWindow {
    OSStatus osStatus;
    WindowClass windowClass;
    // By default, Carbon windows cannot become the main window.
    // What about when the default isn't right?  Requiring subclassing seems harsh.  M.P. Warning - 9/17/01
    // KW -  modify this to allow document windows to become main
    // This is primarily to get the right look, so that you don't have two windows that both look active - one Cocoa document and one Carbon document
    osStatus = GetWindowClass(_windowRef, &windowClass);
    return (osStatus == noErr && windowClass == kDocumentWindowClass);
    
}


// There's no override of deminiaturize:, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of disableCursorRects, despite the fact that NSWindow's invokes [self windowNumber], because Carbon windows won't have subviews, and therefore won't have cursor rects.



// There's no override of enableCursorRects, despite the fact that NSWindow's invokes [self windowNumber], because Carbon windows won't have subviews, and therefore won't have cursor rects.


// Do the right thing for a Carbon window.
- (void)encodeWithCoder:(NSCoder *)coder {

    // Actually, this will probably never be implemented.  M.P. Notice - 8/2/00
    NSLog(@"-[NSCarbonWindow encodeWithCoder:] is not implemented.");

}


// There's no override of frame, despite the fact that NSWindow's returns _frame, because _frame is one of the instance variables whose value we're keeping synchronized with the Carbon window.


// Do the right thing for a Carbon window.
- (id)initWithCoder:(NSCoder *)coder {

    // Actually, this will probably never be implemented.  M.P. Notice - 8/2/00
    NSLog(@"-[NSCarbonWindow initWithCoder:] is not implemented.");
    [self release];
    return nil;

}


// There's no override of level, despite the fact that NSWindow's returns _level, because _level is one of the instance variables whose value we're keeping synchronized with the Carbon window.


// There's no override of miniaturize:, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of resizeToScreenWithEvent:, despite the fact that NSWindow's operates on _windowNum.
// It looks like it's only called when an _NSForceResizeEventType event is passed into -[NSWindow sendEvent:], and I can't find any instances of that happening.

/*
// Do the right thing for a Carbon Window.
- (void)sendEvent:(NSEvent *)theEvent {

    // Not all events are handled in the same manner.
    NSEventType eventType = [theEvent type];
    if (eventType==NSAppKitDefined) {

        // Handle the event the Cocoa way.  Carbon won't understand it anyway.
        [super sendEvent:theEvent];

    } 
}
*/

// There's no override of setAcceptsMouseMovedEvents:, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


// There's no override of setBackingType:, despite the fact that NSWindow's invokes [self windowNumber], because it's apparently not expected to do anything anyway, judging from the current implementation of PSsetwindowtype().


// Do what NSWindow would do, but for a Carbon window.
- (void)setContentView:(NSView *)aView {

    NSRect contentFrameRect;
    OSStatus osStatus;
    Rect windowContentBoundsRect;
    
    // Precondition check.
    assert(_borderView);
    assert([_borderView isKindOfClass:[CarbonWindowFrame class]]);
    assert(_windowRef);

    // Parameter check.
    assert(aView);
    assert([aView isKindOfClass:[CarbonWindowContentView class]]);

    // Find out the window's Carbon window structure region (content) bounds.
    osStatus = GetWindowBounds(_windowRef, kWindowContentRgn, &windowContentBoundsRect);
    if (osStatus!=noErr) NSLog(@"A Carbon window's content bounds couldn't be gotten.");
    contentFrameRect.origin = NSZeroPoint;
    contentFrameRect.size.width = windowContentBoundsRect.right - windowContentBoundsRect.left;
    contentFrameRect.size.height = windowContentBoundsRect.bottom - windowContentBoundsRect.top;

    // If the content view is still in some other view hierarchy, pry it free.
    [_contentView removeFromSuperview];
    assert(![_contentView superview]);

    // Record the content view, and size it to this window's content frame.
    _contentView = aView;
    [(NSView *)_contentView setFrame:contentFrameRect];

    // Make the content view a subview of the border view.
    [_borderView addSubview:_contentView];

    // Tell the content view it's new place in the responder chain.
    [_contentView setNextResponder:self];

}


// There's no override of setDepthLimit:, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.


- (BOOL)worksWhenModal {
    WindowClass windowClass = [self _carbonWindowClass];
    return (windowClass == kFloatingWindowClass || windowClass == kUtilityWindowClass);
}

- (void)_setModalWindowLevel {
    return;
}

- (id)_clearModalWindowLevel {
    return nil;
}

// There's no override of setLevel:, despite the fact that NSWindow's invokes [self windowNumber], because it looks like it might work on Carbon windows as is.
// I thought at first that there should be a mapping between Cocoa level and Carbon window class, but experiments convince me that such is not the case.  M.P. Notice - 9/18/00


// There's no override of windowNumber, despite the fact that NSWindow's returns _windowNum, because _windowNum is one of the instance variables whose value we're keeping synchronized with the Carbon window.


- (UInt32)carbonHICommandIDFromActionSelector:(SEL)inActionSelector {

    // Initialize with the default return value.
    UInt32 hiCommandID = 0;

    // Pretty simple, if tedious.
    if (inActionSelector==@selector(clear:)) hiCommandID = kHICommandClear;
    else if (inActionSelector==@selector(copy:)) hiCommandID = kHICommandCopy;
    else if (inActionSelector==@selector(cut:)) hiCommandID = kHICommandCut;
    else if (inActionSelector==@selector(paste:)) hiCommandID = kHICommandPaste;
    else if (inActionSelector==@selector(redo:)) hiCommandID = kHICommandRedo;
    else if (inActionSelector==@selector(selectAll:)) hiCommandID = kHICommandSelectAll;
    else if (inActionSelector==@selector(undo:)) hiCommandID = kHICommandUndo;

    // Done.
    return hiCommandID;

}


- (void)sendCarbonProcessHICommandEvent:(UInt32)inHICommandID {

    EventTargetRef eventTargetRef;
    HICommand hiCommand;
    OSStatus osStatus;

    // Initialize for safe error handling.
    EventRef eventRef = NULL;

    // Create a Process Command event.  Don't mention anything about the menu item, because we don't want the Carbon Event handler fiddling with it.
    hiCommand.attributes = 0;
    hiCommand.commandID = inHICommandID;
    hiCommand.menu.menuRef = 0;
    hiCommand.menu.menuItemIndex = 0;
    osStatus = CreateEvent(NULL, kEventClassCommand, kEventCommandProcess, GetCurrentEventTime(), kEventAttributeNone, &eventRef);
    if (osStatus!=noErr) {
        NSLog(@"CreateEvent() returned %i.", (int)osStatus);
        goto CleanUp;  
    }
    osStatus = SetEventParameter(eventRef, kEventParamDirectObject, typeHICommand, sizeof(HICommand), &hiCommand);
    if (osStatus!=noErr) {
        NSLog(@"SetEventParameter() returned %i.", (int)osStatus);
        goto CleanUp;
    }

    // Send a Carbon event to whatever has the Carbon user focus.
    eventTargetRef = GetUserFocusEventTarget();
    osStatus = SendEventToEventTarget(eventRef, eventTargetRef);
    if (osStatus!=noErr) {
        NSLog(@"SendEventToEventTarget() returned %i.", (int)osStatus);
        goto CleanUp;
    }

CleanUp:

    // Clean up.
    if (eventRef) ReleaseEvent(eventRef);

}


- (Boolean)sendCarbonUpdateHICommandStatusEvent:(UInt32)inHICommandID withMenuRef:(MenuRef)inMenuRef andMenuItemIndex:(UInt16)inMenuItemIndex {

    EventTargetRef eventTargetRef;
    HICommand hiCommand;
    OSStatus osStatus;

    // Initialize for safe error handling and flag returning.
    Boolean eventWasHandled = FALSE;
    EventRef eventRef = NULL;

    // Create a Process Command event.  Don't mention anything about the menu item, because we don't want the Carbon Event handler fiddling with it.
    hiCommand.attributes = kHICommandFromMenu;
    hiCommand.commandID = inHICommandID;
    hiCommand.menu.menuRef = inMenuRef;
    hiCommand.menu.menuItemIndex = inMenuItemIndex;
    osStatus = CreateEvent(NULL, kEventClassCommand, kEventCommandUpdateStatus, GetCurrentEventTime(), kEventAttributeNone, &eventRef);
    if (osStatus!=noErr) {
        NSLog(@"CreateEvent() returned %i.", (int)osStatus);
        goto CleanUp;
    }
    osStatus = SetEventParameter(eventRef, kEventParamDirectObject, typeHICommand, sizeof(HICommand), &hiCommand);
    if (osStatus!=noErr) {
        NSLog(@"SetEventParameter() returned %i.", (int)osStatus);
        goto CleanUp;
    }

    // Send a Carbon event to whatever has the Carbon user focus.
    eventTargetRef = GetUserFocusEventTarget();
    osStatus = SendEventToEventTarget(eventRef, eventTargetRef);
    if (osStatus==noErr) {
        eventWasHandled = TRUE;
    } else if (osStatus!=eventNotHandledErr) {
        NSLog(@"SendEventToEventTarget() returned %i.", (int)osStatus);
        goto CleanUp;
    }

CleanUp:

    // Clean up.
    if (eventRef) ReleaseEvent(eventRef);

    // Done.
    return eventWasHandled;

}

- (void)_handleRootBoundsChanged
{
	HIViewRef	root = HIViewGetRoot( _windowRef ); 
	HIRect		frame;

	HIViewGetFrame( root, &frame );
	[_borderView setFrameSize:*(NSSize*)&frame.size];
}

- (void)_handleContentBoundsChanged
{
	HIViewRef	root, contentView; 
	HIRect		rootBounds, contentFrame;
	NSRect		oldContentFrameRect;

	root = HIViewGetRoot( _windowRef );
	HIViewFindByID( root, kHIViewWindowContentID, &contentView );
	HIViewGetFrame( contentView, &contentFrame );
	HIViewGetBounds( root, &rootBounds );
	
    // Set the content view's frame rect from the Carbon window's content region bounds.
    contentFrame.origin.y = rootBounds.size.height - CGRectGetMaxY( contentFrame );

    oldContentFrameRect = [(NSView *)_contentView frame];
    if ( !NSEqualRects( *(NSRect*)&contentFrame, oldContentFrameRect ) ) {
        [(NSView *)_contentView setFrame:*(NSRect*)&contentFrame];
    }
}

- (OSStatus)_handleCarbonEvent:(EventRef)inEvent callRef:(EventHandlerCallRef)inCallRef {
    OSStatus result = eventNotHandledErr;
    
    switch ( GetEventClass( inEvent ) )
    {
		case kEventClassControl:
			{
				ControlRef		control;
				
				check( GetEventKind( inEvent ) == kEventControlBoundsChanged );
				
				GetEventParameter( inEvent, kEventParamDirectObject, typeControlRef, NULL,
						sizeof( ControlRef ), NULL, &control );
				
				if ( control == HIViewGetRoot( _windowRef ) )
					[self _handleRootBoundsChanged];
				else
					[self _handleContentBoundsChanged];
			}
			break;
			
    	case kEventClassWindow:
    		switch ( GetEventKind( inEvent ) )
    		{
    			case kEventWindowShown:
					[self _setVisible:YES];
    				break;
    			
    			case kEventWindowHidden:
					[self _setVisible:NO];
    				break;
    			
    			case kEventWindowActivated:
					[self makeKeyWindow];
					break;
				
    			case kEventWindowDeactivated:
					[self resignKeyWindow];
					break;
				
				case kEventWindowBoundsChanged:
					[self reconcileToCarbonWindowBounds];
					break;
			}
    		break;
   	}
   	
    return result;
}

// Handle various events that Carbon is sending to our window.
static OSStatus NSCarbonWindowHandleEvent(EventHandlerCallRef inEventHandlerCallRef, EventRef inEventRef, void *inUserData) {

    // default action is to send event to next handler.  We modify osStatus as necessary where we don't want this behavior
    OSStatus osStatus = eventNotHandledErr;

    // We do different things for different event types.
    CarbonWindowAdapter *carbonWindow = (CarbonWindowAdapter *)inUserData;

	osStatus = [carbonWindow _handleCarbonEvent: inEventRef callRef: inEventHandlerCallRef];
	
    // Done.  If we want to propagate the event, we return eventNotHandledErr to send it to the next handler
    return osStatus;
    
}

// [3364117] We need to make sure this does not fall through to the AppKit implementation! bad things happen.
- (void)_reallyDoOrderWindow:(NSWindowOrderingMode)place relativeTo:(int)otherWin findKey:(BOOL)doKeyCalc forCounter:(BOOL)isACounter force:(BOOL)doForce isModal:(BOOL)isModal {
}

- (NSRect) _growBoxRect
{
      WindowAttributes                attrs;
      NSRect                                  retRect = NSZeroRect;

      GetWindowAttributes( _windowRef, &attrs );

      if ( attrs & kWindowResizableAttribute )
      {
              HIRect          bounds, rect;
              HIViewRef   view;

              HIViewGetBounds( HIViewGetRoot( _windowRef ), &bounds );
              HIViewFindByID( HIViewGetRoot( _windowRef ), kHIViewWindowGrowBoxID, &view );
              HIViewGetFrame( view, &rect );

              rect.origin.y = bounds.size.height - CGRectGetMaxY( rect ) - 1;
              rect.origin.x++;

              retRect = *(NSRect*)&rect;
      }

      return retRect;
}

@end // implementation CarbonWindowAdapter

#endif
