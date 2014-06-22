/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#if ENABLE(NETSCAPE_PLUGIN_API)

#import "WebNetscapePluginView.h"

#import "QuickDrawCompatibility.h"
#import "WebDataSourceInternal.h"
#import "WebDefaultUIDelegate.h"
#import "WebFrameInternal.h" 
#import "WebFrameView.h"
#import "WebKitErrorsPrivate.h"
#import "WebKitLogging.h"
#import "WebKitNSStringExtras.h"
#import "WebKitSystemInterface.h"
#import "WebNSDataExtras.h"
#import "WebNSDictionaryExtras.h"
#import "WebNSObjectExtras.h"
#import "WebNSURLExtras.h"
#import "WebNSURLRequestExtras.h"
#import "WebNSViewExtras.h"
#import "WebNetscapeContainerCheckContextInfo.h"
#import "WebNetscapeContainerCheckPrivate.h"
#import "WebNetscapePluginEventHandler.h"
#import "WebNetscapePluginPackage.h"
#import "WebNetscapePluginStream.h"
#import "WebPluginContainerCheck.h"
#import "WebPluginRequest.h"
#import "WebPreferences.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"
#import <Carbon/Carbon.h>
#import <WebCore/CookieJar.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/Element.h>
#import <WebCore/Frame.h> 
#import <WebCore/FrameLoader.h> 
#import <WebCore/FrameTree.h>
#import <WebCore/FrameView.h>
#import <WebCore/HTMLPlugInElement.h>
#import <WebCore/Page.h> 
#import <WebCore/PluginMainThreadScheduler.h>
#import <WebCore/ProxyServer.h>
#import <WebCore/RunLoop.h>
#import <WebCore/ScriptController.h>
#import <WebCore/SecurityOrigin.h>
#import <WebCore/SoftLinking.h> 
#import <WebCore/UserGestureIndicator.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/WebCoreURLResponse.h>
#import <WebCore/npruntime_impl.h>
#import <WebKit/DOMPrivate.h>
#import <WebKit/WebUIDelegate.h>
#import <objc/runtime.h>
#import <runtime/InitializeThreading.h>
#import <runtime/JSLock.h>
#import <wtf/Assertions.h>
#import <wtf/MainThread.h>
#import <wtf/text/CString.h>

#define LoginWindowDidSwitchFromUserNotification    @"WebLoginWindowDidSwitchFromUserNotification"
#define LoginWindowDidSwitchToUserNotification      @"WebLoginWindowDidSwitchToUserNotification"
#define WKNVSupportsCompositingCoreAnimationPluginsBool 74656  /* TRUE if the browser supports hardware compositing of Core Animation plug-ins  */
static const int WKNVSilverlightFullscreenPerformanceIssueFixed = 7288546; /* TRUE if Siverlight addressed its underlying  bug in <rdar://problem/7288546> */

using namespace WebCore;
using namespace WebKit;

static inline bool isDrawingModelQuickDraw(NPDrawingModel drawingModel)
{
#ifndef NP_NO_QUICKDRAW
    return drawingModel == NPDrawingModelQuickDraw;
#else
    return false;
#endif
};

@interface WebNetscapePluginView (Internal)
- (NPError)_createPlugin;
- (void)_destroyPlugin;
- (NSBitmapImageRep *)_printedPluginBitmap;
- (void)_redeliverStream;
- (BOOL)_shouldCancelSrcStream;
@end

static WebNetscapePluginView *currentPluginView = nil;

typedef struct OpaquePortState* PortState;

static const double ThrottledTimerInterval = 0.25;

class PluginTimer : public TimerBase {
public:
    typedef void (*TimerFunc)(NPP npp, uint32_t timerID);
    
    PluginTimer(NPP npp, uint32_t timerID, uint32_t interval, NPBool repeat, TimerFunc timerFunc)
        : m_npp(npp)
        , m_timerID(timerID)
        , m_interval(interval)
        , m_repeat(repeat)
        , m_timerFunc(timerFunc)
    {
    }
    
    void start(bool throttle)
    {
        ASSERT(!isActive());

        double timeInterval = m_interval / 1000.0;
        
        if (throttle)
            timeInterval = std::max(timeInterval, ThrottledTimerInterval);
        
        if (m_repeat)
            startRepeating(timeInterval);
        else
            startOneShot(timeInterval);
    }

private:
    virtual void fired() 
    {
        m_timerFunc(m_npp, m_timerID);
        if (!m_repeat)
            delete this;
    }
    
    NPP m_npp;
    uint32_t m_timerID;
    uint32_t m_interval;
    NPBool m_repeat;
    TimerFunc m_timerFunc;
};

#ifndef NP_NO_QUICKDRAW

// QuickDraw is not available in 64-bit

typedef struct {
    GrafPtr oldPort;
    GDHandle oldDevice;
    Point oldOrigin;
    RgnHandle oldClipRegion;
    RgnHandle oldVisibleRegion;
    RgnHandle clipRegion;
    BOOL forUpdate;
} PortState_QD;

#endif /* NP_NO_QUICKDRAW */

typedef struct {
    CGContextRef context;
} PortState_CG;

@class NSTextInputContext;
@interface NSResponder (AppKitDetails)
- (NSTextInputContext *)inputContext;
@end

@interface WebNetscapePluginView (ForwardDeclarations)
- (void)setWindowIfNecessary;
- (NPError)loadRequest:(NSMutableURLRequest *)request inTarget:(const char *)cTarget withNotifyData:(void *)notifyData sendNotification:(BOOL)sendNotification;
@end

@implementation WebNetscapePluginView

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
    WebCoreObjCFinalizeOnMainThread(self);
    WKSendUserChangeNotifications();
}

// MARK: EVENTS

// The WindowRef created by -[NSWindow windowRef] has a QuickDraw GrafPort that covers 
// the entire window frame (or structure region to use the Carbon term) rather then just the window content.
// We can remove this when <rdar://problem/4201099> is fixed.
- (void)fixWindowPort
{
#ifndef NP_NO_QUICKDRAW
    ASSERT(isDrawingModelQuickDraw(drawingModel));
    
    NSWindow *currentWindow = [self currentWindow];
    if ([currentWindow isKindOfClass:objc_getClass("NSCarbonWindow")])
        return;
    
    float windowHeight = [currentWindow frame].size.height;
    NSView *contentView = [currentWindow contentView];
    NSRect contentRect = [contentView convertRect:[contentView frame] toView:nil]; // convert to window-relative coordinates
    
    CGrafPtr oldPort;
    GetPort(&oldPort);    
    SetPort(GetWindowPort((WindowRef)[currentWindow windowRef]));
    
    MovePortTo(static_cast<short>(contentRect.origin.x), /* Flip Y */ static_cast<short>(windowHeight - NSMaxY(contentRect)));
    PortSize(static_cast<short>(contentRect.size.width), static_cast<short>(contentRect.size.height));
    
    SetPort(oldPort);
#endif
}

#ifndef NP_NO_QUICKDRAW
static UInt32 getQDPixelFormatForBitmapContext(CGContextRef context)
{
    UInt32 byteOrder = CGBitmapContextGetBitmapInfo(context) & kCGBitmapByteOrderMask;
    if (byteOrder == kCGBitmapByteOrderDefault)
        switch (CGBitmapContextGetBitsPerPixel(context)) {
            case 16:
                byteOrder = kCGBitmapByteOrder16Host;
                break;
            case 32:
                byteOrder = kCGBitmapByteOrder32Host;
                break;
        }
    switch (byteOrder) {
        case kCGBitmapByteOrder16Little:
            return k16LE555PixelFormat;
        case kCGBitmapByteOrder32Little:
            return k32BGRAPixelFormat;
        case kCGBitmapByteOrder16Big:
            return k16BE555PixelFormat;
        case kCGBitmapByteOrder32Big:
            return k32ARGBPixelFormat;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

static inline void getNPRect(const CGRect& cgr, NPRect& npr)
{
    npr.top = static_cast<uint16_t>(cgr.origin.y);
    npr.left = static_cast<uint16_t>(cgr.origin.x);
    npr.bottom = static_cast<uint16_t>(CGRectGetMaxY(cgr));
    npr.right = static_cast<uint16_t>(CGRectGetMaxX(cgr));
}

#endif

static inline void getNPRect(const NSRect& nr, NPRect& npr)
{
    npr.top = static_cast<uint16_t>(nr.origin.y);
    npr.left = static_cast<uint16_t>(nr.origin.x);
    npr.bottom = static_cast<uint16_t>(NSMaxY(nr));
    npr.right = static_cast<uint16_t>(NSMaxX(nr));
}

- (PortState)saveAndSetNewPortStateForUpdate:(BOOL)forUpdate
{
    ASSERT([self currentWindow] != nil);
    
    // The base coordinates of a window and it's contentView happen to be the equal at a userSpaceScaleFactor
    // of 1. For non-1.0 scale factors this assumption is false.
    NSView *windowContentView = [[self window] contentView];
    NSRect boundsInWindow = [self convertRect:[self bounds] toView:windowContentView];
    NSRect visibleRectInWindow = [self actualVisibleRectInWindow];
    
    // Flip Y to convert -[NSWindow contentView] coordinates to top-left-based window coordinates.
    float borderViewHeight = [[self currentWindow] frame].size.height;
    boundsInWindow.origin.y = borderViewHeight - NSMaxY(boundsInWindow);
    visibleRectInWindow.origin.y = borderViewHeight - NSMaxY(visibleRectInWindow);
    
#ifndef NP_NO_QUICKDRAW
    WindowRef windowRef = (WindowRef)[[self currentWindow] windowRef];
    ASSERT(windowRef);

    // Look at the Carbon port to convert top-left-based window coordinates into top-left-based content coordinates.
    if (isDrawingModelQuickDraw(drawingModel)) {
        // If drawing with QuickDraw, fix the window port so that it has the same bounds as the NSWindow's
        // content view.  This makes it easier to convert between AppKit view and QuickDraw port coordinates.
        [self fixWindowPort];
        
        ::Rect portBounds;
        CGrafPtr port = GetWindowPort(windowRef);
        GetPortBounds(port, &portBounds);

        PixMap *pix = *GetPortPixMap(port);
        boundsInWindow.origin.x += pix->bounds.left - portBounds.left;
        boundsInWindow.origin.y += pix->bounds.top - portBounds.top;
        visibleRectInWindow.origin.x += pix->bounds.left - portBounds.left;
        visibleRectInWindow.origin.y += pix->bounds.top - portBounds.top;
    }
#endif
    
    window.type = NPWindowTypeWindow;
    window.x = (int32_t)boundsInWindow.origin.x; 
    window.y = (int32_t)boundsInWindow.origin.y;
    window.width = static_cast<uint32_t>(NSWidth(boundsInWindow));
    window.height = static_cast<uint32_t>(NSHeight(boundsInWindow));
    
    // "Clip-out" the plug-in when:
    // 1) it's not really in a window or off-screen or has no height or width.
    // 2) window.x is a "big negative number" which is how WebCore expresses off-screen widgets.
    // 3) the window is miniaturized or the app is hidden
    // 4) we're inside of viewWillMoveToWindow: with a nil window. In this case, superviews may already have nil 
    // superviews and nil windows and results from convertRect:toView: are incorrect.
    if (window.width <= 0 || window.height <= 0 || window.x < -100000 || [self shouldClipOutPlugin]) {

        // The following code tries to give plug-ins the same size they will eventually have.
        // The specifiedWidth and specifiedHeight variables are used to predict the size that
        // WebCore will eventually resize us to.

        // The QuickTime plug-in has problems if you give it a width or height of 0.
        // Since other plug-ins also might have the same sort of trouble, we make sure
        // to always give plug-ins a size other than 0,0.

        if (window.width <= 0)
            window.width = specifiedWidth > 0 ? specifiedWidth : 100;
        if (window.height <= 0)
            window.height = specifiedHeight > 0 ? specifiedHeight : 100;

        window.clipRect.bottom = window.clipRect.top;
        window.clipRect.left = window.clipRect.right;
        
        // Core Animation plug-ins need to be updated (with a 0,0,0,0 clipRect) when
        // moved to a background tab. We don't do this for Core Graphics plug-ins as
        // older versions of Flash have historical WebKit-specific code that isn't
        // compatible with this behavior.
        if (drawingModel == NPDrawingModelCoreAnimation)
            getNPRect(NSZeroRect, window.clipRect);
    } else {
        getNPRect(visibleRectInWindow, window.clipRect);
    }
    
    // Save the port state, set up the port for entry into the plugin
    PortState portState;
    switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
        case NPDrawingModelQuickDraw: {
            // Set up NS_Port.
            ::Rect portBounds;
            CGrafPtr port = GetWindowPort(windowRef);
            GetPortBounds(port, &portBounds);
            nPort.qdPort.port = port;
            nPort.qdPort.portx = (int32_t)-boundsInWindow.origin.x;
            nPort.qdPort.porty = (int32_t)-boundsInWindow.origin.y;
            window.window = &nPort;

            PortState_QD *qdPortState = (PortState_QD*)malloc(sizeof(PortState_QD));
            portState = (PortState)qdPortState;
            
            GetGWorld(&qdPortState->oldPort, &qdPortState->oldDevice);    

            qdPortState->oldOrigin.h = portBounds.left;
            qdPortState->oldOrigin.v = portBounds.top;

            qdPortState->oldClipRegion = NewRgn();
            GetPortClipRegion(port, qdPortState->oldClipRegion);
            
            qdPortState->oldVisibleRegion = NewRgn();
            GetPortVisibleRegion(port, qdPortState->oldVisibleRegion);
            
            RgnHandle clipRegion = NewRgn();
            qdPortState->clipRegion = clipRegion;

            CGContextRef currentContext = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
            if (currentContext && WKCGContextIsBitmapContext(currentContext)) {
                // We use WKCGContextIsBitmapContext here, because if we just called CGBitmapContextGetData
                // on any context, we'd log to the console every time. But even if WKCGContextIsBitmapContext
                // returns true, it still might not be a context we need to create a GWorld for; for example
                // transparency layers will return true, but return 0 for CGBitmapContextGetData.
                void* offscreenData = CGBitmapContextGetData(currentContext);
                if (offscreenData) {
                    // If the current context is an offscreen bitmap, then create a GWorld for it.
                    ::Rect offscreenBounds;
                    offscreenBounds.top = 0;
                    offscreenBounds.left = 0;
                    offscreenBounds.right = CGBitmapContextGetWidth(currentContext);
                    offscreenBounds.bottom = CGBitmapContextGetHeight(currentContext);
                    GWorldPtr newOffscreenGWorld;
                    QDErr err = NewGWorldFromPtr(&newOffscreenGWorld,
                        getQDPixelFormatForBitmapContext(currentContext), &offscreenBounds, 0, 0, 0,
                        static_cast<char*>(offscreenData), CGBitmapContextGetBytesPerRow(currentContext));
                    ASSERT(newOffscreenGWorld);
                    ASSERT(!err);
                    if (!err) {
                        if (offscreenGWorld)
                            DisposeGWorld(offscreenGWorld);
                        offscreenGWorld = newOffscreenGWorld;

                        SetGWorld(offscreenGWorld, NULL);

                        port = offscreenGWorld;

                        nPort.qdPort.port = port;
                        boundsInWindow = [self bounds];
                        
                        // Generate a QD origin based on the current affine transform for currentContext.
                        CGAffineTransform offscreenMatrix = CGContextGetCTM(currentContext);
                        CGPoint origin = {0,0};
                        CGPoint axisFlip = {1,1};
                        origin = CGPointApplyAffineTransform(origin, offscreenMatrix);
                        axisFlip = CGPointApplyAffineTransform(axisFlip, offscreenMatrix);
                        
                        // Quartz bitmaps have origins at the bottom left, but the axes may be inverted, so handle that.
                        origin.x = offscreenBounds.left - origin.x * (axisFlip.x - origin.x);
                        origin.y = offscreenBounds.bottom + origin.y * (axisFlip.y - origin.y);
                        
                        nPort.qdPort.portx = static_cast<int32_t>(-boundsInWindow.origin.x + origin.x);
                        nPort.qdPort.porty = static_cast<int32_t>(-boundsInWindow.origin.y - origin.y);
                        window.x = 0;
                        window.y = 0;
                        window.window = &nPort;

                        // Use the clip bounds from the context instead of the bounds we created
                        // from the window above.
                        getNPRect(CGRectOffset(CGContextGetClipBoundingBox(currentContext), -origin.x, origin.y), window.clipRect);
                    }
                }
            }

            MacSetRectRgn(clipRegion,
                window.clipRect.left + nPort.qdPort.portx, window.clipRect.top + nPort.qdPort.porty,
                window.clipRect.right + nPort.qdPort.portx, window.clipRect.bottom + nPort.qdPort.porty);
            
            // Clip to the dirty region if drawing to a window. When drawing to another bitmap context, do not clip.
            if ([NSGraphicsContext currentContext] == [[self currentWindow] graphicsContext]) {
                // Clip to dirty region so plug-in does not draw over already-drawn regions of the window that are
                // not going to be redrawn this update.  This forces plug-ins to play nice with z-index ordering.
                if (forUpdate) {
                    RgnHandle viewClipRegion = NewRgn();
                    
                    // Get list of dirty rects from the opaque ancestor -- WebKit does some tricks with invalidation and
                    // display to enable z-ordering for NSViews; a side-effect of this is that only the WebHTMLView
                    // knows about the true set of dirty rects.
                    NSView *opaqueAncestor = [self opaqueAncestor];
                    const NSRect *dirtyRects;
                    NSInteger dirtyRectCount, dirtyRectIndex;
                    [opaqueAncestor getRectsBeingDrawn:&dirtyRects count:&dirtyRectCount];

                    for (dirtyRectIndex = 0; dirtyRectIndex < dirtyRectCount; dirtyRectIndex++) {
                        NSRect dirtyRect = [self convertRect:dirtyRects[dirtyRectIndex] fromView:opaqueAncestor];
                        if (!NSEqualSizes(dirtyRect.size, NSZeroSize)) {
                            // Create a region for this dirty rect
                            RgnHandle dirtyRectRegion = NewRgn();
                            SetRectRgn(dirtyRectRegion, static_cast<short>(NSMinX(dirtyRect)), static_cast<short>(NSMinY(dirtyRect)), static_cast<short>(NSMaxX(dirtyRect)), static_cast<short>(NSMaxY(dirtyRect)));
                            
                            // Union this dirty rect with the rest of the dirty rects
                            UnionRgn(viewClipRegion, dirtyRectRegion, viewClipRegion);
                            DisposeRgn(dirtyRectRegion);
                        }
                    }
                
                    // Intersect the dirty region with the clip region, so that we only draw over dirty parts
                    SectRgn(clipRegion, viewClipRegion, clipRegion);
                    DisposeRgn(viewClipRegion);
                }
            }

            // Switch to the port and set it up.
            SetPort(port);
            PenNormal();
            ForeColor(blackColor);
            BackColor(whiteColor);
            SetOrigin(nPort.qdPort.portx, nPort.qdPort.porty);
            SetPortClipRegion(nPort.qdPort.port, clipRegion);

            if (forUpdate) {
                // AppKit may have tried to help us by doing a BeginUpdate.
                // But the invalid region at that level didn't include AppKit's notion of what was not valid.
                // We reset the port's visible region to counteract what BeginUpdate did.
                SetPortVisibleRegion(nPort.qdPort.port, clipRegion);
                InvalWindowRgn(windowRef, clipRegion);
            }
            
            qdPortState->forUpdate = forUpdate;
            break;
        }
#endif /* NP_NO_QUICKDRAW */

        case NPDrawingModelCoreGraphics: {            
            if (![self canDraw]) {
                portState = NULL;
                break;
            }
            
            ASSERT([NSView focusView] == self);

            CGContextRef context = static_cast<CGContextRef>([[NSGraphicsContext currentContext] graphicsPort]);

            PortState_CG *cgPortState = (PortState_CG *)malloc(sizeof(PortState_CG));
            portState = (PortState)cgPortState;
            cgPortState->context = context;
            
#ifndef NP_NO_CARBON            
            if (eventModel != NPEventModelCocoa) {
                // Update the plugin's window/context
                nPort.cgPort.window = windowRef;
                nPort.cgPort.context = context;
                window.window = &nPort.cgPort;
            }                
#endif /* NP_NO_CARBON */

            // Save current graphics context's state; will be restored by -restorePortState:
            CGContextSaveGState(context);

            // Clip to the dirty region if drawing to a window. When drawing to another bitmap context, do not clip.
            if ([NSGraphicsContext currentContext] == [[self currentWindow] graphicsContext]) {
                // Get list of dirty rects from the opaque ancestor -- WebKit does some tricks with invalidation and
                // display to enable z-ordering for NSViews; a side-effect of this is that only the WebHTMLView
                // knows about the true set of dirty rects.
                NSView *opaqueAncestor = [self opaqueAncestor];
                const NSRect *dirtyRects;
                NSInteger count;
                [opaqueAncestor getRectsBeingDrawn:&dirtyRects count:&count];
                Vector<CGRect, 16> convertedDirtyRects;
                convertedDirtyRects.resize(count);
                for (int i = 0; i < count; ++i)
                    reinterpret_cast<NSRect&>(convertedDirtyRects[i]) = [self convertRect:dirtyRects[i] fromView:opaqueAncestor];
                CGContextClipToRects(context, convertedDirtyRects.data(), count);
            }

            break;
        }
          
        case NPDrawingModelCoreAnimation:
            // Just set the port state to a dummy value.
            portState = (PortState)1;
            break;
        
        default:
            ASSERT_NOT_REACHED();
            portState = NULL;
            break;
    }
    
    return portState;
}

- (PortState)saveAndSetNewPortState
{
    return [self saveAndSetNewPortStateForUpdate:NO];
}

- (void)restorePortState:(PortState)portState
{
    ASSERT([self currentWindow]);
    ASSERT(portState);
    
    switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
        case NPDrawingModelQuickDraw: {
            PortState_QD *qdPortState = (PortState_QD *)portState;
            WindowRef windowRef = (WindowRef)[[self currentWindow] windowRef];
            CGrafPtr port = GetWindowPort(windowRef);

            SetPort(port);

            if (qdPortState->forUpdate)
                ValidWindowRgn(windowRef, qdPortState->clipRegion);

            SetOrigin(qdPortState->oldOrigin.h, qdPortState->oldOrigin.v);

            SetPortClipRegion(port, qdPortState->oldClipRegion);
            if (qdPortState->forUpdate)
                SetPortVisibleRegion(port, qdPortState->oldVisibleRegion);

            DisposeRgn(qdPortState->oldClipRegion);
            DisposeRgn(qdPortState->oldVisibleRegion);
            DisposeRgn(qdPortState->clipRegion);

            SetGWorld(qdPortState->oldPort, qdPortState->oldDevice);
            break;
        }
#endif /* NP_NO_QUICKDRAW */
        
        case NPDrawingModelCoreGraphics: {
            ASSERT([NSView focusView] == self);
            
            CGContextRef context = ((PortState_CG *)portState)->context;
            ASSERT(!nPort.cgPort.context || (context == nPort.cgPort.context));
            CGContextRestoreGState(context);
            break;
        }
        
        case NPDrawingModelCoreAnimation:
            ASSERT(portState == (PortState)1);
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
    }
}

- (BOOL)sendEvent:(void*)event isDrawRect:(BOOL)eventIsDrawRect
{
    if (![self window])
        return NO;
    ASSERT(event);
       
    if (!_isStarted)
        return NO;

    ASSERT([_pluginPackage.get() pluginFuncs]->event);
    
    // Make sure we don't call NPP_HandleEvent while we're inside NPP_SetWindow.
    // We probably don't want more general reentrancy protection; we are really
    // protecting only against this one case, which actually comes up when
    // you first install the SVG viewer plug-in.
    if (inSetWindow)
        return NO;

    Frame* frame = core([self webFrame]);
    if (!frame)
        return NO;
    Page* page = frame->page();
    if (!page)
        return NO;

    // Can only send drawRect (updateEvt) to CoreGraphics plugins when actually drawing
    ASSERT((drawingModel != NPDrawingModelCoreGraphics) || !eventIsDrawRect || [NSView focusView] == self);
    
    PortState portState = NULL;
    
    if (isDrawingModelQuickDraw(drawingModel) || (drawingModel != NPDrawingModelCoreAnimation && eventIsDrawRect)) {
        // In CoreGraphics mode, the port state only needs to be saved/set when redrawing the plug-in view.
        // The plug-in is not allowed to draw at any other time.
        portState = [self saveAndSetNewPortStateForUpdate:eventIsDrawRect];
        // We may have changed the window, so inform the plug-in.
        [self setWindowIfNecessary];
    }
    
#if !defined(NDEBUG) && !defined(NP_NO_QUICKDRAW)
    // Draw green to help debug.
    // If we see any green we know something's wrong.
    // Note that PaintRect() only works for QuickDraw plugins; otherwise the current QD port is undefined.
    if (isDrawingModelQuickDraw(drawingModel) && eventIsDrawRect) {
        ForeColor(greenColor);
        const ::Rect bigRect = { -10000, -10000, 10000, 10000 };
        PaintRect(&bigRect);
        ForeColor(blackColor);
    }
#endif
    
    // Temporarily retain self in case the plug-in view is released while sending an event. 
    [[self retain] autorelease];

    BOOL acceptedEvent;
    [self willCallPlugInFunction];
    // Set the pluginAllowPopup flag.
    ASSERT(_eventHandler);
    {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        UserGestureIndicator gestureIndicator(_eventHandler->currentEventIsUserGesture() ? DefinitelyProcessingUserGesture : PossiblyProcessingUserGesture);
        acceptedEvent = [_pluginPackage.get() pluginFuncs]->event(plugin, event);
    }
    [self didCallPlugInFunction];

    if (portState) {
        if ([self currentWindow])
            [self restorePortState:portState];
        if (portState != (PortState)1)
            free(portState);
    }

    return acceptedEvent;
}

- (void)windowFocusChanged:(BOOL)hasFocus
{
    _eventHandler->windowFocusChanged(hasFocus);
}

- (void)sendDrawRectEvent:(NSRect)rect
{
    ASSERT(_eventHandler);
    
    CGContextRef context = static_cast<CGContextRef>([[NSGraphicsContext currentContext] graphicsPort]);
    _eventHandler->drawRect(context, rect);
}

- (void)stopTimers
{
    [super stopTimers];
    
    if (_eventHandler)
        _eventHandler->stopTimers();
    
    if (!timers)
        return;

    HashMap<uint32_t, PluginTimer*>::const_iterator end = timers->end();
    for (HashMap<uint32_t, PluginTimer*>::const_iterator it = timers->begin(); it != end; ++it) {
        PluginTimer* timer = it->value;
        timer->stop();
    }    
}

- (void)startTimers
{
    [super startTimers];
    
    // If the plugin is completely obscured (scrolled out of view, for example), then we will
    // send null events at a reduced rate.
    _eventHandler->startTimers(_isCompletelyObscured);
    
    if (!timers)
        return;
    
    HashMap<uint32_t, PluginTimer*>::const_iterator end = timers->end();
    for (HashMap<uint32_t, PluginTimer*>::const_iterator it = timers->begin(); it != end; ++it) {
        PluginTimer* timer = it->value;
        ASSERT(!timer->isActive());
        timer->start(_isCompletelyObscured);
    }    
}

- (void)focusChanged
{
    // We need to null check the event handler here because
    // the plug-in view can resign focus after it's been stopped
    // and the event handler has been deleted.
    if (_eventHandler)
        _eventHandler->focusChanged(_hasFocus);
}

- (void)mouseDown:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    _eventHandler->mouseDown(theEvent);
}

- (void)mouseUp:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    _eventHandler->mouseUp(theEvent);
}

- (void)handleMouseEntered:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    // Set cursor to arrow. Plugins often handle cursor internally, but those that don't will just get this default one.
    [[NSCursor arrowCursor] set];

    _eventHandler->mouseEntered(theEvent);
}

- (void)handleMouseExited:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    _eventHandler->mouseExited(theEvent);
    
    // Set cursor back to arrow cursor.  Because NSCursor doesn't know about changes that the plugin made, we could get confused about what we think the
    // current cursor is otherwise.  Therefore we have no choice but to unconditionally reset the cursor when the mouse exits the plugin.
    [[NSCursor arrowCursor] set];
}

- (void)handleMouseMoved:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    _eventHandler->mouseMoved(theEvent);
}
    
- (void)mouseDragged:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    _eventHandler->mouseDragged(theEvent);
}

- (void)scrollWheel:(NSEvent *)theEvent
{
    if (!_isStarted) {
        [super scrollWheel:theEvent];
        return;
    }

    if (!_eventHandler->scrollWheel(theEvent))
        [super scrollWheel:theEvent];
}

- (void)keyUp:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    _eventHandler->keyUp(theEvent);
}

- (void)keyDown:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    _eventHandler->keyDown(theEvent);
}

- (void)flagsChanged:(NSEvent *)theEvent
{
    if (!_isStarted)
        return;

    _eventHandler->flagsChanged(theEvent);
}

- (void)sendModifierEventWithKeyCode:(int)keyCode character:(char)character
{
    if (!_isStarted)
        return;
    
    _eventHandler->syntheticKeyDownWithCommandModifier(keyCode, character);
}

- (void)privateBrowsingModeDidChange
{
    if (!_isStarted)
        return;
    
    NPBool value = _isPrivateBrowsingEnabled;

    [self willCallPlugInFunction];
    {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        if ([_pluginPackage.get() pluginFuncs]->setvalue)
            [_pluginPackage.get() pluginFuncs]->setvalue(plugin, NPNVprivateModeBool, &value);
    }
    [self didCallPlugInFunction];
}

// MARK: WEB_NETSCAPE_PLUGIN

- (BOOL)isNewWindowEqualToOldWindow
{
    if (window.x != lastSetWindow.x)
        return NO;
    if (window.y != lastSetWindow.y)
        return NO;
    if (window.width != lastSetWindow.width)
        return NO;
    if (window.height != lastSetWindow.height)
        return NO;
    if (window.clipRect.top != lastSetWindow.clipRect.top)
        return NO;
    if (window.clipRect.left != lastSetWindow.clipRect.left)
        return NO;
    if (window.clipRect.bottom  != lastSetWindow.clipRect.bottom)
        return NO;
    if (window.clipRect.right != lastSetWindow.clipRect.right)
        return NO;
    if (window.type != lastSetWindow.type)
        return NO;
    
    switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
        case NPDrawingModelQuickDraw:
            if (nPort.qdPort.portx != lastSetPort.qdPort.portx)
                return NO;
            if (nPort.qdPort.porty != lastSetPort.qdPort.porty)
                return NO;
            if (nPort.qdPort.port != lastSetPort.qdPort.port)
                return NO;
        break;
#endif /* NP_NO_QUICKDRAW */
            
        case NPDrawingModelCoreGraphics:
            if (nPort.cgPort.window != lastSetPort.cgPort.window)
                return NO;
            if (nPort.cgPort.context != lastSetPort.cgPort.context)
                return NO;
        break;
                    
        case NPDrawingModelCoreAnimation:
          if (window.window != lastSetWindow.window)
              return NO;
          break;
        default:
            ASSERT_NOT_REACHED();
        break;
    }
    
    return YES;
}

-(void)tellQuickTimeToChill
{
#ifndef NP_NO_QUICKDRAW
    ASSERT(isDrawingModelQuickDraw(drawingModel));
    
    // Make a call to the secret QuickDraw API that makes QuickTime calm down.
    WindowRef windowRef = (WindowRef)[[self window] windowRef];
    if (!windowRef) {
        return;
    }
    CGrafPtr port = GetWindowPort(windowRef);
    ::Rect bounds;
    GetPortBounds(port, &bounds);
    WKCallDrawingNotification(port, &bounds);
#endif /* NP_NO_QUICKDRAW */
}

- (void)updateAndSetWindow
{
    // A plug-in can only update if it's (1) already been started (2) isn't stopped
    // and (3) is able to draw on-screen. To meet condition (3) the plug-in must not
    // be hidden and be attached to a window. There are two exceptions to this rule:
    //
    // Exception 1: QuickDraw plug-ins must be manually told when to stop writing
    // bits to the window backing store, thus to do so requires a new call to
    // NPP_SetWindow() with an empty NPWindow struct.
    //
    // Exception 2: CoreGraphics plug-ins expect to have their drawable area updated
    // when they are moved to a background tab, via a NPP_SetWindow call. This is
    // accomplished by allowing -saveAndSetNewPortStateForUpdate to "clip-out" the window's
    // clipRect. Flash is curently an exception to this. See 6453738.
    //
    
    if (!_isStarted)
        return;
    
#ifdef NP_NO_QUICKDRAW
    if (![self canDraw])
        return;
#else
    if (drawingModel == NPDrawingModelQuickDraw)
        [self tellQuickTimeToChill];
    else if (drawingModel == NPDrawingModelCoreGraphics && ![self canDraw] && _isFlash) {
        // The Flash plug-in does not expect an NPP_SetWindow call from WebKit in this case.
        // See Exception 2 above.
        return;
    }
#endif // NP_NO_QUICKDRAW
    
    BOOL didLockFocus = [NSView focusView] != self && [self lockFocusIfCanDraw];

    PortState portState = [self saveAndSetNewPortState];
    if (portState) {
        [self setWindowIfNecessary];
        [self restorePortState:portState];
        if (portState != (PortState)1)
            free(portState);
    } else if (drawingModel == NPDrawingModelCoreGraphics)
        [self setWindowIfNecessary];        

    if (didLockFocus)
        [self unlockFocus];
}

- (void)setWindowIfNecessary
{
    if (!_isStarted) 
        return;
    
    if (![self isNewWindowEqualToOldWindow]) {        
        // Make sure we don't call NPP_HandleEvent while we're inside NPP_SetWindow.
        // We probably don't want more general reentrancy protection; we are really
        // protecting only against this one case, which actually comes up when
        // you first install the SVG viewer plug-in.
        NPError npErr;
        
        BOOL wasInSetWindow = inSetWindow;
        inSetWindow = YES;        
        [self willCallPlugInFunction];
        {
            JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
            npErr = [_pluginPackage.get() pluginFuncs]->setwindow(plugin, &window);
        }
        [self didCallPlugInFunction];
        inSetWindow = wasInSetWindow;

#ifndef NDEBUG
        switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
            case NPDrawingModelQuickDraw:
                LOG(Plugins, "NPP_SetWindow (QuickDraw): %d, port=0x%08x, window.x:%d window.y:%d window.width:%d window.height:%d",
                npErr, (int)nPort.qdPort.port, (int)window.x, (int)window.y, (int)window.width, (int)window.height);
            break;
#endif /* NP_NO_QUICKDRAW */
            
            case NPDrawingModelCoreGraphics:
                LOG(Plugins, "NPP_SetWindow (CoreGraphics): %d, window=%p, context=%p, window.x:%d window.y:%d window.width:%d window.height:%d window.clipRect size:%dx%d",
                npErr, nPort.cgPort.window, nPort.cgPort.context, (int)window.x, (int)window.y, (int)window.width, (int)window.height, 
                    window.clipRect.right - window.clipRect.left, window.clipRect.bottom - window.clipRect.top);
            break;

            case NPDrawingModelCoreAnimation:
                LOG(Plugins, "NPP_SetWindow (CoreAnimation): %d, window=%p window.x:%d window.y:%d window.width:%d window.height:%d",
                npErr, window.window, nPort.cgPort.context, (int)window.x, (int)window.y, (int)window.width, (int)window.height);
            break;

            default:
                ASSERT_NOT_REACHED();
            break;
        }
#endif /* !defined(NDEBUG) */
        
        lastSetWindow = window;
        lastSetPort = nPort;
    }
}

+ (void)setCurrentPluginView:(WebNetscapePluginView *)view
{
    currentPluginView = view;
}

+ (WebNetscapePluginView *)currentPluginView
{
    return currentPluginView;
}

- (BOOL)createPlugin
{
    // Open the plug-in package so it remains loaded while our plugin uses it
    [_pluginPackage.get() open];
    
    // Initialize drawingModel to an invalid value so that we can detect when the plugin does not specify a drawingModel
    drawingModel = (NPDrawingModel)-1;
    
    // Initialize eventModel to an invalid value so that we can detect when the plugin does not specify an event model.
    eventModel = (NPEventModel)-1;
    
    NPError npErr = [self _createPlugin];
    if (npErr != NPERR_NO_ERROR) {
        LOG_ERROR("NPP_New failed with error: %d", npErr);
        [self _destroyPlugin];
        [_pluginPackage.get() close];
        return NO;
    }
    
    if (drawingModel == (NPDrawingModel)-1) {
#ifndef NP_NO_QUICKDRAW
        // Default to QuickDraw if the plugin did not specify a drawing model.
        drawingModel = NPDrawingModelQuickDraw;
#else
        // QuickDraw is not available, so we can't default to it. Instead, default to CoreGraphics.
        drawingModel = NPDrawingModelCoreGraphics;
#endif
    }

    if (eventModel == (NPEventModel)-1) {
        // If the plug-in did not specify a drawing model we default to Carbon when it is available.
#ifndef NP_NO_CARBON
        eventModel = NPEventModelCarbon;
#else
        eventModel = NPEventModelCocoa;
#endif // NP_NO_CARBON
    }

#ifndef NP_NO_CARBON
    if (eventModel == NPEventModelCocoa && isDrawingModelQuickDraw(drawingModel)) {
        LOG(Plugins, "Plugin can't use use Cocoa event model with QuickDraw drawing model: %@", _pluginPackage.get());
        [self _destroyPlugin];
        [_pluginPackage.get() close];
        
        return NO;
    }        
#endif // NP_NO_CARBON
    
    if (drawingModel == NPDrawingModelCoreAnimation) {
        void *value = 0;
        if ([_pluginPackage.get() pluginFuncs]->getvalue(plugin, NPPVpluginCoreAnimationLayer, &value) == NPERR_NO_ERROR && value) {

            // The plug-in gives us a retained layer.
            _pluginLayer = adoptNS((CALayer *)value);

            BOOL accleratedCompositingEnabled = false;
#if USE(ACCELERATED_COMPOSITING)
            accleratedCompositingEnabled = [[[self webView] preferences] acceleratedCompositingEnabled];
#endif
            if (accleratedCompositingEnabled) {
                // FIXME: This code can be shared between WebHostedNetscapePluginView and WebNetscapePluginView.
                // Since this layer isn't going to be inserted into a view, we need to create another layer and flip its geometry
                // in order to get the coordinate system right.
                RetainPtr<CALayer> realPluginLayer = adoptNS(_pluginLayer.leakRef());
                
                _pluginLayer = adoptNS([[CALayer alloc] init]);
                _pluginLayer.get().bounds = realPluginLayer.get().bounds;
                _pluginLayer.get().geometryFlipped = YES;

                realPluginLayer.get().autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
                [_pluginLayer.get() addSublayer:realPluginLayer.get()];

                // Eagerly enter compositing mode, since we know we'll need it. This avoids firing setNeedsStyleRecalc()
                // for iframes that contain composited plugins at bad times. https://bugs.webkit.org/show_bug.cgi?id=39033
                core([self webFrame])->view()->enterCompositingMode();
                [self element]->setNeedsStyleRecalc(SyntheticStyleChange);
            } else
                [self setWantsLayer:YES];

            LOG(Plugins, "%@ is using Core Animation drawing model with layer %@", _pluginPackage.get(), _pluginLayer.get());
        }

        ASSERT(_pluginLayer);
    }
    
    // Create the event handler
    _eventHandler = WebNetscapePluginEventHandler::create(self);

    return YES;
}

// FIXME: This method is an ideal candidate to move up to the base class
- (CALayer *)pluginLayer
{
    return _pluginLayer.get();
}

- (void)setLayer:(CALayer *)newLayer
{
    [super setLayer:newLayer];

    if (newLayer && _pluginLayer) {
        _pluginLayer.get().frame = [newLayer frame];
        _pluginLayer.get().autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
        [newLayer addSublayer:_pluginLayer.get()];
    }
}

- (void)loadStream
{
    if ([self _shouldCancelSrcStream])
        return;
    
    if (_loadManually) {
        [self _redeliverStream];
        return;
    }
    
    // If the OBJECT/EMBED tag has no SRC, the URL is passed to us as "".
    // Check for this and don't start a load in this case.
    if (_sourceURL && ![_sourceURL.get() _web_isEmpty]) {
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:_sourceURL.get()];
        [request _web_setHTTPReferrer:core([self webFrame])->loader()->outgoingReferrer()];
        [self loadRequest:request inTarget:nil withNotifyData:nil sendNotification:NO];
    } 
}

- (BOOL)shouldStop
{
    // If we're already calling a plug-in function, do not call NPP_Destroy().  The plug-in function we are calling
    // may assume that its instance->pdata, or other memory freed by NPP_Destroy(), is valid and unchanged until said
    // plugin-function returns.
    // See <rdar://problem/4480737>.
    if (pluginFunctionCallDepth > 0) {
        shouldStopSoon = YES;
        return NO;
    }

    return YES;
}

- (void)destroyPlugin
{
    // To stop active streams it's necessary to invoke stop() on a copy 
    // of streams. This is because calling WebNetscapePluginStream::stop() also has the side effect
    // of removing a stream from this hash set.
    Vector<RefPtr<WebNetscapePluginStream> > streamsCopy;
    copyToVector(streams, streamsCopy);
    for (size_t i = 0; i < streamsCopy.size(); i++)
        streamsCopy[i]->stop();

    for (WebFrame *frame in [_pendingFrameLoads keyEnumerator])
        [frame _setInternalLoadDelegate:nil];
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

    // Setting the window type to 0 ensures that NPP_SetWindow will be called if the plug-in is restarted.
    lastSetWindow.type = (NPWindowType)0;
    
    _pluginLayer = 0;
    
    [self _destroyPlugin];
    [_pluginPackage.get() close];
    
    _eventHandler.clear();
}

- (NPEventModel)eventModel
{
    return eventModel;
}

- (NPP)plugin
{
    return plugin;
}

- (void)setAttributeKeys:(NSArray *)keys andValues:(NSArray *)values
{
    ASSERT([keys count] == [values count]);
    
    // Convert the attributes to 2 C string arrays.
    // These arrays are passed to NPP_New, but the strings need to be
    // modifiable and live the entire life of the plugin.

    // The Java plug-in requires the first argument to be the base URL
    if ([_MIMEType.get() isEqualToString:@"application/x-java-applet"]) {
        cAttributes = (char **)malloc(([keys count] + 1) * sizeof(char *));
        cValues = (char **)malloc(([values count] + 1) * sizeof(char *));
        cAttributes[0] = strdup("DOCBASE");
        cValues[0] = strdup([_baseURL.get() _web_URLCString]);
        argsCount++;
    } else {
        cAttributes = (char **)malloc([keys count] * sizeof(char *));
        cValues = (char **)malloc([values count] * sizeof(char *));
    }

    BOOL isWMP = [_pluginPackage.get() bundleIdentifier] == "com.microsoft.WMP.defaultplugin";
    
    unsigned i;
    unsigned count = [keys count];
    for (i = 0; i < count; i++) {
        NSString *key = [keys objectAtIndex:i];
        NSString *value = [values objectAtIndex:i];
        if ([key _webkit_isCaseInsensitiveEqualToString:@"height"]) {
            specifiedHeight = [value intValue];
        } else if ([key _webkit_isCaseInsensitiveEqualToString:@"width"]) {
            specifiedWidth = [value intValue];
        }
        // Avoid Window Media Player crash when these attributes are present.
        if (isWMP && ([key _webkit_isCaseInsensitiveEqualToString:@"SAMIStyle"] || [key _webkit_isCaseInsensitiveEqualToString:@"SAMILang"])) {
            continue;
        }
        cAttributes[argsCount] = strdup([key UTF8String]);
        cValues[argsCount] = strdup([value UTF8String]);
        LOG(Plugins, "%@ = %@", key, value);
        argsCount++;
    }
}

- (uint32_t)checkIfAllowedToLoadURL:(const char*)urlCString frame:(const char*)frameNameCString 
                       callbackFunc:(void (*)(NPP npp, uint32_t checkID, NPBool allowed, void* context))callbackFunc 
                            context:(void*)context
{
    if (!_containerChecksInProgress) 
        _containerChecksInProgress = [[NSMutableDictionary alloc] init];
    
    NSString *frameName = frameNameCString ? [NSString stringWithCString:frameNameCString encoding:NSISOLatin1StringEncoding] : nil;
    
    ++_currentContainerCheckRequestID;
    WebNetscapeContainerCheckContextInfo *contextInfo = [[WebNetscapeContainerCheckContextInfo alloc] initWithCheckRequestID:_currentContainerCheckRequestID 
                                                                                                                callbackFunc:callbackFunc
                                                                                                                      context:context];
    
    WebPluginContainerCheck *check = [WebPluginContainerCheck checkWithRequest:[self requestWithURLCString:urlCString]
                                                                        target:frameName
                                                                  resultObject:self
                                                                      selector:@selector(_containerCheckResult:contextInfo:)
                                                                    controller:self 
                                                                   contextInfo:contextInfo];
    
    [contextInfo release];
    [_containerChecksInProgress setObject:check forKey:[NSNumber numberWithInt:_currentContainerCheckRequestID]];
    [check start];
    
    return _currentContainerCheckRequestID;
}

- (void)_containerCheckResult:(PolicyAction)policy contextInfo:(id)contextInfo
{
    ASSERT([contextInfo isKindOfClass:[WebNetscapeContainerCheckContextInfo class]]);
    void (*pluginCallback)(NPP npp, uint32_t, NPBool, void*) = [contextInfo callback];
    
    if (!pluginCallback) {
        ASSERT_NOT_REACHED();
        return;
    }
    
    pluginCallback([self plugin], [contextInfo checkRequestID], (policy == PolicyUse), [contextInfo context]);
}

- (void)cancelCheckIfAllowedToLoadURL:(uint32_t)checkID
{
    WebPluginContainerCheck *check = (WebPluginContainerCheck *)[_containerChecksInProgress objectForKey:[NSNumber numberWithInt:checkID]];
    
    if (!check)
        return;
    
    [check cancel];
    [_containerChecksInProgress removeObjectForKey:[NSNumber numberWithInt:checkID]];
}

// WebPluginContainerCheck automatically calls this method after invoking our _containerCheckResult: selector.
// It works this way because calling -[WebPluginContainerCheck cancel] allows it to do it's teardown process.
- (void)_webPluginContainerCancelCheckIfAllowedToLoadRequest:(id)webPluginContainerCheck
{
    ASSERT([webPluginContainerCheck isKindOfClass:[WebPluginContainerCheck class]]);
    WebPluginContainerCheck *check = (WebPluginContainerCheck *)webPluginContainerCheck;
    ASSERT([[check contextInfo] isKindOfClass:[WebNetscapeContainerCheckContextInfo class]]);
    
    [self cancelCheckIfAllowedToLoadURL:[[check contextInfo] checkRequestID]];
}


// MARK: NSVIEW

- (id)initWithFrame:(NSRect)frame
      pluginPackage:(WebNetscapePluginPackage *)pluginPackage
                URL:(NSURL *)URL
            baseURL:(NSURL *)baseURL
           MIMEType:(NSString *)MIME
      attributeKeys:(NSArray *)keys
    attributeValues:(NSArray *)values
       loadManually:(BOOL)loadManually
            element:(PassRefPtr<WebCore::HTMLPlugInElement>)element
{
    self = [super initWithFrame:frame pluginPackage:pluginPackage URL:URL baseURL:baseURL MIMEType:MIME attributeKeys:keys attributeValues:values loadManually:loadManually element:element];
    if (!self)
        return nil;

    _pendingFrameLoads = adoptNS([[NSMapTable alloc] initWithKeyOptions:NSPointerFunctionsStrongMemory valueOptions:NSPointerFunctionsStrongMemory capacity:0]);

    // load the plug-in if it is not already loaded
    if (![pluginPackage load]) {
        [self release];
        return nil;
    }

    return self;
}

- (id)initWithFrame:(NSRect)frame
{
    ASSERT_NOT_REACHED();
    return nil;
}

- (void)fini
{
#ifndef NP_NO_QUICKDRAW
    if (offscreenGWorld)
        DisposeGWorld(offscreenGWorld);
#endif

    for (unsigned i = 0; i < argsCount; i++) {
        free(cAttributes[i]);
        free(cValues[i]);
    }
    free(cAttributes);
    free(cValues);
    
    ASSERT(!_eventHandler);
    
    if (timers) {
        deleteAllValues(*timers);
        delete timers;
    }  
    
    [_containerChecksInProgress release];
}

- (void)disconnectStream:(WebNetscapePluginStream*)stream
{
    streams.remove(stream);
}

- (void)dealloc
{
    ASSERT(!_isStarted);
    ASSERT(!plugin);

    [self fini];

    [super dealloc];
}

- (void)finalize
{
    ASSERT_MAIN_THREAD();
    ASSERT(!_isStarted);

    [self fini];

    [super finalize];
}

- (void)drawRect:(NSRect)rect
{
    if (_cachedSnapshot) {
        NSRect sourceRect = { NSZeroPoint, [_cachedSnapshot.get() size] };
        [_cachedSnapshot.get() drawInRect:[self bounds] fromRect:sourceRect operation:NSCompositeSourceOver fraction:1];
        return;
    }
    
    if (drawingModel == NPDrawingModelCoreAnimation && (!_snapshotting || ![self supportsSnapshotting]))
        return;

    if (!_isStarted)
        return;
    
    if ([NSGraphicsContext currentContextDrawingToScreen] || _isFlash)
        [self sendDrawRectEvent:rect];
    else {
        NSBitmapImageRep *printedPluginBitmap = [self _printedPluginBitmap];
        if (printedPluginBitmap) {
            // Flip the bitmap before drawing because the QuickDraw port is flipped relative
            // to this view.
            CGContextRef cgContext = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
            CGContextSaveGState(cgContext);
            NSRect bounds = [self bounds];
            CGContextTranslateCTM(cgContext, 0.0f, NSHeight(bounds));
            CGContextScaleCTM(cgContext, 1.0f, -1.0f);
            [printedPluginBitmap drawInRect:bounds];
            CGContextRestoreGState(cgContext);
        }
    }
}

- (NPObject *)createPluginScriptableObject
{
    if (![_pluginPackage.get() pluginFuncs]->getvalue || !_isStarted)
        return NULL;
        
    NPObject *value = NULL;
    NPError error;
    [self willCallPlugInFunction];
    {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        error = [_pluginPackage.get() pluginFuncs]->getvalue(plugin, NPPVpluginScriptableNPObject, &value);
    }
    [self didCallPlugInFunction];
    if (error != NPERR_NO_ERROR)
        return NULL;
    
    return value;
}

- (BOOL)getFormValue:(NSString **)value
{
    if (![_pluginPackage.get() pluginFuncs]->getvalue || !_isStarted)
        return false;
    // Plugins will allocate memory for the buffer by using NPN_MemAlloc().
    char* buffer = NULL;
    NPError error;
    [self willCallPlugInFunction];
    {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        error = [_pluginPackage.get() pluginFuncs]->getvalue(plugin, NPPVformValue, &buffer);
    }
    [self didCallPlugInFunction];
    if (error != NPERR_NO_ERROR || !buffer)
        return false;
    *value = [[NSString alloc] initWithUTF8String:buffer];
    [_pluginPackage.get() browserFuncs]->memfree(buffer);
    return true;
}

- (void)willCallPlugInFunction
{
    ASSERT(plugin);

    // Could try to prevent infinite recursion here, but it's probably not worth the effort.
    pluginFunctionCallDepth++;
}

- (void)didCallPlugInFunction
{
    ASSERT(pluginFunctionCallDepth > 0);
    pluginFunctionCallDepth--;
    
    // If -stop was called while we were calling into a plug-in function, and we're no longer
    // inside a plug-in function, stop now.
    if (pluginFunctionCallDepth == 0 && shouldStopSoon) {
        shouldStopSoon = NO;
        [self stop];
    }
}

-(void)pluginView:(NSView *)pluginView receivedResponse:(NSURLResponse *)response
{
    ASSERT(_loadManually);
    ASSERT(!_manualStream);

    _manualStream = WebNetscapePluginStream::create(core([self webFrame])->loader());
}

- (void)pluginView:(NSView *)pluginView receivedData:(NSData *)data
{
    ASSERT(_loadManually);
    ASSERT(_manualStream);
    
    _dataLengthReceived += [data length];
    
    if (!_isStarted)
        return;

    if (!_manualStream->plugin()) {
        // Check if the load should be cancelled
        if ([self _shouldCancelSrcStream]) {
            NSURLResponse *response = [[self dataSource] response];
            
            NSError *error = [[NSError alloc] _initWithPluginErrorCode:WebKitErrorPlugInWillHandleLoad
                                                            contentURL:[response URL]
                                                         pluginPageURL:nil
                                                            pluginName:nil // FIXME: Get this from somewhere
                                                              MIMEType:[response MIMEType]];
            [[self dataSource] _documentLoader]->cancelMainResourceLoad(error);
            [error release];
            return;
        }
        
        _manualStream->setRequestURL([[[self dataSource] request] URL]);
        _manualStream->setPlugin([self plugin]);
        ASSERT(_manualStream->plugin());
        
        _manualStream->startStreamWithResponse([[self dataSource] response]);
    }

    if (_manualStream->plugin())
        _manualStream->didReceiveData(0, static_cast<const char *>([data bytes]), [data length]);
}

- (void)pluginView:(NSView *)pluginView receivedError:(NSError *)error
{
    ASSERT(_loadManually);

    _error = error;
    
    if (!_isStarted) {
        return;
    }

    _manualStream->destroyStreamWithError(error);
}

- (void)pluginViewFinishedLoading:(NSView *)pluginView 
{
    ASSERT(_loadManually);
    ASSERT(_manualStream);
    
    if (_isStarted)
        _manualStream->didFinishLoading(0);
}

- (NSTextInputContext *)inputContext
{
    return nil;
}

@end

@implementation WebNetscapePluginView (WebNPPCallbacks)

- (void)evaluateJavaScriptPluginRequest:(WebPluginRequest *)JSPluginRequest
{
    // FIXME: Is this isStarted check needed here? evaluateJavaScriptPluginRequest should not be called
    // if we are stopped since this method is called after a delay and we call 
    // cancelPreviousPerformRequestsWithTarget inside of stop.
    if (!_isStarted) {
        return;
    }
    
    NSURL *URL = [[JSPluginRequest request] URL];
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    ASSERT(JSString);
    
    NSString *result = [[self webFrame] _stringByEvaluatingJavaScriptFromString:JSString forceUserGesture:[JSPluginRequest isCurrentEventUserGesture]];
    
    // Don't continue if stringByEvaluatingJavaScriptFromString caused the plug-in to stop.
    if (!_isStarted) {
        return;
    }
        
    if ([JSPluginRequest frameName] != nil) {
        // FIXME: If the result is a string, we probably want to put that string into the frame.
        if ([JSPluginRequest sendNotification]) {
            [self willCallPlugInFunction];
            {
                JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
                [_pluginPackage.get() pluginFuncs]->urlnotify(plugin, [URL _web_URLCString], NPRES_DONE, [JSPluginRequest notifyData]);
            }
            [self didCallPlugInFunction];
        }
    } else if ([result length] > 0) {
        // Don't call NPP_NewStream and other stream methods if there is no JS result to deliver. This is what Mozilla does.
        NSData *JSData = [result dataUsingEncoding:NSUTF8StringEncoding];
        
        RefPtr<WebNetscapePluginStream> stream = WebNetscapePluginStream::create([NSURLRequest requestWithURL:URL], plugin, [JSPluginRequest sendNotification], [JSPluginRequest notifyData]);
        
        RetainPtr<NSURLResponse> response = adoptNS([[NSURLResponse alloc] initWithURL:URL 
                                                                             MIMEType:@"text/plain" 
                                                                expectedContentLength:[JSData length]
                                                                     textEncodingName:nil]);
        
        stream->startStreamWithResponse(response.get());
        stream->didReceiveData(0, static_cast<const char*>([JSData bytes]), [JSData length]);
        stream->didFinishLoading(0);
    }
}

- (void)webFrame:(WebFrame *)webFrame didFinishLoadWithReason:(NPReason)reason
{
    ASSERT(_isStarted);
    
    WebPluginRequest *pluginRequest = [_pendingFrameLoads objectForKey:webFrame];
    ASSERT(pluginRequest != nil);
    ASSERT([pluginRequest sendNotification]);
        
    [self willCallPlugInFunction];
    {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        [_pluginPackage.get() pluginFuncs]->urlnotify(plugin, [[[pluginRequest request] URL] _web_URLCString], reason, [pluginRequest notifyData]);
    }
    [self didCallPlugInFunction];
    
    [_pendingFrameLoads removeObjectForKey:webFrame];
    [webFrame _setInternalLoadDelegate:nil];
}

- (void)webFrame:(WebFrame *)webFrame didFinishLoadWithError:(NSError *)error
{
    NPReason reason = NPRES_DONE;
    if (error != nil)
        reason = WebNetscapePluginStream::reasonForError(error);
    [self webFrame:webFrame didFinishLoadWithReason:reason];
}

- (void)loadPluginRequest:(WebPluginRequest *)pluginRequest
{
    NSURLRequest *request = [pluginRequest request];
    NSString *frameName = [pluginRequest frameName];
    WebFrame *frame = nil;
    
    NSURL *URL = [request URL];
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    
    ASSERT(frameName || JSString);
    
    if (frameName) {
        // FIXME - need to get rid of this window creation which
        // bypasses normal targeted link handling
        frame = kit(core([self webFrame])->loader()->findFrameForNavigation(frameName));
        if (frame == nil) {
            WebView *currentWebView = [self webView];
            NSDictionary *features = [[NSDictionary alloc] init];
            WebView *newWebView = [[currentWebView _UIDelegateForwarder] webView:currentWebView
                                                        createWebViewWithRequest:nil
                                                                  windowFeatures:features];
            [features release];

            if (!newWebView) {
                if ([pluginRequest sendNotification]) {
                    [self willCallPlugInFunction];
                    {
                        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
                        [_pluginPackage.get() pluginFuncs]->urlnotify(plugin, [[[pluginRequest request] URL] _web_URLCString], NPERR_GENERIC_ERROR, [pluginRequest notifyData]);
                    }
                    [self didCallPlugInFunction];
                }
                return;
            }
            
            frame = [newWebView mainFrame];
            core(frame)->tree()->setName(frameName);
            [[newWebView _UIDelegateForwarder] webViewShow:newWebView];
        }
    }

    if (JSString) {
        ASSERT(frame == nil || [self webFrame] == frame);
        [self evaluateJavaScriptPluginRequest:pluginRequest];
    } else {
        [frame loadRequest:request];
        if ([pluginRequest sendNotification]) {
            // Check if another plug-in view or even this view is waiting for the frame to load.
            // If it is, tell it that the load was cancelled because it will be anyway.
            WebNetscapePluginView *view = [frame _internalLoadDelegate];
            if (view != nil) {
                ASSERT([view isKindOfClass:[WebNetscapePluginView class]]);
                [view webFrame:frame didFinishLoadWithReason:NPRES_USER_BREAK];
            }
            [_pendingFrameLoads setObject:pluginRequest forKey:frame];
            [frame _setInternalLoadDelegate:self];
        }
    }
}

- (NPError)loadRequest:(NSMutableURLRequest *)request inTarget:(const char *)cTarget withNotifyData:(void *)notifyData sendNotification:(BOOL)sendNotification
{
    NSURL *URL = [request URL];

    if (!URL) 
        return NPERR_INVALID_URL;

    // Don't allow requests to be loaded when the document loader is stopping all loaders.
    if ([[self dataSource] _documentLoader]->isStopping())
        return NPERR_GENERIC_ERROR;
    
    NSString *target = nil;
    if (cTarget) {
        // Find the frame given the target string.
        target = [NSString stringWithCString:cTarget encoding:NSISOLatin1StringEncoding];
    }
    WebFrame *frame = [self webFrame];

    // don't let a plugin start any loads if it is no longer part of a document that is being 
    // displayed unless the loads are in the same frame as the plugin.
    if ([[self dataSource] _documentLoader] != core([self webFrame])->loader()->activeDocumentLoader() &&
        (!cTarget || [frame findFrameNamed:target] != frame)) {
        return NPERR_GENERIC_ERROR; 
    }
    
    NSString *JSString = [URL _webkit_scriptIfJavaScriptURL];
    if (JSString != nil) {
        if (![[[self webView] preferences] isJavaScriptEnabled]) {
            // Return NPERR_GENERIC_ERROR if JS is disabled. This is what Mozilla does.
            return NPERR_GENERIC_ERROR;
        } else if (cTarget == NULL && _mode == NP_FULL) {
            // Don't allow a JavaScript request from a standalone plug-in that is self-targetted
            // because this can cause the user to be redirected to a blank page (3424039).
            return NPERR_INVALID_PARAM;
        }
    } else {
        if (!core([self webFrame])->document()->securityOrigin()->canDisplay(URL))
            return NPERR_GENERIC_ERROR;
    }
        
    if (cTarget || JSString) {
        // Make when targetting a frame or evaluating a JS string, perform the request after a delay because we don't
        // want to potentially kill the plug-in inside of its URL request.
        
        if (JSString && target && [frame findFrameNamed:target] != frame) {
            // For security reasons, only allow JS requests to be made on the frame that contains the plug-in.
            return NPERR_INVALID_PARAM;
        }
        
        bool currentEventIsUserGesture = false;
        if (_eventHandler)
            currentEventIsUserGesture = _eventHandler->currentEventIsUserGesture();
        
        WebPluginRequest *pluginRequest = [[WebPluginRequest alloc] initWithRequest:request 
                                                                          frameName:target
                                                                         notifyData:notifyData 
                                                                   sendNotification:sendNotification
                                                            didStartFromUserGesture:currentEventIsUserGesture];
        [self performSelector:@selector(loadPluginRequest:) withObject:pluginRequest afterDelay:0];
        [pluginRequest release];
    } else {
        RefPtr<WebNetscapePluginStream> stream = WebNetscapePluginStream::create(request, plugin, sendNotification, notifyData);

        streams.add(stream.get());
        stream->start();
    }
    
    return NPERR_NO_ERROR;
}

-(NPError)getURLNotify:(const char *)URLCString target:(const char *)cTarget notifyData:(void *)notifyData
{
    LOG(Plugins, "NPN_GetURLNotify: %s target: %s", URLCString, cTarget);

    NSMutableURLRequest *request = [self requestWithURLCString:URLCString];
    return [self loadRequest:request inTarget:cTarget withNotifyData:notifyData sendNotification:YES];
}

-(NPError)getURL:(const char *)URLCString target:(const char *)cTarget
{
    LOG(Plugins, "NPN_GetURL: %s target: %s", URLCString, cTarget);

    NSMutableURLRequest *request = [self requestWithURLCString:URLCString];
    return [self loadRequest:request inTarget:cTarget withNotifyData:NULL sendNotification:NO];
}

- (NPError)_postURL:(const char *)URLCString
             target:(const char *)target
                len:(UInt32)len
                buf:(const char *)buf
               file:(NPBool)file
         notifyData:(void *)notifyData
   sendNotification:(BOOL)sendNotification
       allowHeaders:(BOOL)allowHeaders
{
    if (!URLCString || !len || !buf) {
        return NPERR_INVALID_PARAM;
    }
    
    NSData *postData = nil;

    if (file) {
        // If we're posting a file, buf is either a file URL or a path to the file.
        NSString *bufString = (NSString *)CFStringCreateWithCString(kCFAllocatorDefault, buf, kCFStringEncodingWindowsLatin1);
        if (!bufString) {
            return NPERR_INVALID_PARAM;
        }
        NSURL *fileURL = [NSURL _web_URLWithDataAsString:bufString];
        NSString *path;
        if ([fileURL isFileURL]) {
            path = [fileURL path];
        } else {
            path = bufString;
        }
        postData = [NSData dataWithContentsOfFile:[path _webkit_fixedCarbonPOSIXPath]];
        CFRelease(bufString);
        if (!postData) {
            return NPERR_FILE_NOT_FOUND;
        }
    } else {
        postData = [NSData dataWithBytes:buf length:len];
    }

    if ([postData length] == 0) {
        return NPERR_INVALID_PARAM;
    }

    NSMutableURLRequest *request = [self requestWithURLCString:URLCString];
    [request setHTTPMethod:@"POST"];
    
    if (allowHeaders) {
        if ([postData _web_startsWithBlankLine]) {
            postData = [postData subdataWithRange:NSMakeRange(1, [postData length] - 1)];
        } else {
            NSInteger location = [postData _web_locationAfterFirstBlankLine];
            if (location != NSNotFound) {
                // If the blank line is somewhere in the middle of postData, everything before is the header.
                NSData *headerData = [postData subdataWithRange:NSMakeRange(0, location)];
                NSMutableDictionary *header = [headerData _webkit_parseRFC822HeaderFields];
                unsigned dataLength = [postData length] - location;

                // Sometimes plugins like to set Content-Length themselves when they post,
                // but WebFoundation does not like that. So we will remove the header
                // and instead truncate the data to the requested length.
                NSString *contentLength = [header objectForKey:@"Content-Length"];

                if (contentLength != nil)
                    dataLength = std::min<unsigned>([contentLength intValue], dataLength);
                [header removeObjectForKey:@"Content-Length"];

                if ([header count] > 0) {
                    [request setAllHTTPHeaderFields:header];
                }
                // Everything after the blank line is the actual content of the POST.
                postData = [postData subdataWithRange:NSMakeRange(location, dataLength)];

            }
        }
        if ([postData length] == 0) {
            return NPERR_INVALID_PARAM;
        }
    }

    // Plug-ins expect to receive uncached data when doing a POST (3347134).
    [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];
    [request setHTTPBody:postData];
    
    return [self loadRequest:request inTarget:target withNotifyData:notifyData sendNotification:sendNotification];
}

- (NPError)postURLNotify:(const char *)URLCString
                  target:(const char *)target
                     len:(UInt32)len
                     buf:(const char *)buf
                    file:(NPBool)file
              notifyData:(void *)notifyData
{
    LOG(Plugins, "NPN_PostURLNotify: %s", URLCString);
    return [self _postURL:URLCString target:target len:len buf:buf file:file notifyData:notifyData sendNotification:YES allowHeaders:YES];
}

-(NPError)postURL:(const char *)URLCString
           target:(const char *)target
              len:(UInt32)len
              buf:(const char *)buf
             file:(NPBool)file
{
    LOG(Plugins, "NPN_PostURL: %s", URLCString);        
    // As documented, only allow headers to be specified via NPP_PostURL when using a file.
    return [self _postURL:URLCString target:target len:len buf:buf file:file notifyData:NULL sendNotification:NO allowHeaders:file];
}

-(NPError)newStream:(NPMIMEType)type target:(const char *)target stream:(NPStream**)stream
{
    LOG(Plugins, "NPN_NewStream");
    return NPERR_GENERIC_ERROR;
}

-(NPError)write:(NPStream*)stream len:(SInt32)len buffer:(void *)buffer
{
    LOG(Plugins, "NPN_Write");
    return NPERR_GENERIC_ERROR;
}

-(NPError)destroyStream:(NPStream*)stream reason:(NPReason)reason
{
    LOG(Plugins, "NPN_DestroyStream");
    // This function does a sanity check to ensure that the NPStream provided actually
    // belongs to the plug-in that provided it, which fixes a crash in the DivX 
    // plug-in: <rdar://problem/5093862> | http://bugs.webkit.org/show_bug.cgi?id=13203
    if (!stream || WebNetscapePluginStream::ownerForStream(stream) != plugin) {
        LOG(Plugins, "Invalid NPStream passed to NPN_DestroyStream: %p", stream);
        return NPERR_INVALID_INSTANCE_ERROR;
    }
    
    WebNetscapePluginStream* browserStream = static_cast<WebNetscapePluginStream*>(stream->ndata);
    browserStream->cancelLoadAndDestroyStreamWithError(browserStream->errorForReason(reason));
    
    return NPERR_NO_ERROR;
}

- (const char *)userAgent
{
    NSString *userAgent = [[self webView] userAgentForURL:_baseURL.get()];
    
    if (_isSilverlight) {
        // Silverlight has a workaround for a leak in Safari 2. This workaround is 
        // applied when the user agent does not contain "Version/3" so we append it
        // at the end of the user agent.
        userAgent = [userAgent stringByAppendingString:@" Version/3.2.1"];
    }        
        
    return [userAgent UTF8String];
}

-(void)status:(const char *)message
{    
    CFStringRef status = CFStringCreateWithCString(NULL, message ? message : "", kCFStringEncodingUTF8);
    if (!status) {
        LOG_ERROR("NPN_Status: the message was not valid UTF-8");
        return;
    }
    
    LOG(Plugins, "NPN_Status: %@", status);
    WebView *wv = [self webView];
    [[wv _UIDelegateForwarder] webView:wv setStatusText:(NSString *)status];
    CFRelease(status);
}

-(void)invalidateRect:(NPRect *)invalidRect
{
    LOG(Plugins, "NPN_InvalidateRect");
    [self invalidatePluginContentRect:NSMakeRect(invalidRect->left, invalidRect->top,
        (float)invalidRect->right - invalidRect->left, (float)invalidRect->bottom - invalidRect->top)];
}

- (void)invalidateRegion:(NPRegion)invalidRegion
{
    LOG(Plugins, "NPN_InvalidateRegion");
    NSRect invalidRect = NSZeroRect;
    switch (drawingModel) {
#ifndef NP_NO_QUICKDRAW
        case NPDrawingModelQuickDraw:
        {
            ::Rect qdRect;
            GetRegionBounds((NPQDRegion)invalidRegion, &qdRect);
            invalidRect = NSMakeRect(qdRect.left, qdRect.top, qdRect.right - qdRect.left, qdRect.bottom - qdRect.top);
        }
        break;
#endif /* NP_NO_QUICKDRAW */
        
        case NPDrawingModelCoreGraphics:
        {
            CGRect cgRect = CGPathGetBoundingBox((NPCGRegion)invalidRegion);
            invalidRect = *(NSRect*)&cgRect;
            break;
        }
        default:
            ASSERT_NOT_REACHED();
        break;
    }
    
    [self invalidatePluginContentRect:invalidRect];
}

-(void)forceRedraw
{
    LOG(Plugins, "forceRedraw");
    [self invalidatePluginContentRect:[self bounds]];
    [[self window] displayIfNeeded];
}

- (NPError)getVariable:(NPNVariable)variable value:(void *)value
{
    switch (static_cast<unsigned>(variable)) {
        case NPNVWindowNPObject:
        {
            Frame* frame = core([self webFrame]);
            NPObject* windowScriptObject = frame ? frame->script()->windowScriptNPObject() : 0;

            // Return value is expected to be retained, as described here: <http://www.mozilla.org/projects/plugins/npruntime.html#browseraccess>
            if (windowScriptObject)
                _NPN_RetainObject(windowScriptObject);
            
            void **v = (void **)value;
            *v = windowScriptObject;

            return NPERR_NO_ERROR;
        }

        case NPNVPluginElementNPObject:
        {
            if (!_element)
                return NPERR_GENERIC_ERROR;
            
            NPObject *plugInScriptObject = _element->getNPObject();

            // Return value is expected to be retained, as described here: <http://www.mozilla.org/projects/plugins/npruntime.html#browseraccess>
            if (plugInScriptObject)
                _NPN_RetainObject(plugInScriptObject);

            void **v = (void **)value;
            *v = plugInScriptObject;

            return NPERR_NO_ERROR;
        }
        
        case NPNVpluginDrawingModel:
        {
            *(NPDrawingModel *)value = drawingModel;
            return NPERR_NO_ERROR;
        }

#ifndef NP_NO_QUICKDRAW
        case NPNVsupportsQuickDrawBool:
        {
            *(NPBool *)value = TRUE;
            return NPERR_NO_ERROR;
        }
#endif /* NP_NO_QUICKDRAW */
        
        case NPNVsupportsCoreGraphicsBool:
        {
            *(NPBool *)value = TRUE;
            return NPERR_NO_ERROR;
        }

        case NPNVsupportsOpenGLBool:
        {
            *(NPBool *)value = FALSE;
            return NPERR_NO_ERROR;
        }
        
        case NPNVsupportsCoreAnimationBool:
        {
            *(NPBool *)value = TRUE;
            return NPERR_NO_ERROR;
        }
            
#ifndef NP_NO_CARBON
        case NPNVsupportsCarbonBool:
        {
            *(NPBool *)value = TRUE;
            return NPERR_NO_ERROR;
        }
#endif /* NP_NO_CARBON */

        case NPNVsupportsCocoaBool:
        {
            *(NPBool *)value = TRUE;
            return NPERR_NO_ERROR;
        }

        case NPNVprivateModeBool:
        {
            *(NPBool *)value = _isPrivateBrowsingEnabled;
            return NPERR_NO_ERROR;
        }

        case WKNVBrowserContainerCheckFuncs:
        {
            *(WKNBrowserContainerCheckFuncs **)value = browserContainerCheckFuncs();
            return NPERR_NO_ERROR;
        }
#if USE(ACCELERATED_COMPOSITING)
        case WKNVSupportsCompositingCoreAnimationPluginsBool:
        {
            *(NPBool *)value = [[[self webView] preferences] acceleratedCompositingEnabled];
            return NPERR_NO_ERROR;
        }
#endif
        default:
            break;
    }

    return NPERR_GENERIC_ERROR;
}

- (NPError)setVariable:(NPPVariable)variable value:(void *)value
{
    switch (variable) {
        case NPPVpluginDrawingModel:
        {
            // Can only set drawing model inside NPP_New()
            if (self != [[self class] currentPluginView])
                return NPERR_GENERIC_ERROR;
            
            // Check for valid, supported drawing model
            NPDrawingModel newDrawingModel = (NPDrawingModel)(uintptr_t)value;
            switch (newDrawingModel) {
                // Supported drawing models:
#ifndef NP_NO_QUICKDRAW
                case NPDrawingModelQuickDraw:
#endif
                case NPDrawingModelCoreGraphics:
                case NPDrawingModelCoreAnimation:
                    drawingModel = newDrawingModel;
                    return NPERR_NO_ERROR;
                    

                // Unsupported (or unknown) drawing models:
                default:
                    LOG(Plugins, "Plugin %@ uses unsupported drawing model: %d", _eventHandler.get(), drawingModel);
                    return NPERR_GENERIC_ERROR;
            }
        }
        
        case NPPVpluginEventModel:
        {
            // Can only set event model inside NPP_New()
            if (self != [[self class] currentPluginView])
                return NPERR_GENERIC_ERROR;
            
            // Check for valid, supported event model
            NPEventModel newEventModel = (NPEventModel)(uintptr_t)value;
            switch (newEventModel) {
                // Supported event models:
#ifndef NP_NO_CARBON
                case NPEventModelCarbon:
#endif
                case NPEventModelCocoa:
                    eventModel = newEventModel;
                    return NPERR_NO_ERROR;
                    
                    // Unsupported (or unknown) event models:
                default:
                    LOG(Plugins, "Plugin %@ uses unsupported event model: %d", _eventHandler.get(), eventModel);
                    return NPERR_GENERIC_ERROR;
            }
        }
            
        default:
            return NPERR_GENERIC_ERROR;
    }
}

- (uint32_t)scheduleTimerWithInterval:(uint32_t)interval repeat:(NPBool)repeat timerFunc:(void (*)(NPP npp, uint32_t timerID))timerFunc
{
    if (!timerFunc)
        return 0;
    
    if (!timers)
        timers = new HashMap<uint32_t, PluginTimer*>;
    
    uint32_t timerID;
    
    do {
        timerID = ++currentTimerID;
    } while (timers->contains(timerID) || timerID == 0);
    
    PluginTimer* timer = new PluginTimer(plugin, timerID, interval, repeat, timerFunc);
    timers->set(timerID, timer);

    if (_shouldFireTimers)
        timer->start(_isCompletelyObscured);
    
    return timerID;
}

- (void)unscheduleTimer:(uint32_t)timerID
{
    if (!timers)
        return;
    
    if (PluginTimer* timer = timers->take(timerID))
        delete timer;
}

- (NPError)popUpContextMenu:(NPMenu *)menu
{
    NSEvent *currentEvent = [NSApp currentEvent];
    
    // NPN_PopUpContextMenu must be called from within the plug-in's NPP_HandleEvent.
    if (!currentEvent)
        return NPERR_GENERIC_ERROR;
    
    [NSMenu popUpContextMenu:(NSMenu *)menu withEvent:currentEvent forView:self];
    return NPERR_NO_ERROR;
}

- (NPError)getVariable:(NPNURLVariable)variable forURL:(const char*)url value:(char**)value length:(uint32_t*)length
{
    switch (variable) {
        case NPNURLVCookie: {
            if (!value)
                break;
            
            NSURL *URL = [self URLWithCString:url];
            if (!URL)
                break;
            
            if (Frame* frame = core([self webFrame])) {
                String cookieString = cookies(frame->document(), URL); 
                CString cookieStringUTF8 = cookieString.utf8();
                if (cookieStringUTF8.isNull())
                    return NPERR_GENERIC_ERROR;

                *value = static_cast<char*>(NPN_MemAlloc(cookieStringUTF8.length()));
                memcpy(*value, cookieStringUTF8.data(), cookieStringUTF8.length());
                
                if (length)
                    *length = cookieStringUTF8.length();
                return NPERR_NO_ERROR;
            }
            break;
        }
        case NPNURLVProxy: {
            if (!value)
                break;
            
            NSURL *URL = [self URLWithCString:url];
            if (!URL)
                break;

            Vector<ProxyServer> proxyServers = proxyServersForURL(URL, 0);
            CString proxiesUTF8 = toString(proxyServers).utf8();
            
            *value = static_cast<char*>(NPN_MemAlloc(proxiesUTF8.length()));
            memcpy(*value, proxiesUTF8.data(), proxiesUTF8.length());
            
           if (length)
               *length = proxiesUTF8.length();
            
            return NPERR_NO_ERROR;
        }
    }
    return NPERR_GENERIC_ERROR;
}

- (NPError)setVariable:(NPNURLVariable)variable forURL:(const char*)url value:(const char*)value length:(uint32_t)length
{
    switch (variable) {
        case NPNURLVCookie: {
            NSURL *URL = [self URLWithCString:url];
            if (!URL)
                break;
            
            String cookieString = String::fromUTF8(value, length);
            if (!cookieString)
                break;
            
            if (Frame* frame = core([self webFrame])) {
                setCookies(frame->document(), URL, cookieString);
                return NPERR_NO_ERROR;
            }
            
            break;
        }
        case NPNURLVProxy:
            // Can't set the proxy for a URL.
            break;
    }
    return NPERR_GENERIC_ERROR;
}

- (NPError)getAuthenticationInfoWithProtocol:(const char*)protocolStr host:(const char*)hostStr port:(int32_t)port scheme:(const char*)schemeStr realm:(const char*)realmStr
                                    username:(char**)usernameStr usernameLength:(uint32_t*)usernameLength 
                                    password:(char**)passwordStr passwordLength:(uint32_t*)passwordLength
{
    if (!protocolStr || !hostStr || !schemeStr || !realmStr || !usernameStr || !usernameLength || !passwordStr || !passwordLength)
        return NPERR_GENERIC_ERROR;
  
    CString username;
    CString password;
    if (!getAuthenticationInfo(protocolStr, hostStr, port, schemeStr, realmStr, username, password))
        return NPERR_GENERIC_ERROR;
    
    *usernameLength = username.length();
    *usernameStr = static_cast<char*>(NPN_MemAlloc(username.length()));
    memcpy(*usernameStr, username.data(), username.length());
    
    *passwordLength = password.length();
    *passwordStr = static_cast<char*>(NPN_MemAlloc(password.length()));
    memcpy(*passwordStr, password.data(), password.length());
    
    return NPERR_NO_ERROR;
}

- (char*)resolveURL:(const char*)url forTarget:(const char*)target
{
    CString location = [self resolvedURLStringForURL:url target:target];

    if (location.isNull())
        return 0;
    
    // We use strdup here because the caller needs to free it with NPN_MemFree (which calls free).
    return strdup(location.data());
}

@end

@implementation WebNetscapePluginView (Internal)

- (BOOL)_shouldCancelSrcStream
{
    ASSERT(_isStarted);
    
    // Check if we should cancel the load
    NPBool cancelSrcStream = 0;
    if ([_pluginPackage.get() pluginFuncs]->getvalue &&
        [_pluginPackage.get() pluginFuncs]->getvalue(plugin, NPPVpluginCancelSrcStream, &cancelSrcStream) == NPERR_NO_ERROR && cancelSrcStream)
        return YES;
    
    return NO;
}

// Work around Silverlight full screen performance issue by maintaining an accelerated GL pixel format.
// We can safely remove it at some point in the future when both:
// 1) Microsoft releases a genuine fix for 7288546.
// 2) Enough Silverlight users update to the new Silverlight.
// For now, we'll distinguish older broken versions of Silverlight by asking the plug-in if it resolved its full screen badness.
- (void)_workaroundSilverlightFullscreenBug:(BOOL)initializedPlugin
{
    ASSERT(_isSilverlight);
    NPBool isFullscreenPerformanceIssueFixed = 0;
    NPPluginFuncs *pluginFuncs = [_pluginPackage.get() pluginFuncs];
    if (pluginFuncs->getvalue && pluginFuncs->getvalue(plugin, static_cast<NPPVariable>(WKNVSilverlightFullscreenPerformanceIssueFixed), &isFullscreenPerformanceIssueFixed) == NPERR_NO_ERROR && isFullscreenPerformanceIssueFixed)
        return;
    
    static CGLPixelFormatObj pixelFormatObject = 0;
    static unsigned refCount = 0;
    
    if (initializedPlugin) {
        refCount++;
        if (refCount == 1) {
            const CGLPixelFormatAttribute attributes[] = { kCGLPFAAccelerated, static_cast<CGLPixelFormatAttribute>(0) };
            GLint npix;
            CGLChoosePixelFormat(attributes, &pixelFormatObject, &npix);
        }  
    } else {
        ASSERT(pixelFormatObject);
        refCount--;
        if (!refCount) 
            CGLReleasePixelFormat(pixelFormatObject);
    }
}

- (NPError)_createPlugin
{
    plugin = (NPP)calloc(1, sizeof(NPP_t));
    plugin->ndata = self;

    ASSERT([_pluginPackage.get() pluginFuncs]->newp);

    // NPN_New(), which creates the plug-in instance, should never be called while calling a plug-in function for that instance.
    ASSERT(pluginFunctionCallDepth == 0);

    PluginMainThreadScheduler::scheduler().registerPlugin(plugin);

    _isFlash = [_pluginPackage.get() bundleIdentifier] == "com.macromedia.Flash Player.plugin";
    _isSilverlight = [_pluginPackage.get() bundleIdentifier] == "com.microsoft.SilverlightPlugin";

    [[self class] setCurrentPluginView:self];
    NPError npErr = [_pluginPackage.get() pluginFuncs]->newp((char *)[_MIMEType.get() cString], plugin, _mode, argsCount, cAttributes, cValues, NULL);
    [[self class] setCurrentPluginView:nil];
    if (_isSilverlight)
        [self _workaroundSilverlightFullscreenBug:YES];
    LOG(Plugins, "NPP_New: %d", npErr);
    return npErr;
}

- (void)_destroyPlugin
{
    PluginMainThreadScheduler::scheduler().unregisterPlugin(plugin);
    
    if (_isSilverlight)
        [self _workaroundSilverlightFullscreenBug:NO];
    
    NPError npErr;
    npErr = ![_pluginPackage.get() pluginFuncs]->destroy(plugin, NULL);
    LOG(Plugins, "NPP_Destroy: %d", npErr);
    
    if (Frame* frame = core([self webFrame]))
        frame->script()->cleanupScriptObjectsForPlugin(self);
        
    free(plugin);
    plugin = NULL;
}

- (NSBitmapImageRep *)_printedPluginBitmap
{
#ifdef NP_NO_QUICKDRAW
    return nil;
#else
    // Cannot print plugins that do not implement NPP_Print
    if (![_pluginPackage.get() pluginFuncs]->print)
        return nil;

    // This NSBitmapImageRep will share its bitmap buffer with a GWorld that the plugin will draw into.
    // The bitmap is created in 32-bits-per-pixel ARGB format, which is the default GWorld pixel format.
    NSBitmapImageRep *bitmap = [[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                         pixelsWide:window.width
                                                         pixelsHigh:window.height
                                                         bitsPerSample:8
                                                         samplesPerPixel:4
                                                         hasAlpha:YES
                                                         isPlanar:NO
                                                         colorSpaceName:NSDeviceRGBColorSpace
                                                         bitmapFormat:NSAlphaFirstBitmapFormat
                                                         bytesPerRow:0
                                                         bitsPerPixel:0] autorelease];
    ASSERT(bitmap);
    
    // Create a GWorld with the same underlying buffer into which the plugin can draw
    ::Rect printGWorldBounds;
    SetRect(&printGWorldBounds, 0, 0, window.width, window.height);
    GWorldPtr printGWorld;
    if (NewGWorldFromPtr(&printGWorld,
                         k32ARGBPixelFormat,
                         &printGWorldBounds,
                         NULL,
                         NULL,
                         0,
                         (Ptr)[bitmap bitmapData],
                         [bitmap bytesPerRow]) != noErr) {
        LOG_ERROR("Could not create GWorld for printing");
        return nil;
    }
    
    /// Create NPWindow for the GWorld
    NPWindow printNPWindow;
    printNPWindow.window = &printGWorld; // Normally this is an NP_Port, but when printing it is the actual CGrafPtr
    printNPWindow.x = 0;
    printNPWindow.y = 0;
    printNPWindow.width = window.width;
    printNPWindow.height = window.height;
    printNPWindow.clipRect.top = 0;
    printNPWindow.clipRect.left = 0;
    printNPWindow.clipRect.right = window.width;
    printNPWindow.clipRect.bottom = window.height;
    printNPWindow.type = NPWindowTypeDrawable; // Offscreen graphics port as opposed to a proper window
    
    // Create embed-mode NPPrint
    NPPrint npPrint;
    npPrint.mode = NP_EMBED;
    npPrint.print.embedPrint.window = printNPWindow;
    npPrint.print.embedPrint.platformPrint = printGWorld;
    
    // Tell the plugin to print into the GWorld
    [self willCallPlugInFunction];
    {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        [_pluginPackage.get() pluginFuncs]->print(plugin, &npPrint);
    }
    [self didCallPlugInFunction];

    // Don't need the GWorld anymore
    DisposeGWorld(printGWorld);
        
    return bitmap;
#endif
}

- (void)_redeliverStream
{
    if ([self dataSource] && _isStarted) {
        // Deliver what has not been passed to the plug-in up to this point.
        if (_dataLengthReceived > 0) {
            NSData *data = [[[self dataSource] data] subdataWithRange:NSMakeRange(0, _dataLengthReceived)];
            _dataLengthReceived = 0;
            [self pluginView:self receivedData:data];
            if (![[self dataSource] isLoading]) {
                if (_error)
                    [self pluginView:self receivedError:_error.get()];
                else
                    [self pluginViewFinishedLoading:self];
            }
        }
    }
}

@end

#endif
