/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorOverlay.h"

#include "DocumentLoader.h"
#include "Element.h"
#include "EmptyClients.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "InspectorClient.h"
#include "InspectorOverlayPage.h"
#include "InspectorValues.h"
#include "Node.h"
#include "Page.h"
#include "RenderBoxModelObject.h"
#include "RenderInline.h"
#include "RenderObject.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "Settings.h"
#include "StyledElement.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

namespace {

Path quadToPath(const FloatQuad& quad)
{
    Path quadPath;
    quadPath.moveTo(quad.p1());
    quadPath.addLineTo(quad.p2());
    quadPath.addLineTo(quad.p3());
    quadPath.addLineTo(quad.p4());
    quadPath.closeSubpath();
    return quadPath;
}

void drawOutlinedQuad(GraphicsContext* context, const FloatQuad& quad, const Color& fillColor, const Color& outlineColor)
{
    static const int outlineThickness = 2;

    Path quadPath = quadToPath(quad);

    // Clip out the quad, then draw with a 2px stroke to get a pixel
    // of outline (because inflating a quad is hard)
    {
        context->save();
        context->clipOut(quadPath);

        context->setStrokeThickness(outlineThickness);
        context->setStrokeColor(outlineColor, ColorSpaceDeviceRGB);
        context->strokePath(quadPath);

        context->restore();
    }

    // Now do the fill
    context->setFillColor(fillColor, ColorSpaceDeviceRGB);
    context->fillPath(quadPath);
}

static void contentsQuadToPage(const FrameView* mainView, const FrameView* view, FloatQuad& quad)
{
    quad.setP1(view->contentsToRootView(roundedIntPoint(quad.p1())));
    quad.setP2(view->contentsToRootView(roundedIntPoint(quad.p2())));
    quad.setP3(view->contentsToRootView(roundedIntPoint(quad.p3())));
    quad.setP4(view->contentsToRootView(roundedIntPoint(quad.p4())));
    quad += mainView->scrollOffset();
}

static void buildNodeHighlight(Node* node, const HighlightConfig& highlightConfig, Highlight* highlight)
{
    RenderObject* renderer = node->renderer();
    Frame* containingFrame = node->document()->frame();

    if (!renderer || !containingFrame)
        return;

    highlight->setDataFromConfig(highlightConfig);
    FrameView* containingView = containingFrame->view();
    FrameView* mainView = containingFrame->page()->mainFrame()->view();
    IntRect boundingBox = pixelSnappedIntRect(containingView->contentsToRootView(renderer->absoluteBoundingBoxRect()));
    boundingBox.move(mainView->scrollOffset());
    IntRect titleAnchorBox = boundingBox;

    // RenderSVGRoot should be highlighted through the isBox() code path, all other SVG elements should just dump their absoluteQuads().
#if ENABLE(SVG)
    bool isSVGRenderer = renderer->node() && renderer->node()->isSVGElement() && !renderer->isSVGRoot();
#else
    bool isSVGRenderer = false;
#endif

    if (isSVGRenderer) {
        highlight->type = HighlightTypeRects;
        renderer->absoluteQuads(highlight->quads);
        for (size_t i = 0; i < highlight->quads.size(); ++i)
            contentsQuadToPage(mainView, containingView, highlight->quads[i]);
    } else if (renderer->isBox() || renderer->isRenderInline()) {
        LayoutRect contentBox;
        LayoutRect paddingBox;
        LayoutRect borderBox;
        LayoutRect marginBox;

        if (renderer->isBox()) {
            RenderBox* renderBox = toRenderBox(renderer);

            // RenderBox returns the "pure" content area box, exclusive of the scrollbars (if present), which also count towards the content area in CSS.
            contentBox = renderBox->contentBoxRect();
            contentBox.setWidth(contentBox.width() + renderBox->verticalScrollbarWidth());
            contentBox.setHeight(contentBox.height() + renderBox->horizontalScrollbarHeight());

            paddingBox = LayoutRect(contentBox.x() - renderBox->paddingLeft(), contentBox.y() - renderBox->paddingTop(),
                    contentBox.width() + renderBox->paddingLeft() + renderBox->paddingRight(), contentBox.height() + renderBox->paddingTop() + renderBox->paddingBottom());
            borderBox = LayoutRect(paddingBox.x() - renderBox->borderLeft(), paddingBox.y() - renderBox->borderTop(),
                    paddingBox.width() + renderBox->borderLeft() + renderBox->borderRight(), paddingBox.height() + renderBox->borderTop() + renderBox->borderBottom());
            marginBox = LayoutRect(borderBox.x() - renderBox->marginLeft(), borderBox.y() - renderBox->marginTop(),
                    borderBox.width() + renderBox->marginWidth(), borderBox.height() + renderBox->marginHeight());
        } else {
            RenderInline* renderInline = toRenderInline(renderer);

            // RenderInline's bounding box includes paddings and borders, excludes margins.
            borderBox = renderInline->linesBoundingBox();
            paddingBox = LayoutRect(borderBox.x() + renderInline->borderLeft(), borderBox.y() + renderInline->borderTop(),
                    borderBox.width() - renderInline->borderLeft() - renderInline->borderRight(), borderBox.height() - renderInline->borderTop() - renderInline->borderBottom());
            contentBox = LayoutRect(paddingBox.x() + renderInline->paddingLeft(), paddingBox.y() + renderInline->paddingTop(),
                    paddingBox.width() - renderInline->paddingLeft() - renderInline->paddingRight(), paddingBox.height() - renderInline->paddingTop() - renderInline->paddingBottom());
            // Ignore marginTop and marginBottom for inlines.
            marginBox = LayoutRect(borderBox.x() - renderInline->marginLeft(), borderBox.y(),
                    borderBox.width() + renderInline->marginWidth(), borderBox.height());
        }

        FloatQuad absContentQuad = renderer->localToAbsoluteQuad(FloatRect(contentBox));
        FloatQuad absPaddingQuad = renderer->localToAbsoluteQuad(FloatRect(paddingBox));
        FloatQuad absBorderQuad = renderer->localToAbsoluteQuad(FloatRect(borderBox));
        FloatQuad absMarginQuad = renderer->localToAbsoluteQuad(FloatRect(marginBox));

        contentsQuadToPage(mainView, containingView, absContentQuad);
        contentsQuadToPage(mainView, containingView, absPaddingQuad);
        contentsQuadToPage(mainView, containingView, absBorderQuad);
        contentsQuadToPage(mainView, containingView, absMarginQuad);

        titleAnchorBox = absMarginQuad.enclosingBoundingBox();

        highlight->type = HighlightTypeNode;
        highlight->quads.append(absMarginQuad);
        highlight->quads.append(absBorderQuad);
        highlight->quads.append(absPaddingQuad);
        highlight->quads.append(absContentQuad);
    }
}

static void buildQuadHighlight(Page* page, const FloatQuad& quad, const HighlightConfig& highlightConfig, Highlight *highlight)
{
    if (!page)
        return;
    highlight->setDataFromConfig(highlightConfig);
    highlight->type = HighlightTypeRects;
    highlight->quads.append(quad);
}

} // anonymous namespace

InspectorOverlay::InspectorOverlay(Page* page, InspectorClient* client)
    : m_page(page)
    , m_client(client)
{
}

InspectorOverlay::~InspectorOverlay()
{
}

void InspectorOverlay::paint(GraphicsContext& context)
{
    if (m_pausedInDebuggerMessage.isNull() && !m_highlightNode && !m_highlightQuad && m_size.isEmpty())
        return;
    GraphicsContextStateSaver stateSaver(context);
    FrameView* view = overlayPage()->mainFrame()->view();
    ASSERT(!view->needsLayout());
    view->paint(&context, IntRect(0, 0, view->width(), view->height()));
}

void InspectorOverlay::drawOutline(GraphicsContext* context, const LayoutRect& rect, const Color& color)
{
    FloatRect outlineRect = rect;
    drawOutlinedQuad(context, outlineRect, Color(), color);
}

void InspectorOverlay::getHighlight(Highlight* highlight) const
{
    if (!m_highlightNode && !m_highlightQuad)
        return;

    highlight->type = HighlightTypeRects;
    if (m_highlightNode)
        buildNodeHighlight(m_highlightNode.get(), m_nodeHighlightConfig, highlight);
    else
        buildQuadHighlight(m_page, *m_highlightQuad, m_quadHighlightConfig, highlight);
}

void InspectorOverlay::resize(const IntSize& size)
{
    m_size = size;
    update();
}

void InspectorOverlay::setPausedInDebuggerMessage(const String* message)
{
    m_pausedInDebuggerMessage = message ? *message : String();
    update();
}

void InspectorOverlay::hideHighlight()
{
    m_highlightNode.clear();
    m_highlightQuad.clear();
    update();
}

void InspectorOverlay::highlightNode(Node* node, const HighlightConfig& highlightConfig)
{
    m_nodeHighlightConfig = highlightConfig;
    m_highlightNode = node;
    update();
}

void InspectorOverlay::highlightQuad(PassOwnPtr<FloatQuad> quad, const HighlightConfig& highlightConfig)
{
    if (m_quadHighlightConfig.usePageCoordinates)
        *quad -= m_page->mainFrame()->view()->scrollOffset();

    m_quadHighlightConfig = highlightConfig;
    m_highlightQuad = quad;
    update();
}

Node* InspectorOverlay::highlightedNode() const
{
    return m_highlightNode.get();
}

void InspectorOverlay::update()
{
    if (!m_highlightNode && !m_highlightQuad && m_pausedInDebuggerMessage.isNull() && m_size.isEmpty()) {
        m_client->hideHighlight();
        return;
    }

    FrameView* view = m_page->mainFrame()->view();
    if (!view)
        return;

    FrameView* overlayView = overlayPage()->mainFrame()->view();
    IntSize viewportSize = view->visibleContentRect().size();
    IntSize frameViewFullSize = view->visibleContentRect(ScrollableArea::IncludeScrollbars).size();
    IntSize size = m_size.isEmpty() ? frameViewFullSize : m_size;
    overlayPage()->setPageScaleFactor(m_page->pageScaleFactor(), IntPoint());
    size.scale(m_page->pageScaleFactor());
    overlayView->resize(size);

    // Clear canvas and paint things.
    reset(viewportSize, m_size.isEmpty() ? IntSize() : frameViewFullSize);

    // Include scrollbars to avoid masking them by the gutter.
    drawGutter();
    drawNodeHighlight();
    drawQuadHighlight();
    drawPausedInDebuggerMessage();

    // Position DOM elements.
    overlayPage()->mainFrame()->document()->recalcStyle(Node::Force);
    if (overlayView->needsLayout())
        overlayView->layout();

    // Kick paint.
    m_client->highlight();
}

static PassRefPtr<InspectorObject> buildObjectForPoint(const FloatPoint& point)
{
    RefPtr<InspectorObject> object = InspectorObject::create();
    object->setNumber("x", point.x());
    object->setNumber("y", point.y());
    return object.release();
}

static PassRefPtr<InspectorArray> buildArrayForQuad(const FloatQuad& quad)
{
    RefPtr<InspectorArray> array = InspectorArray::create();
    array->pushObject(buildObjectForPoint(quad.p1()));
    array->pushObject(buildObjectForPoint(quad.p2()));
    array->pushObject(buildObjectForPoint(quad.p3()));
    array->pushObject(buildObjectForPoint(quad.p4()));
    return array.release();
}

static PassRefPtr<InspectorObject> buildObjectForHighlight(FrameView* mainView, const Highlight& highlight)
{
    RefPtr<InspectorObject> object = InspectorObject::create();
    RefPtr<InspectorArray> array = InspectorArray::create();
    for (size_t i = 0; i < highlight.quads.size(); ++i)
        array->pushArray(buildArrayForQuad(highlight.quads[i]));
    object->setArray("quads", array.release());
    object->setBoolean("showRulers", highlight.showRulers);
    object->setString("contentColor", highlight.contentColor.serialized());
    object->setString("contentOutlineColor", highlight.contentOutlineColor.serialized());
    object->setString("paddingColor", highlight.paddingColor.serialized());
    object->setString("borderColor", highlight.borderColor.serialized());
    object->setString("marginColor", highlight.marginColor.serialized());

    FloatRect visibleRect = mainView->visibleContentRect();
    if (!mainView->delegatesScrolling()) {
        object->setNumber("scrollX", visibleRect.x());
        object->setNumber("scrollY", visibleRect.y());
    } else {
        object->setNumber("scrollX", 0);
        object->setNumber("scrollY", 0);
    }

    return object.release();
}

static PassRefPtr<InspectorObject> buildObjectForSize(const IntSize& size)
{
    RefPtr<InspectorObject> result = InspectorObject::create();
    result->setNumber("width", size.width());
    result->setNumber("height", size.height());
    return result.release();
}

void InspectorOverlay::drawGutter()
{
    evaluateInOverlay("drawGutter", "");
}

void InspectorOverlay::drawNodeHighlight()
{
    if (!m_highlightNode)
        return;

    Highlight highlight;
    buildNodeHighlight(m_highlightNode.get(), m_nodeHighlightConfig, &highlight);
    RefPtr<InspectorObject> highlightObject = buildObjectForHighlight(m_page->mainFrame()->view(), highlight);

    Node* node = m_highlightNode.get();
    if (node->isElementNode() && m_nodeHighlightConfig.showInfo && node->renderer() && node->document()->frame()) {
        RefPtr<InspectorObject> elementInfo = InspectorObject::create();
        Element* element = toElement(node);
        bool isXHTML = element->document()->isXHTMLDocument();
        elementInfo->setString("tagName", isXHTML ? element->nodeName() : element->nodeName().lower());
        elementInfo->setString("idValue", element->getIdAttribute());
        HashSet<AtomicString> usedClassNames;
        if (element->hasClass() && element->isStyledElement()) {
            StringBuilder classNames;
            const SpaceSplitString& classNamesString = static_cast<StyledElement*>(element)->classNames();
            size_t classNameCount = classNamesString.size();
            for (size_t i = 0; i < classNameCount; ++i) {
                const AtomicString& className = classNamesString[i];
                if (usedClassNames.contains(className))
                    continue;
                usedClassNames.add(className);
                classNames.append('.');
                classNames.append(className);
            }
            elementInfo->setString("className", classNames.toString());
        }

        RenderObject* renderer = node->renderer();
        Frame* containingFrame = node->document()->frame();
        FrameView* containingView = containingFrame->view();
        IntRect boundingBox = pixelSnappedIntRect(containingView->contentsToRootView(renderer->absoluteBoundingBoxRect()));
        RenderBoxModelObject* modelObject = renderer->isBoxModelObject() ? toRenderBoxModelObject(renderer) : 0;
        elementInfo->setString("nodeWidth", String::number(modelObject ? adjustForAbsoluteZoom(modelObject->pixelSnappedOffsetWidth(), modelObject) : boundingBox.width()));
        elementInfo->setString("nodeHeight", String::number(modelObject ? adjustForAbsoluteZoom(modelObject->pixelSnappedOffsetHeight(), modelObject) : boundingBox.height()));
        highlightObject->setObject("elementInfo", elementInfo.release());
    }
    evaluateInOverlay("drawNodeHighlight", highlightObject);
}

void InspectorOverlay::drawQuadHighlight()
{
    if (!m_highlightQuad)
        return;

    Highlight highlight;
    buildQuadHighlight(m_page, *m_highlightQuad, m_quadHighlightConfig, &highlight);
    evaluateInOverlay("drawQuadHighlight", buildObjectForHighlight(m_page->mainFrame()->view(), highlight));
}

void InspectorOverlay::drawPausedInDebuggerMessage()
{
    if (!m_pausedInDebuggerMessage.isNull())
        evaluateInOverlay("drawPausedInDebuggerMessage", m_pausedInDebuggerMessage);
}

Page* InspectorOverlay::overlayPage()
{
    if (m_overlayPage)
        return m_overlayPage.get();

    static FrameLoaderClient* dummyFrameLoaderClient =  new EmptyFrameLoaderClient;
    Page::PageClients pageClients;
    fillWithEmptyClients(pageClients);
    m_overlayPage = adoptPtr(new Page(pageClients));

    Settings* settings = m_page->settings();
    Settings* overlaySettings = m_overlayPage->settings();

    overlaySettings->setStandardFontFamily(settings->standardFontFamily());
    overlaySettings->setSerifFontFamily(settings->serifFontFamily());
    overlaySettings->setSansSerifFontFamily(settings->sansSerifFontFamily());
    overlaySettings->setCursiveFontFamily(settings->cursiveFontFamily());
    overlaySettings->setFantasyFontFamily(settings->fantasyFontFamily());
    overlaySettings->setPictographFontFamily(settings->pictographFontFamily());
    overlaySettings->setMinimumFontSize(settings->minimumFontSize());
    overlaySettings->setMinimumLogicalFontSize(settings->minimumLogicalFontSize());
    overlaySettings->setMediaEnabled(false);
    overlaySettings->setScriptEnabled(true);
    overlaySettings->setPluginsEnabled(false);

    RefPtr<Frame> frame = Frame::create(m_overlayPage.get(), 0, dummyFrameLoaderClient);
    frame->setView(FrameView::create(frame.get()));
    frame->init();
    FrameLoader* loader = frame->loader();
    frame->view()->setCanHaveScrollbars(false);
    frame->view()->setTransparent(true);
    ASSERT(loader->activeDocumentLoader());
    loader->activeDocumentLoader()->writer()->setMIMEType("text/html");
    loader->activeDocumentLoader()->writer()->begin();
    loader->activeDocumentLoader()->writer()->addData(reinterpret_cast<const char*>(InspectorOverlayPage_html), sizeof(InspectorOverlayPage_html));
    loader->activeDocumentLoader()->writer()->end();

#if OS(WINDOWS)
    evaluateInOverlay("setPlatform", "windows");
#elif OS(MAC_OS_X)
    evaluateInOverlay("setPlatform", "mac");
#elif OS(UNIX)
    evaluateInOverlay("setPlatform", "linux");
#endif

    return m_overlayPage.get();
}

void InspectorOverlay::reset(const IntSize& viewportSize, const IntSize& frameViewFullSize)
{
    RefPtr<InspectorObject> resetData = InspectorObject::create();
    resetData->setNumber("deviceScaleFactor", m_page->deviceScaleFactor());
    resetData->setObject("viewportSize", buildObjectForSize(viewportSize));
    resetData->setObject("frameViewFullSize", buildObjectForSize(frameViewFullSize));
    evaluateInOverlay("reset", resetData.release());
}

void InspectorOverlay::evaluateInOverlay(const String& method, const String& argument)
{
    RefPtr<InspectorArray> command = InspectorArray::create();
    command->pushString(method);
    command->pushString(argument);
    overlayPage()->mainFrame()->script()->evaluate(ScriptSourceCode(makeString("dispatch(", command->toJSONString(), ")")));
}

void InspectorOverlay::evaluateInOverlay(const String& method, PassRefPtr<InspectorValue> argument)
{
    RefPtr<InspectorArray> command = InspectorArray::create();
    command->pushString(method);
    command->pushValue(argument);
    overlayPage()->mainFrame()->script()->evaluate(ScriptSourceCode(makeString("dispatch(", command->toJSONString(), ")")));
}

void InspectorOverlay::freePage()
{
    m_overlayPage.clear();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
