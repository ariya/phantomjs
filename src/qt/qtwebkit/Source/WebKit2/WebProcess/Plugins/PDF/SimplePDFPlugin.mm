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
#import "SimplePDFPlugin.h"

#import "PDFKitImports.h"
#import "PluginView.h"
#import "ShareableBitmap.h"
#import "WebEvent.h"
#import "WebEventConversion.h"
#import <JavaScriptCore/JSContextRef.h>
#import <JavaScriptCore/JSObjectRef.h>
#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <PDFKit/PDFKit.h>
#import <WebCore/ArchiveResource.h>
#import <WebCore/Chrome.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/FocusController.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/HTTPHeaderMap.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/Page.h>
#import <WebCore/PluginData.h>
#import <WebCore/RenderBoxModelObject.h>
#import <WebCore/ScrollAnimator.h>
#import <WebCore/ScrollbarTheme.h>

using namespace WebCore;

static const char* postScriptMIMEType = "application/postscript";

static void appendValuesInPDFNameSubtreeToVector(CGPDFDictionaryRef subtree, Vector<CGPDFObjectRef>& values)
{
    CGPDFArrayRef names;
    if (CGPDFDictionaryGetArray(subtree, "Names", &names)) {
        size_t nameCount = CGPDFArrayGetCount(names) / 2;
        for (size_t i = 0; i < nameCount; ++i) {
            CGPDFObjectRef object;
            CGPDFArrayGetObject(names, 2 * i + 1, &object);
            values.append(object);
        }
        return;
    }

    CGPDFArrayRef kids;
    if (!CGPDFDictionaryGetArray(subtree, "Kids", &kids))
        return;

    size_t kidCount = CGPDFArrayGetCount(kids);
    for (size_t i = 0; i < kidCount; ++i) {
        CGPDFDictionaryRef kid;
        if (!CGPDFArrayGetDictionary(kids, i, &kid))
            continue;
        appendValuesInPDFNameSubtreeToVector(kid, values);
    }
}

static void getAllValuesInPDFNameTree(CGPDFDictionaryRef tree, Vector<CGPDFObjectRef>& allValues)
{
    appendValuesInPDFNameSubtreeToVector(tree, allValues);
}

static void getAllScriptsInPDFDocument(CGPDFDocumentRef pdfDocument, Vector<RetainPtr<CFStringRef>>& scripts)
{
    if (!pdfDocument)
        return;

    CGPDFDictionaryRef pdfCatalog = CGPDFDocumentGetCatalog(pdfDocument);
    if (!pdfCatalog)
        return;

    // Get the dictionary of all document-level name trees.
    CGPDFDictionaryRef namesDictionary;
    if (!CGPDFDictionaryGetDictionary(pdfCatalog, "Names", &namesDictionary))
        return;

    // Get the document-level "JavaScript" name tree.
    CGPDFDictionaryRef javaScriptNameTree;
    if (!CGPDFDictionaryGetDictionary(namesDictionary, "JavaScript", &javaScriptNameTree))
        return;

    // The names are arbitrary. We are only interested in the values.
    Vector<CGPDFObjectRef> objects;
    getAllValuesInPDFNameTree(javaScriptNameTree, objects);
    size_t objectCount = objects.size();

    for (size_t i = 0; i < objectCount; ++i) {
        CGPDFDictionaryRef javaScriptAction;
        if (!CGPDFObjectGetValue(reinterpret_cast<CGPDFObjectRef>(objects[i]), kCGPDFObjectTypeDictionary, &javaScriptAction))
            continue;

        // A JavaScript action must have an action type of "JavaScript".
        const char* actionType;
        if (!CGPDFDictionaryGetName(javaScriptAction, "S", &actionType) || strcmp(actionType, "JavaScript"))
            continue;

        const UInt8* bytes = 0;
        CFIndex length;
        CGPDFStreamRef stream;
        CGPDFStringRef string;
        RetainPtr<CFDataRef> data;
        if (CGPDFDictionaryGetStream(javaScriptAction, "JS", &stream)) {
            CGPDFDataFormat format;
            data = adoptCF(CGPDFStreamCopyData(stream, &format));
            if (!data)
                continue;
            bytes = CFDataGetBytePtr(data.get());
            length = CFDataGetLength(data.get());
        } else if (CGPDFDictionaryGetString(javaScriptAction, "JS", &string)) {
            bytes = CGPDFStringGetBytePtr(string);
            length = CGPDFStringGetLength(string);
        }
        if (!bytes)
            continue;

        CFStringEncoding encoding = (length > 1 && bytes[0] == 0xFE && bytes[1] == 0xFF) ? kCFStringEncodingUnicode : kCFStringEncodingUTF8;
        RetainPtr<CFStringRef> script = adoptCF(CFStringCreateWithBytes(kCFAllocatorDefault, bytes, length, encoding, true));
        if (!script)
            continue;

        scripts.append(script);
    }
}

namespace WebKit {

const uint64_t pdfDocumentRequestID = 1; // PluginController supports loading multiple streams, but we only need one for PDF.

const int gutterHeight = 10;
const int shadowOffsetX = 0;
const int shadowOffsetY = -2;
const int shadowSize = 7;

PassRefPtr<SimplePDFPlugin> SimplePDFPlugin::create(WebFrame* frame)
{
    return adoptRef(new SimplePDFPlugin(frame));
}

SimplePDFPlugin::SimplePDFPlugin(WebFrame* frame)
    : m_frame(frame)
    , m_isPostScript(false)
    , m_pdfDocumentWasMutated(false)
{
}

SimplePDFPlugin::~SimplePDFPlugin()
{
}

PluginInfo SimplePDFPlugin::pluginInfo()
{
    PluginInfo info;
    info.name = builtInPDFPluginName();
    info.isApplicationPlugin = true;

    MimeClassInfo pdfMimeClassInfo;
    pdfMimeClassInfo.type = "application/pdf";
    pdfMimeClassInfo.desc = pdfDocumentTypeDescription();
    pdfMimeClassInfo.extensions.append("pdf");
    info.mimes.append(pdfMimeClassInfo);
    
    MimeClassInfo textPDFMimeClassInfo;
    textPDFMimeClassInfo.type = "text/pdf";
    textPDFMimeClassInfo.desc = pdfDocumentTypeDescription();
    textPDFMimeClassInfo.extensions.append("pdf");
    info.mimes.append(textPDFMimeClassInfo);

    MimeClassInfo postScriptMimeClassInfo;
    postScriptMimeClassInfo.type = postScriptMIMEType;
    postScriptMimeClassInfo.desc = postScriptDocumentTypeDescription();
    postScriptMimeClassInfo.extensions.append("ps");
    info.mimes.append(postScriptMimeClassInfo);

    return info;
}

PluginView* SimplePDFPlugin::pluginView()
{
    return static_cast<PluginView*>(controller());
}

const PluginView* SimplePDFPlugin::pluginView() const
{
    return static_cast<const PluginView*>(controller());
}

void SimplePDFPlugin::updateScrollbars()
{
    bool hadScrollbars = m_horizontalScrollbar || m_verticalScrollbar;

    if (m_horizontalScrollbar) {
        if (m_size.width() >= m_pdfDocumentSize.width())
            destroyScrollbar(HorizontalScrollbar);
    } else if (m_size.width() < m_pdfDocumentSize.width())
        m_horizontalScrollbar = createScrollbar(HorizontalScrollbar);

    if (m_verticalScrollbar) {
        if (m_size.height() >= m_pdfDocumentSize.height())
            destroyScrollbar(VerticalScrollbar);
    } else if (m_size.height() < m_pdfDocumentSize.height())
        m_verticalScrollbar = createScrollbar(VerticalScrollbar);

    int horizontalScrollbarHeight = (m_horizontalScrollbar && !m_horizontalScrollbar->isOverlayScrollbar()) ? m_horizontalScrollbar->height() : 0;
    int verticalScrollbarWidth = (m_verticalScrollbar && !m_verticalScrollbar->isOverlayScrollbar()) ? m_verticalScrollbar->width() : 0;

    int pageStep = m_pageBoxes.isEmpty() ? 0 : m_pageBoxes[0].height();

    if (m_horizontalScrollbar) {
        m_horizontalScrollbar->setSteps(Scrollbar::pixelsPerLineStep(), pageStep);
        m_horizontalScrollbar->setProportion(m_size.width() - verticalScrollbarWidth, m_pdfDocumentSize.width());
        IntRect scrollbarRect(pluginView()->x(), pluginView()->y() + m_size.height() - m_horizontalScrollbar->height(), m_size.width(), m_horizontalScrollbar->height());
        if (m_verticalScrollbar)
            scrollbarRect.contract(m_verticalScrollbar->width(), 0);
        m_horizontalScrollbar->setFrameRect(scrollbarRect);
    }
    if (m_verticalScrollbar) {
        m_verticalScrollbar->setSteps(Scrollbar::pixelsPerLineStep(), pageStep);
        m_verticalScrollbar->setProportion(m_size.height() - horizontalScrollbarHeight, m_pdfDocumentSize.height());
        IntRect scrollbarRect(IntRect(pluginView()->x() + m_size.width() - m_verticalScrollbar->width(), pluginView()->y(), m_verticalScrollbar->width(), m_size.height()));
        if (m_horizontalScrollbar)
            scrollbarRect.contract(0, m_horizontalScrollbar->height());
        m_verticalScrollbar->setFrameRect(scrollbarRect);
    }
    
    FrameView* frameView = m_frame->coreFrame()->view();
    if (!frameView)
        return;

    bool hasScrollbars = m_horizontalScrollbar || m_verticalScrollbar;
    if (hadScrollbars != hasScrollbars) {
        if (hasScrollbars)
            frameView->addScrollableArea(this);
        else
            frameView->removeScrollableArea(this);

        frameView->setNeedsLayout();
    }
}

PassRefPtr<Scrollbar> SimplePDFPlugin::createScrollbar(ScrollbarOrientation orientation)
{
    RefPtr<Scrollbar> widget = Scrollbar::createNativeScrollbar(this, orientation, RegularScrollbar);
    didAddScrollbar(widget.get(), orientation);
    pluginView()->frame()->view()->addChild(widget.get());
    return widget.release();
}

void SimplePDFPlugin::destroyScrollbar(ScrollbarOrientation orientation)
{
    RefPtr<Scrollbar>& scrollbar = orientation == HorizontalScrollbar ? m_horizontalScrollbar : m_verticalScrollbar;
    if (!scrollbar)
        return;

    willRemoveScrollbar(scrollbar.get(), orientation);
    scrollbar->removeFromParent();
    scrollbar->disconnectFromScrollableArea();
    scrollbar = 0;
}

void SimplePDFPlugin::addArchiveResource()
{
    // FIXME: It's a hack to force add a resource to DocumentLoader. PDF documents should just be fetched as CachedResources.

    // Add just enough data for context menu handling and web archives to work.
    ResourceResponse synthesizedResponse;
    synthesizedResponse.setSuggestedFilename(m_suggestedFilename);
    synthesizedResponse.setURL(m_sourceURL); // Needs to match the HitTestResult::absolutePDFURL.
    synthesizedResponse.setMimeType("application/pdf");

    RefPtr<ArchiveResource> resource = ArchiveResource::create(SharedBuffer::wrapCFData(m_data.get()), m_sourceURL, "application/pdf", String(), String(), synthesizedResponse);
    pluginView()->frame()->document()->loader()->addArchiveResource(resource.release());
}

static void jsPDFDocInitialize(JSContextRef ctx, JSObjectRef object)
{
    SimplePDFPlugin* pdfView = static_cast<SimplePDFPlugin*>(JSObjectGetPrivate(object));
    pdfView->ref();
}

static void jsPDFDocFinalize(JSObjectRef object)
{
    SimplePDFPlugin* pdfView = static_cast<SimplePDFPlugin*>(JSObjectGetPrivate(object));
    pdfView->deref();
}

JSValueRef SimplePDFPlugin::jsPDFDocPrint(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    SimplePDFPlugin* pdfView = static_cast<SimplePDFPlugin*>(JSObjectGetPrivate(thisObject));
    
    WebFrame* frame = pdfView->m_frame;
    if (!frame)
        return JSValueMakeUndefined(ctx);
    
    Frame* coreFrame = frame->coreFrame();
    if (!coreFrame)
        return JSValueMakeUndefined(ctx);
    
    Page* page = coreFrame->page();
    if (!page)
        return JSValueMakeUndefined(ctx);
    
    page->chrome().print(coreFrame);
    
    return JSValueMakeUndefined(ctx);
}

JSObjectRef SimplePDFPlugin::makeJSPDFDoc(JSContextRef ctx)
{
    static JSStaticFunction jsPDFDocStaticFunctions[] = {
        { "print", jsPDFDocPrint, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { 0, 0, 0 },
    };
    
    static JSClassDefinition jsPDFDocClassDefinition = {
        0,
        kJSClassAttributeNone,
        "Doc",
        0,
        0,
        jsPDFDocStaticFunctions,
        jsPDFDocInitialize, jsPDFDocFinalize, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    static JSClassRef jsPDFDocClass = JSClassCreate(&jsPDFDocClassDefinition);
    
    return JSObjectMake(ctx, jsPDFDocClass, this);
}

static RetainPtr<CFMutableDataRef> convertPostScriptDataToPDF(RetainPtr<CFDataRef> postScriptData)
{
    // Convert PostScript to PDF using the Quartz 2D API.
    // http://developer.apple.com/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/dq_ps_convert/chapter_16_section_1.html

    CGPSConverterCallbacks callbacks = { 0, 0, 0, 0, 0, 0, 0, 0 };
    RetainPtr<CGPSConverterRef> converter = adoptCF(CGPSConverterCreate(0, &callbacks, 0));
    RetainPtr<CGDataProviderRef> provider = adoptCF(CGDataProviderCreateWithCFData(postScriptData.get()));
    RetainPtr<CFMutableDataRef> pdfData = adoptCF(CFDataCreateMutable(kCFAllocatorDefault, 0));
    RetainPtr<CGDataConsumerRef> consumer = adoptCF(CGDataConsumerCreateWithCFData(pdfData.get()));

    CGPSConverterConvert(converter.get(), provider.get(), consumer.get(), 0);

    return pdfData;
}

void SimplePDFPlugin::pdfDocumentDidLoad()
{
    addArchiveResource();

    m_pdfDocument = adoptNS([[pdfDocumentClass() alloc] initWithData:(NSData *)m_data.get()]);

    calculateSizes();
    updateScrollbars();

    controller()->invalidate(IntRect(0, 0, m_size.width(), m_size.height()));

    runScriptsInPDFDocument();
}
    
void SimplePDFPlugin::runScriptsInPDFDocument()
{
    Vector<RetainPtr<CFStringRef>> scripts;
    getAllScriptsInPDFDocument([m_pdfDocument.get() documentRef], scripts);

    size_t scriptCount = scripts.size();
    if (!scriptCount)
        return;

    JSGlobalContextRef ctx = JSGlobalContextCreate(0);
    JSObjectRef jsPDFDoc = makeJSPDFDoc(ctx);

    for (size_t i = 0; i < scriptCount; ++i) {
        JSStringRef script = JSStringCreateWithCFString(scripts[i].get());
        JSEvaluateScript(ctx, script, jsPDFDoc, 0, 0, 0);
        JSStringRelease(script);
    }

    JSGlobalContextRelease(ctx);
}
    
void SimplePDFPlugin::computePageBoxes()
{
    size_t pageCount = CGPDFDocumentGetNumberOfPages([m_pdfDocument.get() documentRef]);
    for (size_t i = 0; i < pageCount; ++i) {
        CGPDFPageRef pdfPage = CGPDFDocumentGetPage([m_pdfDocument.get() documentRef], i + 1);
        ASSERT(pdfPage);
        
        CGRect box = CGPDFPageGetBoxRect(pdfPage, kCGPDFCropBox);
        if (CGRectIsEmpty(box))
            box = CGPDFPageGetBoxRect(pdfPage, kCGPDFMediaBox);
        m_pageBoxes.append(IntRect(box));
    }
}

void SimplePDFPlugin::calculateSizes()
{
    size_t pageCount = CGPDFDocumentGetNumberOfPages([m_pdfDocument.get() documentRef]);
    for (size_t i = 0; i < pageCount; ++i) {
        CGPDFPageRef pdfPage = CGPDFDocumentGetPage([m_pdfDocument.get() documentRef], i + 1);
        ASSERT(pdfPage);

        CGRect box = CGPDFPageGetBoxRect(pdfPage, kCGPDFCropBox);
        if (CGRectIsEmpty(box))
            box = CGPDFPageGetBoxRect(pdfPage, kCGPDFMediaBox);
        m_pageBoxes.append(IntRect(box));
        m_pdfDocumentSize.setWidth(max(m_pdfDocumentSize.width(), static_cast<int>(box.size.width)));
        m_pdfDocumentSize.expand(0, box.size.height);
    }
    m_pdfDocumentSize.expand(0, gutterHeight * (m_pageBoxes.size() - 1));
}

bool SimplePDFPlugin::initialize(const Parameters& parameters)
{
    // Load the src URL if needed.
    m_sourceURL = parameters.url;
    if (!parameters.shouldUseManualLoader && !parameters.url.isEmpty())
        controller()->loadURL(pdfDocumentRequestID, "GET", parameters.url.string(), String(), HTTPHeaderMap(), Vector<uint8_t>(), false);

    controller()->didInitializePlugin();
    return true;
}

void SimplePDFPlugin::destroy()
{
    if (m_frame) {
        if (FrameView* frameView = m_frame->coreFrame()->view())
            frameView->removeScrollableArea(this);
    }

    destroyScrollbar(HorizontalScrollbar);
    destroyScrollbar(VerticalScrollbar);
}

void SimplePDFPlugin::paint(GraphicsContext* graphicsContext, const IntRect& dirtyRect)
{
    contentAreaWillPaint();

    paintBackground(graphicsContext, dirtyRect);

    if (!m_pdfDocument) // FIXME: Draw loading progress.
        return;

    paintContent(graphicsContext, dirtyRect);
    paintControls(graphicsContext, dirtyRect);
}

void SimplePDFPlugin::paintBackground(GraphicsContext* graphicsContext, const IntRect& dirtyRect)
{
    GraphicsContextStateSaver stateSaver(*graphicsContext);
    graphicsContext->setFillColor(Color::gray, ColorSpaceDeviceRGB);
    graphicsContext->fillRect(dirtyRect);
}

void SimplePDFPlugin::paintContent(GraphicsContext* graphicsContext, const IntRect& dirtyRect)
{
    GraphicsContextStateSaver stateSaver(*graphicsContext);
    CGContextRef context = graphicsContext->platformContext();

    graphicsContext->setImageInterpolationQuality(InterpolationHigh);
    graphicsContext->setShouldAntialias(true);
    graphicsContext->setShouldSmoothFonts(true);
    graphicsContext->setFillColor(Color::white, ColorSpaceDeviceRGB);

    graphicsContext->clip(dirtyRect);
    IntRect contentRect(dirtyRect);
    contentRect.moveBy(IntPoint(m_scrollOffset));
    graphicsContext->translate(-m_scrollOffset.width(), -m_scrollOffset.height());

    CGContextScaleCTM(context, 1, -1);

    int pageTop = 0;
    for (size_t i = 0; i < m_pageBoxes.size(); ++i) {
        IntRect pageBox = m_pageBoxes[i];
        float extraOffsetForCenteringX = max(roundf((m_size.width() - pageBox.width()) / 2.0f), 0.0f);
        float extraOffsetForCenteringY = (m_pageBoxes.size() == 1) ? max(roundf((m_size.height() - pageBox.height() + shadowOffsetY) / 2.0f), 0.0f) : 0;

        if (pageTop > contentRect.maxY())
            break;
        if (pageTop + pageBox.height() + extraOffsetForCenteringY + gutterHeight >= contentRect.y()) {
            CGPDFPageRef pdfPage = CGPDFDocumentGetPage([m_pdfDocument.get() documentRef], i + 1);

            graphicsContext->save();
            graphicsContext->translate(extraOffsetForCenteringX - pageBox.x(), -extraOffsetForCenteringY - pageBox.y() - pageBox.height());

            graphicsContext->setShadow(FloatSize(shadowOffsetX, shadowOffsetY), shadowSize, Color::black, ColorSpaceDeviceRGB);
            graphicsContext->fillRect(pageBox);
            graphicsContext->clearShadow();

            graphicsContext->clip(pageBox);

            CGContextDrawPDFPage(context, pdfPage);
            graphicsContext->restore();
        }
        pageTop += pageBox.height() + gutterHeight;
        CGContextTranslateCTM(context, 0, -pageBox.height() - gutterHeight);
    }
}

void SimplePDFPlugin::paintControls(GraphicsContext* graphicsContext, const IntRect& dirtyRect)
{
    {
        GraphicsContextStateSaver stateSaver(*graphicsContext);
        IntRect scrollbarDirtyRect = dirtyRect;
        scrollbarDirtyRect.moveBy(pluginView()->frameRect().location());
        graphicsContext->translate(-pluginView()->frameRect().x(), -pluginView()->frameRect().y());

        if (m_horizontalScrollbar)
            m_horizontalScrollbar->paint(graphicsContext, scrollbarDirtyRect);

        if (m_verticalScrollbar)
            m_verticalScrollbar->paint(graphicsContext, scrollbarDirtyRect);
    }

    IntRect dirtyCornerRect = intersection(scrollCornerRect(), dirtyRect);
    ScrollbarTheme::theme()->paintScrollCorner(0, graphicsContext, dirtyCornerRect);
}

void SimplePDFPlugin::updateControlTints(GraphicsContext* graphicsContext)
{
    ASSERT(graphicsContext->updatingControlTints());

    if (m_horizontalScrollbar)
        m_horizontalScrollbar->invalidate();
    if (m_verticalScrollbar)
        m_verticalScrollbar->invalidate();
    invalidateScrollCorner(scrollCornerRect());
}

PassRefPtr<ShareableBitmap> SimplePDFPlugin::snapshot()
{
    return 0;
}

#if PLATFORM(MAC)
PlatformLayer* SimplePDFPlugin::pluginLayer()
{
    return 0;
}
#endif


bool SimplePDFPlugin::isTransparent()
{
    // This should never be called from the web process.
    ASSERT_NOT_REACHED();
    return false;
}

bool SimplePDFPlugin::wantsWheelEvents()
{
    return true;
}

void SimplePDFPlugin::geometryDidChange(const IntSize& size, const IntRect& clipRect, const AffineTransform& pluginToRootViewTransform)
{
    if (m_size == size) {
        // Nothing to do.
        return;
    }

    m_size = size;
    updateScrollbars();
}

void SimplePDFPlugin::visibilityDidChange()
{
}

void SimplePDFPlugin::frameDidFinishLoading(uint64_t)
{
    ASSERT_NOT_REACHED();
}

void SimplePDFPlugin::frameDidFail(uint64_t, bool)
{
    ASSERT_NOT_REACHED();
}

void SimplePDFPlugin::didEvaluateJavaScript(uint64_t, const WTF::String&)
{
    ASSERT_NOT_REACHED();
}

void SimplePDFPlugin::convertPostScriptDataIfNeeded()
{
    if (!m_isPostScript)
        return;

    m_suggestedFilename = String(m_suggestedFilename + ".pdf");
    m_data = convertPostScriptDataToPDF(m_data);
}

void SimplePDFPlugin::streamDidReceiveResponse(uint64_t streamID, const KURL&, uint32_t, uint32_t, const String& mimeType, const String&, const String& suggestedFilename)
{
    ASSERT_UNUSED(streamID, streamID == pdfDocumentRequestID);

    m_suggestedFilename = suggestedFilename;

    if (equalIgnoringCase(mimeType, postScriptMIMEType))
        m_isPostScript = true;
}
                                           
void SimplePDFPlugin::streamDidReceiveData(uint64_t streamID, const char* bytes, int length)
{
    ASSERT_UNUSED(streamID, streamID == pdfDocumentRequestID);

    if (!m_data)
        m_data = adoptCF(CFDataCreateMutable(0, 0));

    CFDataAppendBytes(m_data.get(), reinterpret_cast<const UInt8*>(bytes), length);
}

void SimplePDFPlugin::streamDidFinishLoading(uint64_t streamID)
{
    ASSERT_UNUSED(streamID, streamID == pdfDocumentRequestID);

    convertPostScriptDataIfNeeded();
    pdfDocumentDidLoad();
}

void SimplePDFPlugin::streamDidFail(uint64_t streamID, bool wasCancelled)
{
    ASSERT_UNUSED(streamID, streamID == pdfDocumentRequestID);

    m_data.clear();
}

void SimplePDFPlugin::manualStreamDidReceiveResponse(const KURL& responseURL, uint32_t streamLength,  uint32_t lastModifiedTime, const String& mimeType, const String& headers, const String& suggestedFilename)
{
    m_suggestedFilename = suggestedFilename;

    if (equalIgnoringCase(mimeType, postScriptMIMEType))
        m_isPostScript = true;
}

void SimplePDFPlugin::manualStreamDidReceiveData(const char* bytes, int length)
{
    if (!m_data)
        m_data = adoptCF(CFDataCreateMutable(0, 0));

    CFDataAppendBytes(m_data.get(), reinterpret_cast<const UInt8*>(bytes), length);
}

void SimplePDFPlugin::manualStreamDidFinishLoading()
{
    convertPostScriptDataIfNeeded();
    pdfDocumentDidLoad();
}

NSData *SimplePDFPlugin::liveData() const
{
    // Save data straight from the resource instead of PDFKit if the document is
    // untouched by the user, so that PDFs which PDFKit can't display will still be downloadable.
    if (pdfDocumentWasMutated())
        return [m_pdfDocument.get() dataRepresentation];
    else
        return rawData();
}

PassRefPtr<SharedBuffer> SimplePDFPlugin::liveResourceData() const
{
    NSData *pdfData = liveData();

    if (!pdfData)
        return 0;

    return SharedBuffer::wrapNSData(pdfData);
}

void SimplePDFPlugin::manualStreamDidFail(bool)
{
    m_data.clear();
}

bool SimplePDFPlugin::handleMouseEvent(const WebMouseEvent& event)
{
    switch (event.type()) {
    case WebEvent::MouseMove:
        mouseMovedInContentArea();
        // FIXME: Should also notify scrollbar to show hover effect. Should also send mouseExited to hide it.
        break;
    case WebEvent::MouseDown: {
        // Returning false as will make EventHandler unfocus the plug-in, which is appropriate when clicking scrollbars.
        // Ideally, we wouldn't change focus at all, but PluginView already did that for us.
        // When support for PDF forms is added, we'll need to actually focus the plug-in when clicking in a form.
        break;
    }
    case WebEvent::MouseUp: {
        PlatformMouseEvent platformEvent = platform(event);
        if (m_horizontalScrollbar)
            m_horizontalScrollbar->mouseUp(platformEvent);
        if (m_verticalScrollbar)
            m_verticalScrollbar->mouseUp(platformEvent);
        break;
    }
    default:
        break;
    }
        
    return false;
}

bool SimplePDFPlugin::handleWheelEvent(const WebWheelEvent& event)
{
    PlatformWheelEvent platformEvent = platform(event);
    return ScrollableArea::handleWheelEvent(platformEvent);
}

bool SimplePDFPlugin::handleMouseEnterEvent(const WebMouseEvent&)
{
    mouseEnteredContentArea();
    return false;
}

bool SimplePDFPlugin::handleMouseLeaveEvent(const WebMouseEvent&)
{
    mouseExitedContentArea();
    return false;
}

bool SimplePDFPlugin::handleContextMenuEvent(const WebMouseEvent&)
{
    // Use default WebKit context menu.
    return false;
}

bool SimplePDFPlugin::handleKeyboardEvent(const WebKeyboardEvent&)
{
    return false;
}

void SimplePDFPlugin::setFocus(bool hasFocus)
{
}

NPObject* SimplePDFPlugin::pluginScriptableNPObject()
{
    return 0;
}

#if PLATFORM(MAC)

void SimplePDFPlugin::windowFocusChanged(bool)
{
}

void SimplePDFPlugin::windowAndViewFramesChanged(const WebCore::IntRect& windowFrameInScreenCoordinates, const WebCore::IntRect& viewFrameInWindowCoordinates)
{
}

void SimplePDFPlugin::windowVisibilityChanged(bool)
{
}

void SimplePDFPlugin::contentsScaleFactorChanged(float)
{
}

uint64_t SimplePDFPlugin::pluginComplexTextInputIdentifier() const
{
    return 0;
}

void SimplePDFPlugin::sendComplexTextInput(const String&)
{
}

void SimplePDFPlugin::setLayerHostingMode(LayerHostingMode)
{
}

#endif

void SimplePDFPlugin::storageBlockingStateChanged(bool)
{
}

void SimplePDFPlugin::privateBrowsingStateChanged(bool)
{
}

bool SimplePDFPlugin::getFormValue(String&)
{
    return false;
}

bool SimplePDFPlugin::handleScroll(ScrollDirection direction, ScrollGranularity granularity)
{
    return scroll(direction, granularity);
}

Scrollbar* SimplePDFPlugin::horizontalScrollbar()
{
    return m_horizontalScrollbar.get();
}

Scrollbar* SimplePDFPlugin::verticalScrollbar()
{
    return m_verticalScrollbar.get();
}

IntRect SimplePDFPlugin::scrollCornerRect() const
{
    if (!m_horizontalScrollbar || !m_verticalScrollbar)
        return IntRect();
    if (m_horizontalScrollbar->isOverlayScrollbar()) {
        ASSERT(m_verticalScrollbar->isOverlayScrollbar());
        return IntRect();
    }
    return IntRect(pluginView()->width() - m_verticalScrollbar->width(), pluginView()->height() - m_horizontalScrollbar->height(), m_verticalScrollbar->width(), m_horizontalScrollbar->height());
}

ScrollableArea* SimplePDFPlugin::enclosingScrollableArea() const
{
    // FIXME: Walk up the frame tree and look for a scrollable parent frame or RenderLayer.
    return 0;
}

IntRect SimplePDFPlugin::scrollableAreaBoundingBox() const
{
    return pluginView()->frameRect();
}

void SimplePDFPlugin::setScrollOffset(const IntPoint& offset)
{
    m_scrollOffset = IntSize(offset.x(), offset.y());
    // FIXME: It would be better for performance to blit parts that remain visible.
    controller()->invalidate(IntRect(0, 0, m_size.width(), m_size.height()));
}

int SimplePDFPlugin::scrollSize(ScrollbarOrientation orientation) const
{
    Scrollbar* scrollbar = ((orientation == HorizontalScrollbar) ? m_horizontalScrollbar : m_verticalScrollbar).get();
    return scrollbar ? (scrollbar->totalSize() - scrollbar->visibleSize()) : 0;
}

bool SimplePDFPlugin::isActive() const
{
    if (Frame* coreFrame = m_frame->coreFrame()) {
        if (Page* page = coreFrame->page())
            return page->focusController()->isActive();
    }

    return false;
}

void SimplePDFPlugin::invalidateScrollbarRect(Scrollbar* scrollbar, const IntRect& rect)
{
    IntRect dirtyRect = rect;
    dirtyRect.moveBy(scrollbar->location());
    dirtyRect.moveBy(-pluginView()->location());
    controller()->invalidate(dirtyRect);
}

void SimplePDFPlugin::invalidateScrollCornerRect(const IntRect& rect)
{
    controller()->invalidate(rect);
}

bool SimplePDFPlugin::isScrollCornerVisible() const
{
    return false;
}

int SimplePDFPlugin::scrollPosition(Scrollbar* scrollbar) const
{
    if (scrollbar->orientation() == HorizontalScrollbar)
        return m_scrollOffset.width();
    if (scrollbar->orientation() == VerticalScrollbar)
        return m_scrollOffset.height();
    ASSERT_NOT_REACHED();
    return 0;
}

IntPoint SimplePDFPlugin::scrollPosition() const
{
    return IntPoint(m_scrollOffset.width(), m_scrollOffset.height());
}

IntPoint SimplePDFPlugin::minimumScrollPosition() const
{
    return IntPoint(0, 0);
}

IntPoint SimplePDFPlugin::maximumScrollPosition() const
{
    int horizontalScrollbarHeight = (m_horizontalScrollbar && !m_horizontalScrollbar->isOverlayScrollbar()) ? m_horizontalScrollbar->height() : 0;
    int verticalScrollbarWidth = (m_verticalScrollbar && !m_verticalScrollbar->isOverlayScrollbar()) ? m_verticalScrollbar->width() : 0;

    IntPoint maximumOffset(m_pdfDocumentSize.width() - m_size.width() + verticalScrollbarWidth, m_pdfDocumentSize.height() - m_size.height() + horizontalScrollbarHeight);
    maximumOffset.clampNegativeToZero();
    return maximumOffset;
}

int SimplePDFPlugin::visibleHeight() const
{
    return m_size.height();
}

int SimplePDFPlugin::visibleWidth() const
{
    return m_size.width();
}

IntSize SimplePDFPlugin::contentsSize() const
{
    return m_pdfDocumentSize;
}

bool SimplePDFPlugin::scrollbarsCanBeActive() const
{
    return !pluginView()->frame()->document()->inPageCache();
}

void SimplePDFPlugin::scrollbarStyleChanged(int, bool forceUpdate)
{
    if (!forceUpdate)
        return;

    // If the PDF was scrolled all the way to bottom right and scrollbars change to overlay style, we don't want to display white rectangles where scrollbars were.
    IntPoint newScrollOffset = IntPoint(m_scrollOffset).shrunkTo(maximumScrollPosition());
    setScrollOffset(newScrollOffset);

    // As size of the content area changes, scrollbars may need to appear or to disappear.
    updateScrollbars();

    ScrollableArea::contentsResized();
}

IntRect SimplePDFPlugin::convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntRect& scrollbarRect) const
{
    IntRect rect = scrollbarRect;
    rect.move(scrollbar->location() - pluginView()->location());
    
    return pluginView()->frame()->view()->convertFromRenderer(pluginView()->renderer(), rect);
}

IntRect SimplePDFPlugin::convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntRect& parentRect) const
{
    IntRect rect = pluginView()->frame()->view()->convertToRenderer(pluginView()->renderer(), parentRect);
    rect.move(pluginView()->location() - scrollbar->location());
    
    return rect;
}

IntPoint SimplePDFPlugin::convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntPoint& scrollbarPoint) const
{
    IntPoint point = scrollbarPoint;
    point.move(scrollbar->location() - pluginView()->location());
    
    return pluginView()->frame()->view()->convertFromRenderer(pluginView()->renderer(), point);
}

IntPoint SimplePDFPlugin::convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntPoint& parentPoint) const
{
    IntPoint point = pluginView()->frame()->view()->convertToRenderer(pluginView()->renderer(), parentPoint);
    point.move(pluginView()->location() - scrollbar->location());
    
    return point;
}

bool SimplePDFPlugin::isEditingCommandEnabled(const String&)
{
    return false;
}

bool SimplePDFPlugin::handleEditingCommand(const String&, const String&)
{
    return false;
}
    
bool SimplePDFPlugin::handlesPageScaleFactor()
{
    return false;
}

} // namespace WebKit
