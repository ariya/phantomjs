/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef PDFPlugin_h
#define PDFPlugin_h

#if ENABLE(PDFKIT_PLUGIN)

#include "Plugin.h"
#include "SimplePDFPlugin.h"
#include "WebEvent.h"
#include <WebCore/AffineTransform.h>
#include <WebCore/FindOptions.h>
#include <WebCore/ScrollableArea.h>
#include <wtf/RetainPtr.h>

typedef const struct OpaqueJSContext* JSContextRef;
typedef struct OpaqueJSValue* JSObjectRef;
typedef const struct OpaqueJSValue* JSValueRef;

OBJC_CLASS NSArray;
OBJC_CLASS PDFAnnotation;
OBJC_CLASS PDFLayerController;
OBJC_CLASS PDFSelection;
OBJC_CLASS WKPDFPluginAccessibilityObject;
OBJC_CLASS WKPDFLayerControllerDelegate;

namespace CoreIPC {
class DataReference;
}

namespace WebCore {
class Element;
struct PluginInfo;
}

namespace WebKit {

class PDFPluginAnnotation;
class PDFPluginPasswordField;
class PluginView;
class WebFrame;

class PDFPlugin : public SimplePDFPlugin {
public:
    static PassRefPtr<PDFPlugin> create(WebFrame*);
    ~PDFPlugin();

    void paintControlForLayerInContext(CALayer *, CGContextRef);
    void setActiveAnnotation(PDFAnnotation *);
    
    using ScrollableArea::notifyScrollPositionChanged;
    void notifyContentScaleFactorChanged(CGFloat scaleFactor);
    void notifyDisplayModeChanged(int);

    void notifySelectionChanged(PDFSelection *);

    void clickedLink(NSURL *);
    void saveToPDF();
    void openWithNativeApplication();
    void writeItemsToPasteboard(NSArray *items, NSArray *types);
    void showDefinitionForAttributedString(NSAttributedString *, CGPoint);
    void performWebSearch(NSString *);
    void performSpotlightSearch(NSString *);

    void focusNextAnnotation();
    void focusPreviousAnnotation();

    void attemptToUnlockPDF(const String& password);

    WebCore::FloatRect convertFromPDFViewToScreen(const WebCore::FloatRect&) const;
    WebCore::IntRect boundsOnScreen() const;
    
    bool showContextMenuAtPoint(const WebCore::IntPoint&);

private:
    explicit PDFPlugin(WebFrame*);

    virtual void updateScrollbars() OVERRIDE;
    virtual PassRefPtr<WebCore::Scrollbar> createScrollbar(WebCore::ScrollbarOrientation) OVERRIDE;
    virtual void destroyScrollbar(WebCore::ScrollbarOrientation) OVERRIDE;
    virtual void pdfDocumentDidLoad() OVERRIDE;
    virtual void calculateSizes() OVERRIDE;

    virtual void destroy() OVERRIDE;
    virtual void paint(WebCore::GraphicsContext*, const WebCore::IntRect& dirtyRectInWindowCoordinates) OVERRIDE;
    virtual PassRefPtr<ShareableBitmap> snapshot() OVERRIDE;
    virtual PlatformLayer* pluginLayer() OVERRIDE;
    virtual void geometryDidChange(const WebCore::IntSize& pluginSize, const WebCore::IntRect& clipRect, const WebCore::AffineTransform& pluginToRootViewTransform) OVERRIDE;
    virtual void contentsScaleFactorChanged(float) OVERRIDE;
    virtual bool handleMouseEvent(const WebMouseEvent&) OVERRIDE;
    virtual bool handleMouseEnterEvent(const WebMouseEvent&) OVERRIDE;
    virtual bool handleMouseLeaveEvent(const WebMouseEvent&) OVERRIDE;
    virtual bool handleContextMenuEvent(const WebMouseEvent&) OVERRIDE;
    virtual bool handleKeyboardEvent(const WebKeyboardEvent&) OVERRIDE;
    virtual bool handleEditingCommand(const String& commandName, const String& argument) OVERRIDE;
    virtual bool isEditingCommandEnabled(const String&) OVERRIDE;
    virtual bool handlesPageScaleFactor() OVERRIDE;

    virtual unsigned countFindMatches(const String& target, WebCore::FindOptions, unsigned maxMatchCount) OVERRIDE;
    virtual bool findString(const String& target, WebCore::FindOptions, unsigned maxMatchCount) OVERRIDE;

    PDFSelection *nextMatchForString(const String& target, BOOL searchForward, BOOL caseSensitive, BOOL wrapSearch, PDFSelection *initialSelection, BOOL startInSelection);

    virtual bool performDictionaryLookupAtLocation(const WebCore::FloatPoint&) OVERRIDE;
    virtual String getSelectionString() const OVERRIDE;

    virtual bool handleWheelEvent(const WebWheelEvent& event) OVERRIDE;

    // ScrollableArea functions.
    virtual void setScrollOffset(const WebCore::IntPoint&) OVERRIDE;
    virtual void invalidateScrollbarRect(WebCore::Scrollbar*, const WebCore::IntRect&) OVERRIDE;
    virtual void invalidateScrollCornerRect(const WebCore::IntRect&) OVERRIDE;
    virtual WebCore::IntPoint lastKnownMousePosition() const OVERRIDE { return m_lastMousePositionInPluginCoordinates; }
    
    NSEvent *nsEventForWebMouseEvent(const WebMouseEvent&);
    WebCore::IntPoint convertFromPluginToPDFView(const WebCore::IntPoint&) const;
    WebCore::IntPoint convertFromRootViewToPlugin(const WebCore::IntPoint&) const;
    WebCore::IntPoint convertFromPDFViewToRootView(const WebCore::IntPoint&) const;
    
    bool supportsForms();
    bool isFullFramePlugin();

    void updatePageAndDeviceScaleFactors();

    void createPasswordEntryForm();

    virtual NSData *liveData() const OVERRIDE;

    enum UpdateCursorMode {
        UpdateIfNeeded,
        ForceUpdate
    };

    enum HitTestResult {
        None,
        Text
    };

    void updateCursor(const WebMouseEvent&, UpdateCursorMode = UpdateIfNeeded);

    virtual NSObject *accessibilityObject() const OVERRIDE;

    RetainPtr<CALayer> m_containerLayer;
    RetainPtr<CALayer> m_contentLayer;
    RetainPtr<CALayer> m_horizontalScrollbarLayer;
    RetainPtr<CALayer> m_verticalScrollbarLayer;
    RetainPtr<CALayer> m_scrollCornerLayer;
    RetainPtr<PDFLayerController> m_pdfLayerController;
    RetainPtr<WKPDFPluginAccessibilityObject> m_accessibilityObject;
    
    RefPtr<PDFPluginAnnotation> m_activeAnnotation;
    RefPtr<PDFPluginPasswordField> m_passwordField;
    RefPtr<WebCore::Element> m_annotationContainer;

    WebCore::AffineTransform m_rootViewToPluginTransform;
    WebMouseEvent m_lastMouseEvent;
    WebCore::IntPoint m_lastMousePositionInPluginCoordinates;

    String m_temporaryPDFUUID;

    String m_lastFoundString;

    HitTestResult m_lastHitTestResult;
    
    RetainPtr<WKPDFLayerControllerDelegate> m_pdfLayerControllerDelegate;
};

} // namespace WebKit

#endif // ENABLE(PDFKIT_PLUGIN)

#endif // PDFPlugin_h
