/*
 * Copyright (C) 2009, 2011, 2012 Apple Inc. All rights reserved.
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
#import "PDFPlugin.h"

#if ENABLE(PDFKIT_PLUGIN)

#import "ArgumentCoders.h"
#import "AttributedString.h"
#import "DataReference.h"
#import "DictionaryPopupInfo.h"
#import "PDFAnnotationTextWidgetDetails.h"
#import "PDFKitImports.h"
#import "PDFLayerControllerDetails.h"
#import "PDFPluginAnnotation.h"
#import "PDFPluginPasswordField.h"
#import "PluginView.h"
#import "WebContextMessages.h"
#import "WebCoreArgumentCoders.h"
#import "WebEvent.h"
#import "WebEventConversion.h"
#import "WebPage.h"
#import "WebPageProxyMessages.h"
#import "WebProcess.h"
#import "WKAccessibilityWebPageObject.h"
#import <PDFKit/PDFKit.h>
#import <QuartzCore/QuartzCore.h>
#import <WebCore/Cursor.h>
#import <WebCore/FocusController.h>
#import <WebCore/FormState.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameView.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/HTMLElement.h>
#import <WebCore/HTMLFormElement.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/MouseEvent.h>
#import <WebCore/Page.h>
#import <WebCore/Pasteboard.h>
#import <WebCore/PluginDocument.h>
#import <WebCore/ScrollbarTheme.h>
#import <WebCore/UUID.h>
#import <WebKitSystemInterface.h>
#import <wtf/CurrentTime.h>

using namespace WebCore;

// Set overflow: hidden on the annotation container so <input> elements scrolled out of view don't show
// scrollbars on the body. We can't add annotations directly to the body, because overflow: hidden on the body
// will break rubber-banding.
static const char* annotationStyle =
"#annotationContainer {"
"    overflow: hidden; "
"    position: absolute; "
"    pointer-events: none; "
"    top: 0; "
"    left: 0; "
"    right: 0; "
"    bottom: 0; "
"    display: -webkit-box; "
"    -webkit-box-align: center; "
"    -webkit-box-pack: center; "
"} "
".annotation { "
"    position: absolute; "
"    pointer-events: auto; "
"} "
"textarea.annotation { "
"    resize: none; "
"} "
"input.annotation[type='password'] { "
"    position: static; "
"    width: 200px; "
"    margin-top: 100px; "
"} ";

// In non-continuous modes, a single scroll event with a magnitude of >= 20px
// will jump to the next or previous page, to match PDFKit behavior.
static const int defaultScrollMagnitudeThresholdForPageFlip = 20;

@interface WKPDFPluginAccessibilityObject : NSObject
{
    PDFLayerController *_pdfLayerController;
    NSObject *_parent;
    WebKit::PDFPlugin* _pdfPlugin;
}

@property(assign) PDFLayerController *pdfLayerController;
@property(assign) NSObject *parent;
@property(assign) WebKit::PDFPlugin* pdfPlugin;

- (id)initWithPDFPlugin:(WebKit::PDFPlugin *)plugin;

@end

@implementation WKPDFPluginAccessibilityObject

@synthesize pdfLayerController = _pdfLayerController;
@synthesize parent = _parent;
@synthesize pdfPlugin = _pdfPlugin;

- (id)initWithPDFPlugin:(WebKit::PDFPlugin *)plugin
{
    if (!(self = [super init]))
        return nil;

    _pdfPlugin = plugin;

    return self;
}

- (BOOL)accessibilityIsIgnored
{
    return NO;
}

- (id)accessibilityAttributeValue:(NSString *)attribute
{
    if ([attribute isEqualToString:NSAccessibilityParentAttribute])
        return _parent;
    else if ([attribute isEqualToString:NSAccessibilityValueAttribute])
        return [_pdfLayerController accessibilityValueAttribute];
    else if ([attribute isEqualToString:NSAccessibilitySelectedTextAttribute])
        return [_pdfLayerController accessibilitySelectedTextAttribute];
    else if ([attribute isEqualToString:NSAccessibilitySelectedTextRangeAttribute])
        return [_pdfLayerController accessibilitySelectedTextRangeAttribute];
    else if ([attribute isEqualToString:NSAccessibilityNumberOfCharactersAttribute])
        return [_pdfLayerController accessibilityNumberOfCharactersAttribute];
    else if ([attribute isEqualToString:NSAccessibilityVisibleCharacterRangeAttribute])
        return [_pdfLayerController accessibilityVisibleCharacterRangeAttribute];
    else if ([attribute isEqualToString:NSAccessibilityTopLevelUIElementAttribute])
        return [_parent accessibilityAttributeValue:NSAccessibilityTopLevelUIElementAttribute];
    else if ([attribute isEqualToString:NSAccessibilityRoleAttribute])
        return [_pdfLayerController accessibilityRoleAttribute];
    else if ([attribute isEqualToString:NSAccessibilityRoleDescriptionAttribute])
        return [_pdfLayerController accessibilityRoleDescriptionAttribute];
    else if ([attribute isEqualToString:NSAccessibilityWindowAttribute])
        return [_parent accessibilityAttributeValue:NSAccessibilityWindowAttribute];
    else if ([attribute isEqualToString:NSAccessibilitySizeAttribute])
        return [NSValue valueWithSize:_pdfPlugin->boundsOnScreen().size()];
    else if ([attribute isEqualToString:NSAccessibilityFocusedAttribute])
        return [_parent accessibilityAttributeValue:NSAccessibilityFocusedAttribute];
    else if ([attribute isEqualToString:NSAccessibilityEnabledAttribute])
        return [_parent accessibilityAttributeValue:NSAccessibilityEnabledAttribute];
    else if ([attribute isEqualToString:NSAccessibilityPositionAttribute])
        return [NSValue valueWithPoint:_pdfPlugin->boundsOnScreen().location()];

    return 0;
}

- (id)accessibilityAttributeValue:(NSString *)attribute forParameter:(id)parameter
{
    if ([attribute isEqualToString:NSAccessibilityBoundsForRangeParameterizedAttribute]) {
        NSRect boundsInPDFViewCoordinates = [[_pdfLayerController accessibilityBoundsForRangeAttributeForParameter:parameter] rectValue];
        NSRect boundsInScreenCoordinates = _pdfPlugin->convertFromPDFViewToScreen(boundsInPDFViewCoordinates);
        return [NSValue valueWithRect:boundsInScreenCoordinates];;
    } else if ([attribute isEqualToString:NSAccessibilityLineForIndexParameterizedAttribute])
        return [_pdfLayerController accessibilityLineForIndexAttributeForParameter:parameter];
    else if ([attribute isEqualToString:NSAccessibilityRangeForLineParameterizedAttribute])
        return [_pdfLayerController accessibilityRangeForLineAttributeForParameter:parameter];
    else if ([attribute isEqualToString:NSAccessibilityStringForRangeParameterizedAttribute])
        return [_pdfLayerController accessibilityStringForRangeAttributeForParameter:parameter];

    return 0;
}

- (CPReadingModel *)readingModel
{
    return [_pdfLayerController readingModel];
}

- (NSArray *)accessibilityAttributeNames
{
    static NSArray *attributeNames = 0;

    if (!attributeNames)
        attributeNames = [[NSArray arrayWithObjects:NSAccessibilityValueAttribute,
            NSAccessibilitySelectedTextAttribute,
            NSAccessibilitySelectedTextRangeAttribute,
            NSAccessibilityNumberOfCharactersAttribute,
            NSAccessibilityVisibleCharacterRangeAttribute,
            NSAccessibilityParentAttribute,
            NSAccessibilityRoleAttribute,
            NSAccessibilityWindowAttribute,
            NSAccessibilityTopLevelUIElementAttribute,
            NSAccessibilityRoleDescriptionAttribute,
            NSAccessibilitySizeAttribute,
            NSAccessibilityFocusedAttribute,
            NSAccessibilityEnabledAttribute,
            NSAccessibilityPositionAttribute,
            nil] retain];

    return attributeNames;
}

- (NSArray *)accessibilityActionNames
{
    static NSArray *actionNames = 0;
    
    if (!actionNames)
        actionNames = [[NSArray arrayWithObject:NSAccessibilityShowMenuAction] retain];
    
    return actionNames;
}

- (void)accessibilityPerformAction:(NSString *)action
{
    if ([action isEqualToString:NSAccessibilityShowMenuAction])
        _pdfPlugin->showContextMenuAtPoint(IntRect(IntPoint(), _pdfPlugin->size()).center());
}

- (BOOL)accessibilityIsAttributeSettable:(NSString *)attribute
{
    return [_pdfLayerController accessibilityIsAttributeSettable:attribute];
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute
{
    return [_pdfLayerController accessibilitySetValue:value forAttribute:attribute];
}

- (NSArray *)accessibilityParameterizedAttributeNames
{
    return [_pdfLayerController accessibilityParameterizedAttributeNames];
}

- (id)accessibilityFocusedUIElement
{
    return self;
}

- (id)accessibilityHitTest:(NSPoint)point
{
    return self;
}

@end


@interface WKPDFPluginScrollbarLayer : CALayer
{
    WebKit::PDFPlugin* _pdfPlugin;
}

@property (assign) WebKit::PDFPlugin* pdfPlugin;

@end

@implementation WKPDFPluginScrollbarLayer

@synthesize pdfPlugin = _pdfPlugin;

- (id)initWithPDFPlugin:(WebKit::PDFPlugin *)plugin
{
    if (!(self = [super init]))
        return nil;
    
    _pdfPlugin = plugin;
    
    return self;
}

- (id<CAAction>)actionForKey:(NSString *)key
{
    return nil;
}

- (void)drawInContext:(CGContextRef)ctx
{
    _pdfPlugin->paintControlForLayerInContext(self, ctx);
}

@end

@interface WKPDFLayerControllerDelegate : NSObject<PDFLayerControllerDelegate>
{
    WebKit::PDFPlugin* _pdfPlugin;
}

@property(assign) WebKit::PDFPlugin* pdfPlugin;

@end

@implementation WKPDFLayerControllerDelegate

@synthesize pdfPlugin=_pdfPlugin;

- (id)initWithPDFPlugin:(WebKit::PDFPlugin *)plugin
{
    if (!(self = [super init]))
        return nil;
    
    _pdfPlugin = plugin;
    
    return self;
}

- (void)updateScrollPosition:(CGPoint)newPosition
{
    _pdfPlugin->notifyScrollPositionChanged(IntPoint(newPosition));
}

- (void)writeItemsToPasteboard:(NSArray *)items withTypes:(NSArray *)types
{
    _pdfPlugin->writeItemsToPasteboard(items, types);
}

- (void)showDefinitionForAttributedString:(NSAttributedString *)string atPoint:(CGPoint)point
{
    _pdfPlugin->showDefinitionForAttributedString(string, point);
}

- (void)performWebSearch:(NSString *)string
{
    _pdfPlugin->performWebSearch(string);
}

- (void)performSpotlightSearch:(NSString *)string
{
    _pdfPlugin->performSpotlightSearch(string);
}

- (void)openWithNativeApplication
{
    _pdfPlugin->openWithNativeApplication();
}

- (void)saveToPDF
{
    _pdfPlugin->saveToPDF();
}

- (void)pdfLayerController:(PDFLayerController *)pdfLayerController clickedLinkWithURL:(NSURL *)url
{
    _pdfPlugin->clickedLink(url);
}

- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeActiveAnnotation:(PDFAnnotation *)annotation
{
    _pdfPlugin->setActiveAnnotation(annotation);
}

- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeContentScaleFactor:(CGFloat)scaleFactor
{
    _pdfPlugin->notifyContentScaleFactorChanged(scaleFactor);
}

- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeDisplayMode:(int)mode
{
    _pdfPlugin->notifyDisplayModeChanged(mode);
}

- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeSelection:(PDFSelection *)selection
{
    _pdfPlugin->notifySelectionChanged(selection);
}

@end

namespace WebKit {

using namespace HTMLNames;

PassRefPtr<PDFPlugin> PDFPlugin::create(WebFrame* frame)
{
    return adoptRef(new PDFPlugin(frame));
}

PDFPlugin::PDFPlugin(WebFrame* frame)
    : SimplePDFPlugin(frame)
    , m_containerLayer(adoptNS([[CALayer alloc] init]))
    , m_contentLayer(adoptNS([[CALayer alloc] init]))
    , m_scrollCornerLayer(adoptNS([[WKPDFPluginScrollbarLayer alloc] initWithPDFPlugin:this]))
    , m_pdfLayerController(adoptNS([[pdfLayerControllerClass() alloc] init]))
    , m_pdfLayerControllerDelegate(adoptNS([[WKPDFLayerControllerDelegate alloc] initWithPDFPlugin:this]))
{
    m_pdfLayerController.get().delegate = m_pdfLayerControllerDelegate.get();
    m_pdfLayerController.get().parentLayer = m_contentLayer.get();

    if (supportsForms()) {
        Document* document = webFrame()->coreFrame()->document();
        m_annotationContainer = document->createElement(divTag, false);
        m_annotationContainer->setAttribute(idAttr, "annotationContainer");

        RefPtr<Element> m_annotationStyle = document->createElement(styleTag, false);
        m_annotationStyle->setTextContent(annotationStyle, ASSERT_NO_EXCEPTION);

        m_annotationContainer->appendChild(m_annotationStyle.get());
        document->body()->appendChild(m_annotationContainer.get());
    }

    m_accessibilityObject = adoptNS([[WKPDFPluginAccessibilityObject alloc] initWithPDFPlugin:this]);
    m_accessibilityObject.get().pdfLayerController = m_pdfLayerController.get();
    m_accessibilityObject.get().parent = webFrame()->page()->accessibilityRemoteObject();

    [m_containerLayer.get() addSublayer:m_contentLayer.get()];
    [m_containerLayer.get() addSublayer:m_scrollCornerLayer.get()];
}

PDFPlugin::~PDFPlugin()
{
}

void PDFPlugin::updateScrollbars()
{
    SimplePDFPlugin::updateScrollbars();
    
    if (m_verticalScrollbarLayer) {
        m_verticalScrollbarLayer.get().frame = verticalScrollbar()->frameRect();
        [m_verticalScrollbarLayer.get() setNeedsDisplay];
    }
    
    if (m_horizontalScrollbarLayer) {
        m_horizontalScrollbarLayer.get().frame = horizontalScrollbar()->frameRect();
        [m_horizontalScrollbarLayer.get() setNeedsDisplay];
    }
    
    if (m_scrollCornerLayer) {
        m_scrollCornerLayer.get().frame = scrollCornerRect();
        [m_scrollCornerLayer.get() setNeedsDisplay];
    }
}

PassRefPtr<Scrollbar> PDFPlugin::createScrollbar(ScrollbarOrientation orientation)
{
    RefPtr<Scrollbar> widget = Scrollbar::createNativeScrollbar(this, orientation, RegularScrollbar);
    if (orientation == HorizontalScrollbar) {
        m_horizontalScrollbarLayer = adoptNS([[WKPDFPluginScrollbarLayer alloc] initWithPDFPlugin:this]);
        [m_containerLayer.get() addSublayer:m_horizontalScrollbarLayer.get()];
    } else {
        m_verticalScrollbarLayer = adoptNS([[WKPDFPluginScrollbarLayer alloc] initWithPDFPlugin:this]);
        [m_containerLayer.get() addSublayer:m_verticalScrollbarLayer.get()];
    }
    didAddScrollbar(widget.get(), orientation);
    pluginView()->frame()->view()->addChild(widget.get());
    return widget.release();
}

void PDFPlugin::destroyScrollbar(ScrollbarOrientation orientation)
{
    SimplePDFPlugin::destroyScrollbar(orientation);

    if (orientation == HorizontalScrollbar) {
        [m_horizontalScrollbarLayer.get() removeFromSuperlayer];
        m_horizontalScrollbarLayer = 0;
    } else {
        [m_verticalScrollbarLayer.get() removeFromSuperlayer];
        m_verticalScrollbarLayer = 0;
    }
}

void PDFPlugin::pdfDocumentDidLoad()
{
    addArchiveResource();
    
    RetainPtr<PDFDocument> document = adoptNS([[pdfDocumentClass() alloc] initWithData:rawData()]);

    setPDFDocument(document);

    updatePageAndDeviceScaleFactors();

    [m_pdfLayerController.get() setFrameSize:size()];
    m_pdfLayerController.get().document = document.get();

    if (handlesPageScaleFactor())
        pluginView()->setPageScaleFactor([m_pdfLayerController.get() contentScaleFactor], IntPoint());

    notifyScrollPositionChanged(IntPoint([m_pdfLayerController.get() scrollPosition]));

    calculateSizes();
    updateScrollbars();

    runScriptsInPDFDocument();

    if ([document.get() isLocked])
        createPasswordEntryForm();
}

void PDFPlugin::createPasswordEntryForm()
{
    m_passwordField = PDFPluginPasswordField::create(m_pdfLayerController.get(), this);
    m_passwordField->attach(m_annotationContainer.get());
}

void PDFPlugin::attemptToUnlockPDF(const String& password)
{
    [m_pdfLayerController attemptToUnlockWithPassword:password];

    if (![pdfDocument() isLocked]) {
        m_passwordField = nullptr;

        calculateSizes();
        updateScrollbars();
    }
}

void PDFPlugin::updatePageAndDeviceScaleFactors()
{
    double newScaleFactor = controller()->contentsScaleFactor();
    if (!handlesPageScaleFactor())
        newScaleFactor *= webFrame()->page()->pageScaleFactor();

    [m_pdfLayerController.get() setDeviceScaleFactor:newScaleFactor];
}

void PDFPlugin::contentsScaleFactorChanged(float)
{
    updatePageAndDeviceScaleFactors();
}

void PDFPlugin::calculateSizes()
{
    if ([pdfDocument() isLocked]) {
        setPDFDocumentSize(IntSize(0, 0));
        return;
    }

    // FIXME: This should come straight from PDFKit.
    computePageBoxes();

    setPDFDocumentSize(IntSize([m_pdfLayerController.get() contentSizeRespectingZoom]));
}

void PDFPlugin::destroy()
{
    m_pdfLayerController.get().delegate = 0;

    if (webFrame()) {
        if (FrameView* frameView = webFrame()->coreFrame()->view())
            frameView->removeScrollableArea(this);
    }

    m_activeAnnotation = 0;
    m_annotationContainer = 0;

    destroyScrollbar(HorizontalScrollbar);
    destroyScrollbar(VerticalScrollbar);
    
    [m_scrollCornerLayer.get() removeFromSuperlayer];
    [m_contentLayer.get() removeFromSuperlayer];
}

void PDFPlugin::paint(GraphicsContext* graphicsContext, const IntRect& dirtyRect)
{
}
    
void PDFPlugin::paintControlForLayerInContext(CALayer *layer, CGContextRef context)
{
    GraphicsContext graphicsContext(context);
    GraphicsContextStateSaver stateSaver(graphicsContext);
    
    graphicsContext.setIsCALayerContext(true);
    
    if (layer == m_scrollCornerLayer) {
        IntRect scrollCornerRect = this->scrollCornerRect();
        graphicsContext.translate(-scrollCornerRect.x(), -scrollCornerRect.y());
        ScrollbarTheme::theme()->paintScrollCorner(0, &graphicsContext, scrollCornerRect);
        return;
    }
    
    Scrollbar* scrollbar = 0;
    
    if (layer == m_verticalScrollbarLayer)
        scrollbar = verticalScrollbar();
    else if (layer == m_horizontalScrollbarLayer)
        scrollbar = horizontalScrollbar();

    if (!scrollbar)
        return;
    
    graphicsContext.translate(-scrollbar->x(), -scrollbar->y());
    scrollbar->paint(&graphicsContext, scrollbar->frameRect());
}

PassRefPtr<ShareableBitmap> PDFPlugin::snapshot()
{
    if (size().isEmpty())
        return 0;

    float contentsScaleFactor = controller()->contentsScaleFactor();
    IntSize backingStoreSize = size();
    backingStoreSize.scale(contentsScaleFactor);

    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(backingStoreSize, ShareableBitmap::SupportsAlpha);
    OwnPtr<GraphicsContext> context = bitmap->createGraphicsContext();

    context->scale(FloatSize(contentsScaleFactor, -contentsScaleFactor));
    context->translate(0, -size().height());

    [m_pdfLayerController.get() snapshotInContext:context->platformContext()];

    return bitmap.release();
}

PlatformLayer* PDFPlugin::pluginLayer()
{
    return m_containerLayer.get();
}

IntPoint PDFPlugin::convertFromPluginToPDFView(const IntPoint& point) const
{
    return IntPoint(point.x(), size().height() - point.y());
}

IntPoint PDFPlugin::convertFromRootViewToPlugin(const IntPoint& point) const
{
    return m_rootViewToPluginTransform.mapPoint(point);
}

IntPoint PDFPlugin::convertFromPDFViewToRootView(const IntPoint& point) const
{
    IntPoint pointInPluginCoordinates(point.x(), size().height() - point.y());
    return m_rootViewToPluginTransform.inverse().mapPoint(pointInPluginCoordinates);
}

FloatRect PDFPlugin::convertFromPDFViewToScreen(const FloatRect& rect) const
{
    FrameView* frameView = webFrame()->coreFrame()->view();

    if (!frameView)
        return FloatRect();

    FloatPoint originInPluginCoordinates(rect.x(), size().height() - rect.y() - rect.height());
    FloatRect rectInRootViewCoordinates = m_rootViewToPluginTransform.inverse().mapRect(FloatRect(originInPluginCoordinates, rect.size()));

    return frameView->contentsToScreen(enclosingIntRect(rectInRootViewCoordinates));
}

IntRect PDFPlugin::boundsOnScreen() const
{
    FrameView* frameView = webFrame()->coreFrame()->view();

    if (!frameView)
        return IntRect();

    FloatRect bounds = FloatRect(FloatPoint(), size());
    FloatRect rectInRootViewCoordinates = m_rootViewToPluginTransform.inverse().mapRect(bounds);
    return frameView->contentsToScreen(enclosingIntRect(rectInRootViewCoordinates));
}

void PDFPlugin::geometryDidChange(const IntSize& pluginSize, const IntRect&, const AffineTransform& pluginToRootViewTransform)
{
    if (size() == pluginSize && pluginView()->pageScaleFactor() == [m_pdfLayerController.get() contentScaleFactor])
        return;

    setSize(pluginSize);
    m_rootViewToPluginTransform = pluginToRootViewTransform.inverse();
    [m_pdfLayerController.get() setFrameSize:pluginSize];

    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    CATransform3D transform = CATransform3DMakeScale(1, -1, 1);
    transform = CATransform3DTranslate(transform, 0, -pluginSize.height(), 0);
    
    if (handlesPageScaleFactor()) {
        CGFloat magnification = pluginView()->pageScaleFactor() - [m_pdfLayerController.get() contentScaleFactor];

        // FIXME: Instead of m_lastMousePositionInPluginCoordinates, we should use the zoom origin from PluginView::setPageScaleFactor.
        if (magnification)
            [m_pdfLayerController.get() magnifyWithMagnification:magnification atPoint:convertFromPluginToPDFView(m_lastMousePositionInPluginCoordinates) immediately:NO];
    } else {
        // If we don't handle page scale ourselves, we need to respect our parent page's
        // scale, which may have changed.
        updatePageAndDeviceScaleFactors();
    } 

    calculateSizes();
    updateScrollbars();

    if (m_activeAnnotation)
        m_activeAnnotation->updateGeometry();

    [m_contentLayer.get() setSublayerTransform:transform];
    [CATransaction commit];
}
    
static NSUInteger modifierFlagsFromWebEvent(const WebEvent& event)
{
    return (event.shiftKey() ? NSShiftKeyMask : 0)
        | (event.controlKey() ? NSControlKeyMask : 0)
        | (event.altKey() ? NSAlternateKeyMask : 0)
        | (event.metaKey() ? NSCommandKeyMask : 0);
}
    
static NSEventType eventTypeFromWebEvent(const WebEvent& event)
{
    switch (event.type()) {
    case WebEvent::KeyDown:
        return NSKeyDown;
    case WebEvent::KeyUp:
        return NSKeyUp;

    case WebEvent::MouseDown:
        switch (static_cast<const WebMouseEvent&>(event).button()) {
        case WebMouseEvent::LeftButton:
            return NSLeftMouseDown;
        case WebMouseEvent::RightButton:
            return NSRightMouseDown;
        default:
            return 0;
        }
        break;
    case WebEvent::MouseUp:
        switch (static_cast<const WebMouseEvent&>(event).button()) {
        case WebMouseEvent::LeftButton:
            return NSLeftMouseUp;
        case WebMouseEvent::RightButton:
            return NSRightMouseUp;
        default:
            return 0;
        }
        break;
    case WebEvent::MouseMove:
        switch (static_cast<const WebMouseEvent&>(event).button()) {
        case WebMouseEvent::LeftButton:
            return NSLeftMouseDragged;
        case WebMouseEvent::RightButton:
            return NSRightMouseDragged;
        case WebMouseEvent::NoButton:
            return NSMouseMoved;
        default:
            return 0;
        }
        break;

    default:
        return 0;
    }
}
    
NSEvent *PDFPlugin::nsEventForWebMouseEvent(const WebMouseEvent& event)
{
    m_lastMousePositionInPluginCoordinates = convertFromRootViewToPlugin(event.position());

    IntPoint positionInPDFViewCoordinates(convertFromPluginToPDFView(m_lastMousePositionInPluginCoordinates));

    NSEventType eventType = eventTypeFromWebEvent(event);

    if (!eventType)
        return 0;

    NSUInteger modifierFlags = modifierFlagsFromWebEvent(event);

    return [NSEvent mouseEventWithType:eventType location:positionInPDFViewCoordinates modifierFlags:modifierFlags timestamp:0 windowNumber:0 context:nil eventNumber:0 clickCount:event.clickCount() pressure:0];
}

void PDFPlugin::updateCursor(const WebMouseEvent& event, UpdateCursorMode mode)
{
    HitTestResult hitTestResult = None;

    PDFSelection *selectionUnderMouse = [m_pdfLayerController.get() getSelectionForWordAtPoint:convertFromPluginToPDFView(event.position())];
    if (selectionUnderMouse && [[selectionUnderMouse string] length])
        hitTestResult = Text;

    if (hitTestResult == m_lastHitTestResult && mode == UpdateIfNeeded)
        return;

    webFrame()->page()->send(Messages::WebPageProxy::SetCursor(hitTestResult == Text ? iBeamCursor() : pointerCursor()));
    m_lastHitTestResult = hitTestResult;
}

bool PDFPlugin::handleMouseEvent(const WebMouseEvent& event)
{
    PlatformMouseEvent platformEvent = platform(event);
    IntPoint mousePosition = convertFromRootViewToPlugin(event.position());

    m_lastMouseEvent = event;

    RefPtr<Scrollbar> targetScrollbar;
    RefPtr<Scrollbar> targetScrollbarForLastMousePosition;

    if (m_verticalScrollbarLayer) {
        IntRect verticalScrollbarFrame(m_verticalScrollbarLayer.get().frame);
        if (verticalScrollbarFrame.contains(mousePosition))
            targetScrollbar = verticalScrollbar();
        if (verticalScrollbarFrame.contains(m_lastMousePositionInPluginCoordinates))
            targetScrollbarForLastMousePosition = verticalScrollbar();
    }

    if (m_horizontalScrollbarLayer) {
        IntRect horizontalScrollbarFrame(m_horizontalScrollbarLayer.get().frame);
        if (horizontalScrollbarFrame.contains(mousePosition))
            targetScrollbar = horizontalScrollbar();
        if (horizontalScrollbarFrame.contains(m_lastMousePositionInPluginCoordinates))
            targetScrollbarForLastMousePosition = horizontalScrollbar();
    }

    if (m_scrollCornerLayer && IntRect(m_scrollCornerLayer.get().frame).contains(mousePosition))
        return false;

    if ([pdfDocument() isLocked])
        return false;

    // Right-clicks and Control-clicks always call handleContextMenuEvent as well.
    if (event.button() == WebMouseEvent::RightButton || (event.button() == WebMouseEvent::LeftButton && event.controlKey()))
        return true;

    NSEvent *nsEvent = nsEventForWebMouseEvent(event);

    switch (event.type()) {
    case WebEvent::MouseMove:
        mouseMovedInContentArea();
        updateCursor(event);

        if (targetScrollbar) {
            if (!targetScrollbarForLastMousePosition) {
                targetScrollbar->mouseEntered();
                return true;
            }
            return targetScrollbar->mouseMoved(platformEvent);
        }

        if (!targetScrollbar && targetScrollbarForLastMousePosition)
            targetScrollbarForLastMousePosition->mouseExited();

        switch (event.button()) {
        case WebMouseEvent::LeftButton:
            [m_pdfLayerController.get() mouseDragged:nsEvent];
            return true;
        case WebMouseEvent::RightButton:
        case WebMouseEvent::MiddleButton:
            return false;
        case WebMouseEvent::NoButton:
            [m_pdfLayerController.get() mouseMoved:nsEvent];
            return true;
        }
    case WebEvent::MouseDown:
        switch (event.button()) {
        case WebMouseEvent::LeftButton:
            if (targetScrollbar)
                return targetScrollbar->mouseDown(platformEvent);

            [m_pdfLayerController.get() mouseDown:nsEvent];
            return true;
        case WebMouseEvent::RightButton:
            [m_pdfLayerController.get() rightMouseDown:nsEvent];
            return true;
        case WebMouseEvent::MiddleButton:
        case WebMouseEvent::NoButton:
            return false;
        }
    case WebEvent::MouseUp:
        switch (event.button()) {
        case WebMouseEvent::LeftButton:
            if (targetScrollbar)
                return targetScrollbar->mouseUp(platformEvent);

            [m_pdfLayerController.get() mouseUp:nsEvent];
            return true;
        case WebMouseEvent::RightButton:
        case WebMouseEvent::MiddleButton:
        case WebMouseEvent::NoButton:
            return false;
        }
    default:
        break;
    }

    return false;
}

bool PDFPlugin::handleMouseEnterEvent(const WebMouseEvent& event)
{
    mouseEnteredContentArea();
    updateCursor(event, ForceUpdate);
    return false;
}

bool PDFPlugin::handleMouseLeaveEvent(const WebMouseEvent&)
{
    mouseExitedContentArea();
    return false;
}
    
bool PDFPlugin::showContextMenuAtPoint(const IntPoint& point)
{
    FrameView* frameView = webFrame()->coreFrame()->view();
    IntPoint contentsPoint = frameView->contentsToRootView(point);
    WebMouseEvent event(WebEvent::MouseDown, WebMouseEvent::RightButton, contentsPoint, contentsPoint, 0, 0, 0, 1, static_cast<WebEvent::Modifiers>(0), monotonicallyIncreasingTime());
    return handleContextMenuEvent(event);
}

bool PDFPlugin::handleContextMenuEvent(const WebMouseEvent& event)
{
    FrameView* frameView = webFrame()->coreFrame()->view();
    IntPoint point = frameView->contentsToScreen(IntRect(frameView->windowToContents(event.position()), IntSize())).location();
    
    if (NSMenu *nsMenu = [m_pdfLayerController.get() menuForEvent:nsEventForWebMouseEvent(event)]) {
        WKPopupContextMenu(nsMenu, point);
        return true;
    }
    
    return false;
}

bool PDFPlugin::handleKeyboardEvent(const WebKeyboardEvent& event)
{
    NSEventType eventType = eventTypeFromWebEvent(event);
    NSUInteger modifierFlags = modifierFlagsFromWebEvent(event);
    
    NSEvent *fakeEvent = [NSEvent keyEventWithType:eventType location:NSZeroPoint modifierFlags:modifierFlags timestamp:0 windowNumber:0 context:0 characters:event.text() charactersIgnoringModifiers:event.unmodifiedText() isARepeat:event.isAutoRepeat() keyCode:event.nativeVirtualKeyCode()];
    
    switch (event.type()) {
    case WebEvent::KeyDown:
        return [m_pdfLayerController.get() keyDown:fakeEvent];
    default:
        return false;
    }
    
    return false;
}
    
bool PDFPlugin::handleEditingCommand(const String& commandName, const String& argument)
{
    if (commandName == "copy")
        [m_pdfLayerController.get() copySelection];
    else if (commandName == "selectAll")
        [m_pdfLayerController.get() selectAll];
    
    return true;
}

bool PDFPlugin::isEditingCommandEnabled(const String& commandName)
{
    if (commandName == "copy")
        return [m_pdfLayerController.get() currentSelection];
        
    if (commandName == "selectAll")
        return true;

    return false;
}

void PDFPlugin::setScrollOffset(const IntPoint& offset)
{
    m_scrollOffset = IntSize(offset.x(), offset.y());

    [CATransaction begin];
    [m_pdfLayerController.get() setScrollPosition:offset];

    if (m_activeAnnotation)
        m_activeAnnotation->updateGeometry();

    [CATransaction commit];
}

void PDFPlugin::invalidateScrollbarRect(Scrollbar* scrollbar, const IntRect& rect)
{
    if (scrollbar == horizontalScrollbar())
        [m_horizontalScrollbarLayer.get() setNeedsDisplay];
    else if (scrollbar == verticalScrollbar())
        [m_verticalScrollbarLayer.get() setNeedsDisplay];
}

void PDFPlugin::invalidateScrollCornerRect(const IntRect& rect)
{
    [m_scrollCornerLayer.get() setNeedsDisplay];
}

bool PDFPlugin::isFullFramePlugin()
{
    // <object> or <embed> plugins will appear to be in their parent frame, so we have to
    // check whether our frame's widget is exactly our PluginView.
    Document* document = webFrame()->coreFrame()->document();
    return document->isPluginDocument() && static_cast<PluginDocument*>(document)->pluginWidget() == pluginView();
}

bool PDFPlugin::handlesPageScaleFactor()
{
    return webFrame()->isMainFrame() && isFullFramePlugin();
}

void PDFPlugin::clickedLink(NSURL *url)
{
    Frame* frame = webFrame()->coreFrame();

    RefPtr<Event> coreEvent;
    if (m_lastMouseEvent.type() != WebEvent::NoType)
        coreEvent = MouseEvent::create(eventNames().clickEvent, frame->document()->defaultView(), platform(m_lastMouseEvent), 0, 0);

    frame->loader()->urlSelected(url, emptyString(), coreEvent.get(), false, false, MaybeSendReferrer);
}

void PDFPlugin::setActiveAnnotation(PDFAnnotation *annotation)
{
    if (!supportsForms())
        return;

    if (m_activeAnnotation)
        m_activeAnnotation->commit();

    if (annotation) {
        if ([annotation isKindOfClass:pdfAnnotationTextWidgetClass()] && static_cast<PDFAnnotationTextWidget *>(annotation).isReadOnly) {
            m_activeAnnotation = 0;
            return;
        }

        m_activeAnnotation = PDFPluginAnnotation::create(annotation, m_pdfLayerController.get(), this);
        m_activeAnnotation->attach(m_annotationContainer.get());
    } else
        m_activeAnnotation = 0;
}

bool PDFPlugin::supportsForms()
{
    // FIXME: We support forms for full-main-frame and <iframe> PDFs, but not <embed> or <object>, because those cases do not have their own Document into which to inject form elements.
    return isFullFramePlugin();
}

void PDFPlugin::notifyContentScaleFactorChanged(CGFloat scaleFactor)
{
    if (handlesPageScaleFactor())
        pluginView()->setPageScaleFactor(scaleFactor, IntPoint());

    calculateSizes();
    updateScrollbars();
}

void PDFPlugin::notifyDisplayModeChanged(int)
{
    calculateSizes();
    updateScrollbars();
}

void PDFPlugin::saveToPDF()
{
    // FIXME: We should probably notify the user that they can't save before the document is finished loading.
    // PDFViewController does an NSBeep(), but that seems insufficient.
    if (!pdfDocument())
        return;

    NSData *data = liveData();
    webFrame()->page()->savePDFToFileInDownloadsFolder(suggestedFilename(), webFrame()->url(), static_cast<const unsigned char *>([data bytes]), [data length]);
}

void PDFPlugin::openWithNativeApplication()
{
    if (!m_temporaryPDFUUID) {
        // FIXME: We should probably notify the user that they can't save before the document is finished loading.
        // PDFViewController does an NSBeep(), but that seems insufficient.
        if (!pdfDocument())
            return;

        NSData *data = liveData();

        m_temporaryPDFUUID = WebCore::createCanonicalUUIDString();
        ASSERT(m_temporaryPDFUUID);

        webFrame()->page()->savePDFToTemporaryFolderAndOpenWithNativeApplication(suggestedFilename(), webFrame()->url(), static_cast<const unsigned char *>([data bytes]), [data length], m_temporaryPDFUUID);
        return;
    }

    webFrame()->page()->send(Messages::WebPageProxy::OpenPDFFromTemporaryFolderWithNativeApplication(m_temporaryPDFUUID));
}

void PDFPlugin::writeItemsToPasteboard(NSArray *items, NSArray *types)
{
    Vector<String> pasteboardTypes;

    for (NSString *type in types)
        pasteboardTypes.append(type);

    WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::SetPasteboardTypes(NSGeneralPboard, pasteboardTypes), 0);

    for (NSUInteger i = 0, count = items.count; i < count; ++i) {
        NSString *type = [types objectAtIndex:i];
        NSData *data = [items objectAtIndex:i];

        // We don't expect the data for any items to be empty, but aren't completely sure.
        // Avoid crashing in the SharedMemory constructor in release builds if we're wrong.
        ASSERT(data.length);
        if (!data.length)
            continue;

        if ([type isEqualToString:NSStringPboardType] || [type isEqualToString:NSPasteboardTypeString]) {
            RetainPtr<NSString> plainTextString = adoptNS([[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]);
            WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::SetPasteboardStringForType(NSGeneralPboard, type, plainTextString.get()), 0);
        } else {
            RefPtr<SharedBuffer> buffer = SharedBuffer::wrapNSData(data);

            if (!buffer)
                continue;

            SharedMemory::Handle handle;
            RefPtr<SharedMemory> sharedMemory = SharedMemory::create(buffer->size());
            memcpy(sharedMemory->data(), buffer->data(), buffer->size());
            sharedMemory->createHandle(handle, SharedMemory::ReadOnly);
            WebProcess::shared().parentProcessConnection()->send(Messages::WebContext::SetPasteboardBufferForType(NSGeneralPboard, type, handle, buffer->size()), 0);
        }
    }
}

void PDFPlugin::showDefinitionForAttributedString(NSAttributedString *string, CGPoint point)
{
    DictionaryPopupInfo dictionaryPopupInfo;
    dictionaryPopupInfo.origin = convertFromPDFViewToRootView(IntPoint(point));

    AttributedString attributedString;
    attributedString.string = string;

    webFrame()->page()->send(Messages::WebPageProxy::DidPerformDictionaryLookup(attributedString, dictionaryPopupInfo));
}

unsigned PDFPlugin::countFindMatches(const String& target, WebCore::FindOptions options, unsigned maxMatchCount)
{
    if (!target.length())
        return 0;

    int nsOptions = (options & FindOptionsCaseInsensitive) ? NSCaseInsensitiveSearch : 0;

    return [[pdfDocument().get() findString:target withOptions:nsOptions] count];
}

PDFSelection *PDFPlugin::nextMatchForString(const String& target, BOOL searchForward, BOOL caseSensitive, BOOL wrapSearch, PDFSelection *initialSelection, BOOL startInSelection)
{
    if (!target.length())
        return nil;

    NSStringCompareOptions options = 0;
    if (!searchForward)
        options |= NSBackwardsSearch;

    if (!caseSensitive)
        options |= NSCaseInsensitiveSearch;

    PDFDocument *document = pdfDocument().get();

    PDFSelection *selectionForInitialSearch = [initialSelection copy];
    if (startInSelection) {
        // Initially we want to include the selected text in the search.  So we must modify the starting search
        // selection to fit PDFDocument's search requirements: selection must have a length >= 1, begin before
        // the current selection (if searching forwards) or after (if searching backwards).
        int initialSelectionLength = [[initialSelection string] length];
        if (searchForward) {
            [selectionForInitialSearch extendSelectionAtStart:1];
            [selectionForInitialSearch extendSelectionAtEnd:-initialSelectionLength];
        } else {
            [selectionForInitialSearch extendSelectionAtEnd:1];
            [selectionForInitialSearch extendSelectionAtStart:-initialSelectionLength];
        }
    }

    PDFSelection *foundSelection = [document findString:target fromSelection:selectionForInitialSearch withOptions:options];
    [selectionForInitialSearch release];

    // If we first searched in the selection, and we found the selection, search again from just past the selection.
    if (startInSelection && [foundSelection isEqual:initialSelection])
        foundSelection = [document findString:target fromSelection:initialSelection withOptions:options];
        
    if (!foundSelection && wrapSearch)
        foundSelection = [document findString:target fromSelection:nil withOptions:options];
        
    return foundSelection;
}

bool PDFPlugin::findString(const String& target, WebCore::FindOptions options, unsigned maxMatchCount)
{
    BOOL searchForward = !(options & FindOptionsBackwards);
    BOOL caseSensitive = !(options & FindOptionsCaseInsensitive);
    BOOL wrapSearch = options & FindOptionsWrapAround;

    unsigned matchCount;
    if (!maxMatchCount) {
        // If the max was zero, any result means we exceeded the max. We can skip computing the actual count.
        matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
    } else {
        matchCount = countFindMatches(target, options, maxMatchCount);
        if (matchCount > maxMatchCount)
            matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
    }

    if (target.isEmpty()) {
        PDFSelection* searchSelection = [m_pdfLayerController.get() searchSelection];
        [m_pdfLayerController.get() findString:target caseSensitive:caseSensitive highlightMatches:YES];
        [m_pdfLayerController.get() setSearchSelection:searchSelection];
        m_lastFoundString = emptyString();
        return false;
    }

    if (m_lastFoundString == target) {
        PDFSelection *selection = nextMatchForString(target, searchForward, caseSensitive, wrapSearch, [m_pdfLayerController.get() searchSelection], NO);
        if (!selection)
            return false;

        [m_pdfLayerController.get() setSearchSelection:selection];
        [m_pdfLayerController.get() gotoSelection:selection];
    } else {
        [m_pdfLayerController.get() findString:target caseSensitive:caseSensitive highlightMatches:YES];
        m_lastFoundString = target;
    }

    return matchCount > 0;
}

bool PDFPlugin::performDictionaryLookupAtLocation(const WebCore::FloatPoint& point)
{
    PDFSelection* lookupSelection = [m_pdfLayerController.get() getSelectionForWordAtPoint:convertFromPluginToPDFView(roundedIntPoint(point))];

    if ([[lookupSelection string] length])
        [m_pdfLayerController.get() searchInDictionaryWithSelection:lookupSelection];

    return true;
}

void PDFPlugin::focusNextAnnotation()
{
    [m_pdfLayerController.get() activateNextAnnotation:false];
}

void PDFPlugin::focusPreviousAnnotation()
{
    [m_pdfLayerController.get() activateNextAnnotation:true];
}

void PDFPlugin::notifySelectionChanged(PDFSelection *)
{
    webFrame()->page()->didChangeSelection();
}

String PDFPlugin::getSelectionString() const
{
    return [[m_pdfLayerController.get() currentSelection] string];
}

void PDFPlugin::performWebSearch(NSString *string)
{
    webFrame()->page()->send(Messages::WebPageProxy::SearchTheWeb(string));
}

void PDFPlugin::performSpotlightSearch(NSString *string)
{
    webFrame()->page()->send(Messages::WebPageProxy::SearchWithSpotlight(string));
}

bool PDFPlugin::handleWheelEvent(const WebWheelEvent& event)
{
    PDFDisplayMode displayMode = [m_pdfLayerController.get() displayMode];

    if (displayMode == kPDFDisplaySinglePageContinuous || displayMode == kPDFDisplayTwoUpContinuous)
        return SimplePDFPlugin::handleWheelEvent(event);

    NSUInteger currentPageIndex = [m_pdfLayerController.get() currentPageIndex];
    bool inFirstPage = currentPageIndex == 0;
    bool inLastPage = [m_pdfLayerController.get() lastPageIndex] == currentPageIndex;

    bool atScrollTop = scrollPosition().y() == 0;
    bool atScrollBottom = scrollPosition().y() == maximumScrollPosition().y();

    bool inMomentumScroll = event.momentumPhase() != WebWheelEvent::PhaseNone;

    int scrollMagnitudeThresholdForPageFlip = defaultScrollMagnitudeThresholdForPageFlip;

    // Imprecise input devices should have a lower threshold so that "clicky" scroll wheels can flip pages.
    if (!event.hasPreciseScrollingDeltas())
        scrollMagnitudeThresholdForPageFlip = 0;

    if (atScrollBottom && !inLastPage && event.delta().height() < 0) {
        if (event.delta().height() <= -scrollMagnitudeThresholdForPageFlip && !inMomentumScroll)
            [m_pdfLayerController.get() gotoNextPage];
        return true;
    } else if (atScrollTop && !inFirstPage && event.delta().height() > 0) {
        if (event.delta().height() >= scrollMagnitudeThresholdForPageFlip && !inMomentumScroll) {
            [CATransaction begin];
            [m_pdfLayerController.get() gotoPreviousPage];
            scrollToOffsetWithoutAnimation(maximumScrollPosition());
            [CATransaction commit];
        }
        return true;
    }

    return SimplePDFPlugin::handleWheelEvent(event);
}

NSData *PDFPlugin::liveData() const
{
    if (m_activeAnnotation)
        m_activeAnnotation->commit();

    return SimplePDFPlugin::liveData();
}

NSObject *PDFPlugin::accessibilityObject() const
{
    return m_accessibilityObject.get();
}

} // namespace WebKit

#endif // ENABLE(PDFKIT_PLUGIN)
